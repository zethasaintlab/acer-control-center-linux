// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#include "hal/ClevoKeyboardBackend.h"
#include "core/DiagnosticRedactor.h"
#include "hal/SysfsRoot.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <algorithm>
#include <cmath>

namespace acc::hal {

bool ClevoKeyboardBackend::encodeZoneColorPacket(
    int zoneIndex,
    int r,
    int g,
    int b,
    uint32_t& outPacket,
    QString* outError
) {
    outPacket = 0U;
    if (zoneIndex < 0 || zoneIndex > 2) {
        if (outError) {
            *outError = QStringLiteral("Clevo WMBB zoneIndex %1 is out of hardware range [0..2].").arg(zoneIndex);
        }
        return false;
    }
    if (r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
        if (outError) {
            *outError = QStringLiteral("RGB channel values (%1, %2, %3) are out of range [0..255].").arg(r).arg(g).arg(b);
        }
        return false;
    }

    // Clevo WMBB 0x67 32-bit zone color sub-command:
    // High byte: 0xF0 (Zone 0/Left), 0xF1 (Zone 1/Center), 0xF2 (Zone 2/Right)
    // Lower 24 bits: BRG byte ordering ((B << 16) | (R << 8) | G) per tuxedo-drivers clevo_keyboard.c
    const uint32_t cmdByte = 0xF0U + static_cast<uint32_t>(zoneIndex);
    outPacket = (cmdByte << 24U) |
                ((static_cast<uint32_t>(b) & 0xFFU) << 16U) |
                ((static_cast<uint32_t>(r) & 0xFFU) << 8U) |
                (static_cast<uint32_t>(g) & 0xFFU);
    return true;
}

bool ClevoKeyboardBackend::encodeBrightnessPacket(
    int brightnessPct,
    uint32_t& outPacket,
    QString* outError
) {
    outPacket = 0U;
    if (brightnessPct < 0 || brightnessPct > 100) {
        if (outError) {
            *outError = QStringLiteral("Keyboard brightness %1% is out of range [0..100].").arg(brightnessPct);
        }
        return false;
    }

    const int rawByte = std::clamp(static_cast<int>(std::lround((brightnessPct * 255.0) / 100.0)), 0, 255);
    outPacket = 0xF4000000U | (static_cast<uint32_t>(rawByte) & 0xFFU);
    return true;
}

bool ClevoKeyboardBackend::encodeModePacket(
    const QString& mode,
    bool enabled,
    uint32_t& outPacket,
    QString* outError
) {
    outPacket = 0U;
    const QString norm = mode.trimmed().toLower();
    if (!enabled || norm == QLatin1String("off")) {
        outPacket = 0x00000000U;
        return true;
    }
    if (norm == QLatin1String("static")) {
        outPacket = 0x10000000U;
        return true;
    }
    if (norm == QLatin1String("breathing") || norm == QLatin1String("breathe")) {
        outPacket = 0x1002A000U;
        return true;
    }
    if (norm == QLatin1String("wave") || norm == QLatin1String("cycle")) {
        outPacket = 0x33010000U;
        return true;
    }

    if (outError) {
        *outError = QStringLiteral("Keyboard lighting mode '%1' is not allowlisted for Clevo WMBB.").arg(mode);
    }
    return false;
}

QString ClevoKeyboardBackend::formatPacketHex(uint32_t packet) {
    return QStringLiteral("0x") + QStringLiteral("%1").arg(packet, 8, 16, QLatin1Char('0')).toUpper();
}

bool ClevoKeyboardBackend::verifyTargetModelAndBios(const core::DmiIdentity& dmi, QString* outReason) {
    if (dmi.sysVendor.trimmed() != QLatin1String(kExpectedSysVendor) ||
        dmi.productName.trimmed() != QLatin1String(kExpectedProductName) ||
        dmi.boardName.trimmed() != QLatin1String(kExpectedBoardName) ||
        dmi.boardVendor.trimmed() != QLatin1String(kExpectedBoardVendor)) {
        if (outReason) {
            *outReason = QStringLiteral(
                "DMI platform mismatch (got vendor='%1', product='%2', board='%3/%4'; "
                "expected 'Acer Aspire A715-59G' on 'RTH/Kelilu_RTH')."
            ).arg(dmi.sysVendor, dmi.productName, dmi.boardName, dmi.boardVendor);
        }
        return false;
    }

    if (dmi.biosVersion.trimmed() != QLatin1String(kExpectedBiosVersion)) {
        if (outReason) {
            *outReason = QStringLiteral(
                "BIOS revision '%1' is not verified for Clevo WMBB keyboard writes (expected '%2')."
            ).arg(dmi.biosVersion, QLatin1String(kExpectedBiosVersion));
        }
        return false;
    }
    return true;
}

ClevoWmiTopology ClevoKeyboardBackend::probeTopology(const core::DmiIdentity& dmi) {
    ClevoWmiTopology topo;
    const auto& root = SysfsRoot::instance();

    const QString wmiBase = root.resolvePath(QStringLiteral("/sys/bus/wmi/devices"));
    QDir wmiDir(wmiBase);
    if (wmiDir.exists()) {
        const QStringList entries = wmiDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString& entry : entries) {
            if (entry.startsWith(QLatin1String(kWmiEventGuid0), Qt::CaseInsensitive)) {
                topo.eventGuid0Present = true;
            } else if (entry.startsWith(QLatin1String(kWmiEventGuid1), Qt::CaseInsensitive)) {
                topo.eventGuid1Present = true;
            } else if (entry.startsWith(QLatin1String(kWmiMethodGuidBb), Qt::CaseInsensitive)) {
                topo.methodGuidBbPresent = true;
                topo.methodDeviceSysfsPath = wmiDir.filePath(entry);
                topo.objectId = SysfsRoot::readFileTrimmed(topo.methodDeviceSysfsPath + QStringLiteral("/object_id"));
                const QString uevent = SysfsRoot::readFileTrimmed(topo.methodDeviceSysfsPath + QStringLiteral("/uevent"));
                topo.isMethodDevtype = uevent.contains(QLatin1String("DEVTYPE=method"));
            }
        }
    }

