// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "hal/Clv0001KeyboardBackend.h"
#include "core/DiagnosticRedactor.h"
#include "hal/SysfsRoot.h"

#include <QElapsedTimer>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include <limits>

namespace acc::hal {

namespace {

QMutex& clvBackendMutex() {
    static QMutex s_mutex;
    return s_mutex;
}

int g_minIntervalMs = Clv0001KeyboardBackend::kDefaultMinIntervalMs;
qint64 g_lastCommandMs = -1;

QElapsedTimer& rateTimer() {
    static QElapsedTimer s_timer;
    if (!s_timer.isValid()) {
        s_timer.start();
    }
    return s_timer;
}

QMap<QString, QString> parseKeyValueState(const QString& stateText) {
    QMap<QString, QString> kv;
    const QStringList lines = stateText.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString& line : lines) {
        const int eq = line.indexOf(QLatin1Char('='));
        if (eq > 0) {
            kv.insert(line.left(eq).trimmed(), line.mid(eq + 1).trimmed());
        }
    }
    return kv;
}

} // namespace

const std::vector<core::Clv0001PresetInfo>& Clv0001KeyboardBackend::verifiedPresets() {
    static const std::vector<core::Clv0001PresetInfo> kPresets = {
        { QStringLiteral("off"),          0x00, 0x00, 0x00, QStringLiteral("#000000") },
        { QStringLiteral("red"),          0x00, 0xFF, 0x00, QStringLiteral("#FF0000") },
        { QStringLiteral("orange"),       0x7F, 0xFF, 0x00, QStringLiteral("#FF7F00") },
        { QStringLiteral("yellow"),       0xFF, 0xFF, 0x00, QStringLiteral("#FFFF00") },
        { QStringLiteral("lime"),         0xFF, 0x7F, 0x00, QStringLiteral("#7FFF00") },
        { QStringLiteral("light-green"),  0xFF, 0x3F, 0x00, QStringLiteral("#3FFF00") },
        { QStringLiteral("green"),        0xFF, 0x00, 0x00, QStringLiteral("#00FF00") },
        { QStringLiteral("green-cyan"),   0xFF, 0x00, 0x46, QStringLiteral("#00FF46") },
        { QStringLiteral("cyan"),         0xFF, 0x00, 0xC8, QStringLiteral("#00FFC8") },
        { QStringLiteral("light-blue"),   0x7F, 0x00, 0xC8, QStringLiteral("#007FC8") },
        { QStringLiteral("blue"),         0x00, 0x00, 0xC8, QStringLiteral("#0000C8") },
        { QStringLiteral("violet"),       0x00, 0x7F, 0xC8, QStringLiteral("#7F00C8") },
        { QStringLiteral("magenta"),      0x00, 0xFF, 0xC8, QStringLiteral("#FF00C8") },
        { QStringLiteral("pink"),         0x00, 0xFF, 0x7F, QStringLiteral("#FF007F") },
        { QStringLiteral("flesh"),        0x7F, 0xFF, 0x3F, QStringLiteral("#FF7F3F") },
        { QStringLiteral("bluish-white"), 0x7F, 0x3F, 0x7F, QStringLiteral("#3F7F7F") },
        { QStringLiteral("white"),        0xFF, 0xFF, 0xFF, QStringLiteral("#FFFFFF") },
    };
    return kPresets;
}

bool Clv0001KeyboardBackend::findPreset(const QString& name, core::Clv0001PresetInfo& outPreset) {
    const QString norm = name.trimmed().toLower();
    for (const auto& p : verifiedPresets()) {
        if (p.name == norm) {
            outPreset = p;
            return true;
        }
    }
    return false;
}

uint8_t Clv0001KeyboardBackend::brightnessScaleForLevel(int level) {
    static constexpr std::array<uint8_t, 5> kScales = { 0x00U, 0x2FU, 0x5FU, 0x8FU, 0xBFU };
    if (level < 0 || level > 4) {
        return 0x00U;
    }
    return kScales[static_cast<size_t>(level)];
}

int Clv0001KeyboardBackend::brightnessPctToLevel(int pct) {
    const int clamped = std::clamp(pct, 0, 100);
    if (clamped == 0) {
        return 0;
    }
    if (clamped <= 25) {
        return 1;
    }
    if (clamped <= 50) {
        return 2;
    }
    if (clamped <= 75) {
        return 3;
    }
    return 4;
}

int Clv0001KeyboardBackend::brightnessLevelToPct(int level) {
    const int clamped = std::clamp(level, 0, 4);
    return clamped * 25;
}

