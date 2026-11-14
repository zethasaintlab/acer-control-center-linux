// SPDX-License-Identifier: GPL-3.0-or-later WITH GPL-3.0-linking-source-exception
#pragma once

#include "core/ColorScience.h"
#include "core/DomainModels.h"
#include <QString>
#include <cstdint>
#include <vector>

namespace acc::hal {

struct ClevoWmiTopology {
    bool eventGuid0Present = false; // ABBC0F6B-8EA1-11D1-00A0-C90629100000 (notify_id D0)
    bool eventGuid1Present = false; // ABBC0F6C-8EA1-11D1-00A0-C90629100000 (notify_id D1)
    bool methodGuidBbPresent = false; // ABBC0F6D-8EA1-11D1-00A0-C90629100000 (object_id BB / \_SB_.WMI_.WMBB)
    bool isMethodDevtype = false;
    QString objectId;
    QString methodDeviceSysfsPath;
    bool clevoAcpiDchuPresent = false; // CLV0001:00 (\_SB_.DCHU)
    bool clevoAcpiDchpPresent = false; // CLV0002:00 (\_SB_.DCHP)
    bool ecFirmwarePresent = false;    // PNP0C09:00 (\_SB_.PC00.LPCB.EC__)
    bool i8042Ps2KeyboardPresent = false; // MSFT0001:00 (\_SB_.PC00.LPCB.PS2K)
    bool dmiModelAndBiosVerified = false;
    bool verifiedBridgeReady = false;
};

/**
 * Isolated, strictly typed Clevo WMI keyboard backend (Backend B).
 *
 * Implements the Clevo WMBB (GUID ABBC0F6D-8EA1-11D1-00A0-C90629100000, object_id "BB",
 * method_id 0x67 CLEVO_CMD_SET_KB_RGB_LEDS) 32-bit packet encoder and hardware gate.
 *
 * SAFETY INVARIANTS:
 * 1. Never executes arbitrary WMI method IDs, raw EC register writes, or /proc/acpi/call strings.
 * 2. Requires exact DMI model (Acer Aspire A715-59G), board (RTH / Kelilu_RTH), and BIOS (V1.17).
 * 3. Disabled by default on physical hardware when no kernel driver exposes a verified bridge,
 *    preventing speculative ACPI/EC writes while allowing deterministic mock/fixture verification.
 */
class ClevoKeyboardBackend {
public:
    static constexpr const char* kWmiEventGuid0 = "ABBC0F6B-8EA1-11D1-00A0-C90629100000";
    static constexpr const char* kWmiEventGuid1 = "ABBC0F6C-8EA1-11D1-00A0-C90629100000";
    static constexpr const char* kWmiMethodGuidBb = "ABBC0F6D-8EA1-11D1-00A0-C90629100000";

    static constexpr const char* kExpectedSysVendor = "Acer";
    static constexpr const char* kExpectedProductName = "Aspire A715-59G";
    static constexpr const char* kExpectedBoardName = "RTH";
    static constexpr const char* kExpectedBoardVendor = "Kelilu_RTH";
    static constexpr const char* kExpectedBiosVersion = "V1.17";

    // Clevo WMBB method IDs (from tuxedo-drivers / clevo-xsm-wmi / Lekensteyn clevo-wmi)
    static constexpr uint32_t kWmbbMethodEnableEvents = 0x46U;
    static constexpr uint32_t kWmbbMethodSetKbRgbLeds = 0x67U;

    // Pure, deterministic 32-bit WMBB packet encoders (method_id = 0x67)
    static bool encodeZoneColorPacket(
        int zoneIndex,
        int r,
        int g,
        int b,
        uint32_t& outPacket,
        QString* outError = nullptr
    );

    static bool encodeBrightnessPacket(
        int brightnessPct,
        uint32_t& outPacket,
        QString* outError = nullptr
    );

    static bool encodeModePacket(
        const QString& mode,
        bool enabled,
        uint32_t& outPacket,
        QString* outError = nullptr
    );

    static QString formatPacketHex(uint32_t packet);

    // Hardware & firmware identity gates
    static bool verifyTargetModelAndBios(const core::DmiIdentity& dmi, QString* outReason = nullptr);
    static ClevoWmiTopology probeTopology(const core::DmiIdentity& dmi);

    // Guarded execution over a verified WMBB bridge fixture
    static core::ControlResult applyViaWmiBridge(
        const core::DmiIdentity& dmi,
        const QString& mode,
        int brightnessPct,
        const std::vector<core::RgbColor>& zones
    );
};

} // namespace acc::hal