    topo.clevoAcpiDchuPresent = QFileInfo::exists(root.resolvePath(QStringLiteral("/sys/bus/acpi/devices/CLV0001:00")));
    topo.clevoAcpiDchpPresent = QFileInfo::exists(root.resolvePath(QStringLiteral("/sys/bus/acpi/devices/CLV0002:00")));
    topo.ecFirmwarePresent = QFileInfo::exists(root.resolvePath(QStringLiteral("/sys/bus/acpi/devices/PNP0C09:00")));
    topo.i8042Ps2KeyboardPresent = QFileInfo::exists(root.resolvePath(QStringLiteral("/sys/bus/acpi/devices/MSFT0001:00")));
    topo.dmiModelAndBiosVerified = verifyTargetModelAndBios(dmi);

    // SAFETY INVARIANT: Physical hardware never executes speculative WMBB commands without an in-tree
    // kernel LED driver. The direct WMBB bridge mailbox is enabled only in verified mock/fixture environments
    // when DMI, BIOS V1.17, WMI GUID ABBC0F6D, object_id==BB, DEVTYPE=method, and wmbb_verified_bridge all match.
    if (root.isMockActive() &&
        topo.dmiModelAndBiosVerified &&
        topo.methodGuidBbPresent &&
        topo.objectId == QLatin1String("BB") &&
        topo.isMethodDevtype &&
        !topo.methodDeviceSysfsPath.isEmpty()) {
        const QString bridgeFile = topo.methodDeviceSysfsPath + QStringLiteral("/wmbb_verified_bridge");
        topo.verifiedBridgeReady = QFileInfo::exists(bridgeFile);
    }

    return topo;
}