bool Clv0001KeyboardBackend::buildFirmwarePayload(
    const QString& presetName,
    int brightnessLevel,
    std::array<uint8_t, 4>& outPayload,
    QString* outError
) {
    outPayload = { 0x00U, 0x00U, 0x00U, kCommandByte };

    core::Clv0001PresetInfo preset;
    if (!findPreset(presetName, preset)) {
        if (outError) {
            *outError = QStringLiteral("Unsupported CLV0001 preset '%1' (must be one of the 17 allowlisted presets).").arg(presetName);
        }
        return false;
    }

    if (brightnessLevel < 0 || brightnessLevel > 4) {
        if (outError) {
            *outError = QStringLiteral("CLV0001 discrete brightness level %1 is out of range [0..4].").arg(brightnessLevel);
        }
        return false;
    }

    const uint8_t scale = (preset.name == QLatin1String("off")) ? 0x00U : brightnessScaleForLevel(brightnessLevel);
    outPayload[0] = static_cast<uint8_t>((static_cast<uint16_t>(preset.g) * scale) / 0xBFU);
    outPayload[1] = static_cast<uint8_t>((static_cast<uint16_t>(preset.r) * scale) / 0xBFU);
    outPayload[2] = static_cast<uint8_t>((static_cast<uint16_t>(preset.b) * scale) / 0xBFU);
    outPayload[3] = kCommandByte;
    return true;
}

QString Clv0001KeyboardBackend::formatPayloadHex(const std::array<uint8_t, 4>& payload) {
    return QStringLiteral("%1 %2 %3 %4")
        .arg(payload[0], 2, 16, QLatin1Char('0'))
        .arg(payload[1], 2, 16, QLatin1Char('0'))
        .arg(payload[2], 2, 16, QLatin1Char('0'))
        .arg(payload[3], 2, 16, QLatin1Char('0'))
        .toUpper();
}

QString Clv0001KeyboardBackend::findNearestPresetName(const core::RgbColor& color, bool* outExactMatch) {
    if (outExactMatch) {
        *outExactMatch = false;
    }
    const core::RgbColor clamped = core::ColorScience::clampRgb(color.r, color.g, color.b);

    // Check exact match first (including off for pure black)
    for (const auto& p : verifiedPresets()) {
        if (clamped.r == static_cast<int>(p.r) &&
            clamped.g == static_cast<int>(p.g) &&
            clamped.b == static_cast<int>(p.b)) {
            if (outExactMatch) {
                *outExactMatch = true;
            }
            return p.name;
        }
    }

    // Nearest non-off preset by Euclidean sRGB distance
    QString bestName = QStringLiteral("cyan");
    int bestDistSq = std::numeric_limits<int>::max();
    for (const auto& p : verifiedPresets()) {
        if (p.name == QLatin1String("off")) {
            continue;
        }
        const int dr = clamped.r - static_cast<int>(p.r);
        const int dg = clamped.g - static_cast<int>(p.g);
        const int db = clamped.b - static_cast<int>(p.b);
        const int distSq = dr * dr + dg * dg + db * db;
        if (distSq < bestDistSq) {
            bestDistSq = distSq;
            bestName = p.name;
        }
    }
    return bestName;
}

bool Clv0001KeyboardBackend::simulateLegacyAlgWriteParser(
    const QString& rawWriteBuffer,
    QString* outParsedColor,
    int* outParsedBrightness,
    QString* outFailureReason
) {
    // Reproduces upstream/fork kernel/alg_rgb.c `alg_write()` exactly:
    //   strim(kbuf); sscanf(kbuf, "%31s %d", color_name, &brightness);
    //   for (i = 0; i < ARRAY_SIZE(colors); i++) if (!strcmp(color_name, colors[i].name)) ...
    const QString trimmed = rawWriteBuffer.trimmed();
    const QStringList parts = trimmed.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (parts.isEmpty()) {
        if (outFailureReason) {
            *outFailureReason = QStringLiteral("-EINVAL: empty command buffer");
        }
        return false;
    }

    const QString colorToken = parts.at(0).left(31);
    int parsedBrightness = 4;
    if (parts.size() >= 2) {
        bool okInt = false;
        const int val = parts.at(1).toInt(&okInt);
        if (okInt) {
            parsedBrightness = val;
        }
    }

    if (outParsedColor) {
        *outParsedColor = colorToken;
    }
    if (outParsedBrightness) {
        *outParsedBrightness = parsedBrightness;
    }

    core::Clv0001PresetInfo preset;
    if (!findPreset(colorToken, preset)) {
        if (outFailureReason) {
            *outFailureReason = QStringLiteral("-EINVAL: unknown color '%1' (brightness token parsed as %2)")
                                    .arg(colorToken)
                                    .arg(parsedBrightness);
        }
        return false;
    }
    return true;
}

