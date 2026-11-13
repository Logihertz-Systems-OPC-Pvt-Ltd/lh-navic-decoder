// Copyright 2026 Logihertz Systems (OPC) Private Limited
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef NAVIC_TYPES_HPP
#define NAVIC_TYPES_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace logihertz {
namespace navic {

/// Speed of light in vacuum (m/s) per BIPM / CODATA.
inline constexpr double SPEED_OF_LIGHT_M_S = 299792458.0;

/// NavIC SPS Carrier Frequencies (Hz).
inline constexpr double FREQ_NAVIC_L5_HZ = 1176450000.0;
inline constexpr double FREQ_NAVIC_S_BAND_HZ = 2492028000.0;

/// NavIC SPS Gold code length (chips) and chipping rate (chips/s).
inline constexpr size_t NAVIC_GOLD_CODE_LENGTH = 1023;
inline constexpr double NAVIC_CHIPPING_RATE_HZ = 1023000.0;

/// NavIC 292-bit Navigation Subframe parameters.
inline constexpr size_t NAVIC_SUBFRAME_TOTAL_BITS = 292;
inline constexpr size_t NAVIC_SUBFRAME_BODY_BITS = 262;
inline constexpr size_t NAVIC_CRC24Q_BITS = 24;
inline constexpr size_t NAVIC_TAIL_BITS = 6;

/// Preamble sync words.
inline constexpr uint16_t NAVIC_PREAMBLE_SYNC = 0xEB90;
inline constexpr uint16_t NAVIC_PREAMBLE_INVERTED_SYNC = 0x146F;

/// Qualcomm CRC-24Q Generator Polynomial (0x1864CFB).
inline constexpr uint32_t CRC24Q_POLYNOMIAL = 0x1864CFB;

/// Maximum physical ionospheric delay on L5 under nominal space weather (meters).
inline constexpr double MAX_PHYSICAL_IONO_DELAY_M = 65.0;

/// Generated PRN sequence container.
struct PrnSequence {
    int prn_id{0};
    std::vector<uint8_t> chips;            // 0 or 1
    std::vector<int8_t> bipolar_chips;     // +1 (for 0) or -1 (for 1)
    size_t ones_count{0};
    size_t zeros_count{0};
};

/// Early-Prompt-Late (EPL) correlator output and discriminators.
struct CorrelatorOutput {
    double early{0.0};
    double prompt{0.0};
    double late{0.0};
    double dll_discriminator{0.0};  // (E - L) / (E + L)
    double pll_discriminator{0.0};  // atan2(Q_P, I_P)
    double sqm_asymmetry{0.0};      // |E - L| / (P + eps)
    bool sqm_anomaly{false};        // true if sqm_asymmetry > threshold
};

/// Decoded 292-bit Subframe Header.
struct SubframeHeader {
    uint16_t sync_word{0};
    uint8_t tlmh{0};
    uint32_t towc{0};               // Time of Week Count (17 bits: 0..50399)
    bool alert{false};              // 1 = Alert (do not use)
    bool autonav{false};            // 1 = Autonomous navigation mode
    uint8_t subframe_id{0};         // 0: SF1, 1: SF2, 2: SF3, 3: SF4
    bool spare{false};
};

/// Full decoded subframe representation.
struct DecodedSubframe {
    SubframeHeader header;
    std::vector<uint8_t> payload_bits; // 216 bits
    uint32_t crc24q_received{0};
    uint32_t crc24q_calculated{0};
    bool crc_valid{false};
    bool is_inverted{false};
    bool is_valid{false};
    std::string error_message;
};

/// Dual-band L5/S ionospheric delay and iono-free pseudorange.
struct DualBandIonoResult {
    double rho_l5_m{0.0};
    double rho_s_m{0.0};
    double delta_rho_iono_l5_m{0.0};
    double rho_if_m{0.0};
    bool is_spoofed_anomaly{false};
    std::string status;
};

/// Individual posture rule finding.
struct PostureRuleFinding {
    std::string rule_id;
    std::string title;
    std::string severity; // CRITICAL, HIGH, MEDIUM, LOW
    int deduction{0};
    std::string cwe;
    bool passed{false};
    std::string message;
    std::string remediation;
};

/// Overall security posture assessment report.
struct PostureReport {
    int score{100};
    int passed_count{10};
    int failed_count{0};
    int total_deductions{0};
    std::vector<PostureRuleFinding> findings;
    std::string verdict;
};

/// NavIC receiver security configuration manifest.
struct NavicReceiverPolicy {
    bool dual_band_l5_s_enabled{true};
    bool verify_crc24q_integrity{true};
    bool strict_sync_word_check{true};
    bool honor_satellite_alert_flag{true};
    int max_allowed_ura_index{7};
    bool epl_symmetry_sqm_enabled{true};
    double max_epl_asymmetry_ratio{0.20};
    bool navic_nma_verification{true};
    bool towc_monotonicity_check{true};
    bool wn_rollover_guard_enabled{true};
    bool verify_gso_geo_subsatellite_bounds{true};
    double dll_bandwidth_hz{1.0};
    double pll_bandwidth_hz{12.0};
    std::string telemetry_bind_address{"127.0.0.1"};
    bool require_client_auth{true};
    int autonav_max_hours{72};
    bool log_baseband_anomalies{true};
};

} // namespace navic
} // namespace logihertz

#endif // NAVIC_TYPES_HPP