core::ControlResult ClevoKeyboardBackend::applyViaWmiBridge(
    const core::DmiIdentity& dmi,
    const QString& mode,
    int brightnessPct,
    const std::vector<core::RgbColor>& zones
) {
    core::ControlResult res;

    // 1. Validate DMI model, board, and BIOS version first
    QString modelErr;
    if (!verifyTargetModelAndBios(dmi, &modelErr)) {
        res.applied = false;
        res.errorCode = QStringLiteral("ERR_UNSUPPORTED_MODEL_OR_BIOS");
        res.message = modelErr;
        return res;
    }

    // 2. Validate WMI device presence, GUID, object_id == "BB", and DEVTYPE=method
    const ClevoWmiTopology topo = probeTopology(dmi);
    if (!topo.methodGuidBbPresent || topo.objectId != QLatin1String("BB") || !topo.isMethodDevtype) {
        res.applied = false;
        res.errorCode = QStringLiteral("ERR_UNSUPPORTED_WMI_DEVICE");
        res.message = QStringLiteral(
            "Required Clevo WMI method block (%1, object_id=BB, DEVTYPE=method) is not present or valid."
        ).arg(QLatin1String(kWmiMethodGuidBb));
        return res;
    }

    // 3. Validate input mode, brightness, and 3-zone RGB arguments before touching any bridge
    QString encErr;
    uint32_t pktMode = 0U;
    if (!encodeModePacket(mode, brightnessPct > 0, pktMode, &encErr)) {
        res.applied = false;
        res.errorCode = QStringLiteral("ERR_INVALID_KEYBOARD_MODE");
        res.message = encErr;
        return res;
    }

    uint32_t pktBri = 0U;
    if (!encodeBrightnessPacket(brightnessPct, pktBri, &encErr)) {
        res.applied = false;
        res.errorCode = QStringLiteral("ERR_INVALID_BRIGHTNESS");
        res.message = encErr;
        return res;
    }

    uint32_t pktZones[3] = {0U, 0U, 0U};
    for (int z = 0; z < 3; ++z) {
        const core::RgbColor c = zones.empty()
                                     ? core::RgbColor{59, 158, 255}
                                     : zones.at(std::min(static_cast<size_t>(z), zones.size() - 1));
        if (!encodeZoneColorPacket(z, c.r, c.g, c.b, pktZones[z], &encErr)) {
            res.applied = false;
            res.errorCode = QStringLiteral("ERR_INVALID_COLOR_HEX");
            res.message = encErr;
            return res;
        }
    }

    // 4. Enforce hardware write safety gate: refuse physical speculative execution when unbridged
    if (!topo.verifiedBridgeReady) {
        res.applied = false;
        res.errorCode = QStringLiteral("ERR_KEYBOARD_LED_NOT_DETECTED");
        res.message = QStringLiteral(
            "Clevo WMI method block (%1, object_id=BB) is unbound by the kernel and direct WMBB execution is "
            "disabled for hardware safety. Profile saved to Keyboard Studio Preview."
        ).arg(QLatin1String(kWmiMethodGuidBb));
        return res;
    }

    // 5. Check simulated EC/WMI status register before & after write (fail closed on error status)
    const QString statusPath = topo.methodDeviceSysfsPath + QStringLiteral("/wmbb_status");
    if (QFileInfo::exists(statusPath)) {
        const QString statusStr = SysfsRoot::readFileTrimmed(statusPath);
        if (!statusStr.isEmpty() && statusStr != QLatin1String("0x00000000") && statusStr != QLatin1String("0")) {
            res.applied = false;
            res.errorCode = QStringLiteral("ERR_WMI_EC_RESPONSE_INVALID");
            res.message = QStringLiteral(
                "Clevo WMBB bridge reported non-zero EC status '%1'; aborting keyboard write."
            ).arg(statusStr);
            return res;
        }
    }

    const QString bridgePath = topo.methodDeviceSysfsPath + QStringLiteral("/wmbb_verified_bridge");
    const QString prevBridgePayload = SysfsRoot::readFileTrimmed(bridgePath);

    const QString payload = QStringLiteral("method=0x67 z0=%1 z1=%2 z2=%3 bri=%4 mode=%5")
                                .arg(formatPacketHex(pktZones[0]),
                                     formatPacketHex(pktZones[1]),
                                     formatPacketHex(pktZones[2]),
                                     formatPacketHex(pktBri),
                                     formatPacketHex(pktMode));

    QString rbPayload, writeErr;
    if (!SysfsRoot::writeFileVerified(bridgePath, payload, &rbPayload, &writeErr)) {
        if (!prevBridgePayload.isEmpty()) {
            SysfsRoot::writeFileVerified(bridgePath, prevBridgePayload);
        }
        res.applied = false;
        res.errorCode = QStringLiteral("ERR_WMI_BRIDGE_WRITE_FAILED");
        res.message = writeErr;
        return res;
    }

    QJsonObject rbObj;
    rbObj.insert(QStringLiteral("backend"), QStringLiteral("clevo_wmbb_bridge"));
    rbObj.insert(QStringLiteral("wmi_guid"), QString::fromLatin1(kWmiMethodGuidBb));
    rbObj.insert(QStringLiteral("object_id"), topo.objectId);
    rbObj.insert(QStringLiteral("method_id"), QStringLiteral("0x67"));
    QJsonArray pktArr;
    pktArr.append(formatPacketHex(pktZones[0]));
    pktArr.append(formatPacketHex(pktZones[1]));
    pktArr.append(formatPacketHex(pktZones[2]));
    pktArr.append(formatPacketHex(pktBri));
    pktArr.append(formatPacketHex(pktMode));
    rbObj.insert(QStringLiteral("packets"), pktArr);
    rbObj.insert(QStringLiteral("readback"), rbPayload);

    res.applied = true;
    res.errorCode = QStringLiteral("OK");
    res.readbackSummary = QString::fromUtf8(QJsonDocument(rbObj).toJson(QJsonDocument::Compact));
    res.message = QStringLiteral("Verified Clevo WMBB (0x67) keyboard transaction (%1).").arg(rbPayload);
    core::DiagnosticRedactor::instance().appendLog(QStringLiteral("INFO"), QStringLiteral("HAL"), res.message);
    return res;
}

} // namespace acc::hal