bool Clv0001KeyboardBackend::isDmiEligible(const core::DmiIdentity& dmi, QString* outReason) {
    const QString vendor = dmi.sysVendor.trimmed();
    const QString product = dmi.productName.trimmed();
    const QString board = dmi.boardName.trimmed();
    const QString boardVendor = dmi.boardVendor.trimmed();

    const bool exactTarget =
        (vendor == QLatin1String("Acer") &&
         product == QLatin1String("Aspire A715-59G") &&
         board == QLatin1String("RTH") &&
         boardVendor == QLatin1String("Kelilu_RTH"));

    if (!exactTarget) {
        if (outReason) {
            *outReason = QStringLiteral(
                "DMI identity ('%1' / '%2' / '%3' / '%4') does not match verified Acer Aspire A715-59G (RTH / Kelilu_RTH)."
            ).arg(vendor, product, board, boardVendor);
        }
        return false;
    }

    return true;
}

Clv0001Topology Clv0001KeyboardBackend::probeTopology(const core::DmiIdentity& dmi) {
    Clv0001Topology topo;
    const auto& root = SysfsRoot::instance();

    const QString devDir = root.resolvePath(QStringLiteral("/sys/bus/acpi/devices/CLV0001:00"));
    if (!QFileInfo::exists(devDir)) {
        return topo;
    }

    topo.sysfsDeviceDir = devDir;
    topo.hid = SysfsRoot::readFileTrimmed(devDir + QStringLiteral("/hid"));
    topo.acpiPath = SysfsRoot::readFileTrimmed(devDir + QStringLiteral("/path"));
    topo.status = SysfsRoot::readFileTrimmed(devDir + QStringLiteral("/status"));

    const bool validHid = (topo.hid == QLatin1String(kAcpiHid));
    const bool validPath = (topo.acpiPath == QLatin1String("\\_SB.DCHU") ||
                            topo.acpiPath == QLatin1String("\\_SB_.DCHU"));
    bool validAcpiSta = true;
    if (!topo.status.isEmpty()) {
        bool okSta = false;
        const int staVal = topo.status.toInt(&okSta);
        // ACPI _STA bit 0 = Present (0x01), bit 1 = Enabled (0x02)
        validAcpiSta = okSta && ((staVal & 0x03) == 0x03);
    }
    topo.devicePresent = validHid && validPath && validAcpiSta;
    topo.dmiEligible = isDmiEligible(dmi);

    topo.commandPath = devDir + QStringLiteral("/clv0001_kbd_command");
    topo.statePath = devDir + QStringLiteral("/clv0001_kbd_state");
    topo.presetsPath = devDir + QStringLiteral("/clv0001_kbd_presets");

    if (topo.devicePresent &&
        QFileInfo(topo.commandPath).isFile() &&
        QFileInfo(topo.statePath).isFile() &&
        QFileInfo(topo.presetsPath).isFile()) {
        topo.driverBound = true;
        const QString stateText = SysfsRoot::readFileTrimmed(topo.statePath);
        const QMap<QString, QString> kv = parseKeyValueState(stateText);
        topo.lastAppliedStatus = kv.value(QStringLiteral("status"), QStringLiteral("uninitialized"));
        topo.lastAppliedPreset = kv.value(QStringLiteral("preset"), QStringLiteral("none"));
        bool okBri = false;
        const int briLvl = kv.value(QStringLiteral("brightness_level")).toInt(&okBri);
        topo.lastAppliedBrightnessLevel = okBri ? std::clamp(briLvl, 0, 4) : 0;
        topo.lastPayloadHex = kv.value(QStringLiteral("payload_grb"), QStringLiteral("00 00 00 F0"));
        topo.lastAcpiReturnType = kv.value(QStringLiteral("acpi_return_type"), QStringLiteral("none"));
    }

    return topo;
}

void Clv0001KeyboardBackend::setMinCommandIntervalMsForTesting(int intervalMs) {
    QMutexLocker locker(&clvBackendMutex());
    g_minIntervalMs = std::max(0, intervalMs);
}

void Clv0001KeyboardBackend::resetRateLimiterForTesting() {
    QMutexLocker locker(&clvBackendMutex());
    g_lastCommandMs = -1;
}

core::ControlResult Clv0001KeyboardBackend::applyPresetCommand(
    const core::DmiIdentity& dmi,
    const QString& presetName,
    int brightnessLevel
) {
    QMutexLocker locker(&clvBackendMutex());
    core::ControlResult res;
    res.physicalStateVerified = false;

    // 1. Validate DMI eligibility
    QString dmiErr;
    if (!isDmiEligible(dmi, &dmiErr)) {
        res.applied = false;
        res.errorCode = QStringLiteral("ERR_UNSUPPORTED_MODEL");
        res.verificationStatusId = QStringLiteral("rejected_before_execution");
        res.message = dmiErr;
        return res;
    }

    // 2. Validate preset and discrete brightness level before touching sysfs/ACPI
    const QString normPreset = presetName.trimmed().toLower();
    const int effectiveLevel = (normPreset == QLatin1String("off")) ? 0 : brightnessLevel;
    std::array<uint8_t, 4> expectedPayload{};
    QString encErr;
    if (!buildFirmwarePayload(normPreset, effectiveLevel, expectedPayload, &encErr)) {
        res.applied = false;
        res.errorCode = (effectiveLevel < 0 || effectiveLevel > 4)
                            ? QStringLiteral("ERR_INVALID_BRIGHTNESS")
                            : QStringLiteral("ERR_INVALID_CLV0001_PRESET");
        res.verificationStatusId = QStringLiteral("rejected_before_execution");
        res.message = encErr;
        return res;
    }

    // 3. Probe CLV0001 ACPI device and kernel driver binding
    const Clv0001Topology topo = probeTopology(dmi);
    if (!topo.devicePresent) {
        res.applied = false;
        res.errorCode = QStringLiteral("ERR_CLV0001_NOT_DETECTED");
        res.verificationStatusId = QStringLiteral("rejected_before_execution");
        res.message = QStringLiteral("ACPI device CLV0001 (\\_SB_.DCHU) was not detected.");
        return res;
    }
    if (!topo.driverBound) {
        res.applied = false;
        res.errorCode = QStringLiteral("ERR_CLV0001_DRIVER_NOT_BOUND");
        res.verificationStatusId = QStringLiteral("rejected_before_execution");
        res.message = QStringLiteral(
            "ACPI device CLV0001:00 (\\_SB_.DCHU) is present, but kernel driver `acer_clv0001_kbd` is not bound. "
            "Profile saved in Keyboard Studio Preview."
        );
        return res;
    }

    // 4. Rate limit firmware _DSM calls
    const qint64 nowMs = rateTimer().elapsed();
    if (g_minIntervalMs > 0 && g_lastCommandMs >= 0 && (nowMs - g_lastCommandMs) < g_minIntervalMs) {
        res.applied = false;
        res.errorCode = QStringLiteral("ERR_RATE_LIMITED");
        res.verificationStatusId = QStringLiteral("rejected_before_execution");
        res.message = QStringLiteral(
            "CLV0001 ACPI _DSM command rate-limited (minimum %1 ms between firmware updates)."
        ).arg(g_minIntervalMs);
        return res;
    }

    // 5. Write strictly allowlisted preset + discrete brightness command and read driver state
    const QString cmdStr = QStringLiteral("%1 %2").arg(normPreset).arg(effectiveLevel);
    QString stateOutput, writeErr;
    const bool writeOk = SysfsRoot::writeCommandAndReadState(
        topo.commandPath,
        topo.statePath,
        cmdStr,
        &stateOutput,
        &writeErr
    );
    g_lastCommandMs = rateTimer().elapsed();

    const QMap<QString, QString> kv = parseKeyValueState(stateOutput);
    const QString drvStatus = kv.value(QStringLiteral("status"));
    const QString acpiRetType = kv.value(QStringLiteral("acpi_return_type"), QStringLiteral("none"));
    const QString payloadHex = formatPayloadHex(expectedPayload);

    if (drvStatus == QLatin1String("error:acpi_eval_failed")) {
        res.applied = false;
        res.physicalStateVerified = false;
        res.errorCode = QStringLiteral("ERR_ACPI_DSM_EVAL_FAILED");
        res.verificationStatusId = QStringLiteral("acpi_eval_failed");
        res.message = QStringLiteral(
            "CLV0001 _DSM 0x67 evaluation failed (ACPI returned NULL) for '%1'."
        ).arg(cmdStr);
        core::DiagnosticRedactor::instance().appendLog(QStringLiteral("ERROR"), QStringLiteral("HAL"), res.message);
        return res;
    }

    if (drvStatus == QLatin1String("error:unexpected_return_type") ||
        acpiRetType == QLatin1String("unexpected")) {
        res.applied = false;
        res.physicalStateVerified = false;
        res.errorCode = QStringLiteral("ERR_ACPI_UNEXPECTED_RETURN_TYPE");
        res.verificationStatusId = QStringLiteral("unexpected_acpi_return_type");
        res.message = QStringLiteral(
            "CLV0001 _DSM 0x67 returned an unexpected ACPI object type for '%1'."
        ).arg(cmdStr);
        core::DiagnosticRedactor::instance().appendLog(QStringLiteral("WARN"), QStringLiteral("HAL"), res.message);
        return res;
    }

    if (drvStatus == QLatin1String("error:rate_limited")) {
        res.applied = false;
        res.physicalStateVerified = false;
        res.errorCode = QStringLiteral("ERR_RATE_LIMITED");
        res.verificationStatusId = QStringLiteral("rejected_before_execution");
        res.message = QStringLiteral("CLV0001 kernel driver rate-limited command '%1' (-EBUSY).").arg(cmdStr);
        return res;
    }

    if (!writeOk) {
        res.applied = false;
        res.physicalStateVerified = false;
        res.errorCode = QStringLiteral("ERR_CLV0001_WRITE_FAILED");
        res.verificationStatusId = QStringLiteral("rejected_before_execution");
        res.message = writeErr.isEmpty() ? QStringLiteral("Failed writing clv0001_kbd_command.") : writeErr;
        return res;
    }

    const QString rbPreset = kv.value(QStringLiteral("preset"));
    const QString rbLevel = kv.value(QStringLiteral("brightness_level"));
    const QString rbPayload = kv.value(QStringLiteral("payload_grb"));
    if (drvStatus != QLatin1String("accepted_no_hw_readback") ||
        rbPreset != normPreset ||
        rbLevel != QString::number(effectiveLevel) ||
        rbPayload != payloadHex) {
        res.applied = false;
        res.physicalStateVerified = false;
        res.errorCode = QStringLiteral("ERR_CLV0001_STATE_MISMATCH");
        res.verificationStatusId = QStringLiteral("rejected_before_execution");
        res.message = QStringLiteral(
            "CLV0001 driver state mismatch after command '%1' (status='%2', preset='%3', level='%4', payload='%5')."
        ).arg(cmdStr, drvStatus, rbPreset, rbLevel, rbPayload);
        return res;
    }

    QJsonObject rbObj;
    rbObj.insert(QStringLiteral("backend"), QStringLiteral("clv0001_acpi_dsm"));
    rbObj.insert(QStringLiteral("acpi_hid"), QString::fromLatin1(kAcpiHid));
    rbObj.insert(QStringLiteral("acpi_path"), topo.acpiPath);
    rbObj.insert(QStringLiteral("dsm_uuid"), QString::fromLatin1(kDsmUuid));
    rbObj.insert(QStringLiteral("dsm_function"), QStringLiteral("0x67"));
    rbObj.insert(QStringLiteral("preset"), normPreset);
    rbObj.insert(QStringLiteral("brightness_level"), effectiveLevel);
    rbObj.insert(QStringLiteral("payload_grb"), payloadHex);
    rbObj.insert(QStringLiteral("acpi_return_type"), acpiRetType);
    rbObj.insert(QStringLiteral("physical_readback"), false);
    rbObj.insert(QStringLiteral("verification_status"), QStringLiteral("accepted_no_hw_readback"));

    res.applied = true;
    res.physicalStateVerified = false;
    res.errorCode = QStringLiteral("OK");
    res.verificationStatusId = QStringLiteral("accepted_no_hw_readback");
    res.readbackSummary = QString::fromUtf8(QJsonDocument(rbObj).toJson(QJsonDocument::Compact));
    res.message = QStringLiteral(
        "Accepted by CLV0001 _DSM 0x67 (preset=%1, level=%2/4, GRB=%3; physical hardware readback not exposed by firmware)."
    ).arg(normPreset).arg(effectiveLevel).arg(payloadHex);
    core::DiagnosticRedactor::instance().appendLog(QStringLiteral("INFO"), QStringLiteral("HAL"), res.message);
    return res;
}

} // namespace acc::hal
