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

#include "telemetry_decoder.hpp"
#include <sstream>
#include <iomanip>
#include <cmath>

namespace logihertz {
namespace navic {

uint32_t TelemetryDecoder::compute_crc24q(const uint8_t *bits, size_t bit_len) {
    uint32_t crc = 0;
    for (size_t i = 0; i < bit_len; ++i) {
        crc = ((crc << 1) | static_cast<uint32_t>(bits[i] & 1)) & 0x1FFFFFF;
        if (crc & 0x1000000) {
            crc ^= CRC24Q_POLYNOMIAL;
        }
    }
    return crc & 0xFFFFFF;
}

uint32_t TelemetryDecoder::compute_crc24q(const std::vector<uint8_t> &bits) {
    return compute_crc24q(bits.data(), bits.size());
}

std::vector<uint8_t> TelemetryDecoder::hex_to_bits(const std::string &hex_str) {
    std::vector<uint8_t> bits;
    size_t start = 0;
    if (hex_str.rfind("0x", 0) == 0 || hex_str.rfind("0X", 0) == 0) {
        start = 2;
    }

    for (size_t i = start; i < hex_str.size(); ++i) {
        char c = hex_str[i];
        if (std::isspace(static_cast<unsigned char>(c))) {
            continue;
        }
        int val = 0;
        if (c >= '0' && c <= '9') {
            val = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            val = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            val = c - 'A' + 10;
        } else {
            continue;
        }

        for (int b = 3; b >= 0; --b) {
            bits.push_back(static_cast<uint8_t>((val >> b) & 1));
        }
    }
    return bits;
}

std::string TelemetryDecoder::bits_to_hex(const std::vector<uint8_t> &bits) {
    std::ostringstream oss;
    size_t n = bits.size();
    for (size_t i = 0; i < n; i += 4) {
        int val = 0;
        for (size_t b = 0; b < 4 && (i + b) < n; ++b) {
            val = (val << 1) | (bits[i + b] & 1);
        }
        if (n - i < 4) {
            val <<= (4 - (n - i));
        }
        oss << std::uppercase << std::hex << val;
    }
    return oss.str();
}

static uint32_t extract_bits(const std::vector<uint8_t> &bits, size_t offset, size_t count) {
    uint32_t val = 0;
    for (size_t i = 0; i < count; ++i) {
        val = (val << 1) | (bits[offset + i] & 1);
    }
    return val;
}

DecodedSubframe TelemetryDecoder::decode_subframe_bits(const std::vector<uint8_t> &raw_bits_292) {
    DecodedSubframe result;
    if (raw_bits_292.size() < NAVIC_SUBFRAME_TOTAL_BITS) {
        result.is_valid = false;
        result.error_message = "Subframe truncated: requires exactly 292 bits";
        return result;
    }

    std::vector<uint8_t> bits = raw_bits_292;
    bits.resize(NAVIC_SUBFRAME_TOTAL_BITS);

    // Read first 16 bits to check synchronization
    uint16_t sync = static_cast<uint16_t>(extract_bits(bits, 0, 16));
    if (sync == NAVIC_PREAMBLE_INVERTED_SYNC) {
        result.is_inverted = true;
        for (size_t i = 0; i < bits.size(); ++i) {
            bits[i] ^= 1;
        }
        sync = static_cast<uint16_t>(extract_bits(bits, 0, 16));
    }

    result.header.sync_word = sync;
    if (sync != NAVIC_PREAMBLE_SYNC) {
        result.is_valid = false;
        result.error_message = "Invalid preamble sync word (expected 0xEB90 or inverted 0x146F)";
        return result;
    }

    // Unpack header fields
    result.header.tlmh = static_cast<uint8_t>(extract_bits(bits, 16, 8));
    result.header.towc = extract_bits(bits, 24, 17);
    result.header.alert = (bits[41] != 0);
    result.header.autonav = (bits[42] != 0);
    result.header.subframe_id = static_cast<uint8_t>(extract_bits(bits, 43, 2));
    result.header.spare = (bits[45] != 0);

    // Extract 216 bits of payload (offset 46 to 261)
    result.payload_bits.assign(bits.begin() + 46, bits.begin() + 262);

    // Extract 24 bits of CRC24Q (offset 262 to 285)
    result.crc24q_received = extract_bits(bits, 262, 24);

    // Compute CRC24Q over first 262 bits (bits 0..261)
    result.crc24q_calculated = compute_crc24q(bits.data(), NAVIC_SUBFRAME_BODY_BITS);
    result.crc_valid = (result.crc24q_received == result.crc24q_calculated);

    if (!result.crc_valid) {
        result.is_valid = false;
        result.error_message = "CRC-24Q checksum mismatch: transmission corrupted or tampered";
        return result;
    }

    if (result.header.alert) {
        result.is_valid = false;
        result.error_message = "Satellite Alert flag asserted: space vehicle unhealthy / do not use";
        return result;
    }

    result.is_valid = true;
    return result;
}

DecodedSubframe TelemetryDecoder::decode_subframe_hex(const std::string &hex_str) {
    std::vector<uint8_t> bits = hex_to_bits(hex_str);
    return decode_subframe_bits(bits);
}

static void append_bits(std::vector<uint8_t> &target, uint32_t val, size_t count) {
    for (int i = static_cast<int>(count) - 1; i >= 0; --i) {
        target.push_back(static_cast<uint8_t>((val >> i) & 1));
    }
}

std::vector<uint8_t> TelemetryDecoder::construct_synthetic_subframe(
    uint8_t tlmh,
    uint32_t towc,
    bool alert,
    bool autonav,
    uint8_t subframe_id,
    const std::vector<uint8_t> &payload_216)
{
    std::vector<uint8_t> bits;
    bits.reserve(NAVIC_SUBFRAME_TOTAL_BITS);

    // 1. Sync word (16 bits)
    append_bits(bits, NAVIC_PREAMBLE_SYNC, 16);

    // 2. TLMH (8 bits)
    append_bits(bits, tlmh, 8);

    // 3. TOWC (17 bits)
    append_bits(bits, towc & 0x1FFFF, 17);

    // 4. Alert (1 bit)
    bits.push_back(alert ? 1 : 0);

    // 5. AutoNav (1 bit)
    bits.push_back(autonav ? 1 : 0);

    // 6. Subframe ID (2 bits)
    append_bits(bits, subframe_id & 0x03, 2);

    // 7. Spare (1 bit)
    bits.push_back(0);

    // 8. Payload (216 bits)
    for (size_t i = 0; i < 216; ++i) {
        if (i < payload_216.size()) {
            bits.push_back(payload_216[i] & 1);
        } else {
            bits.push_back(0);
        }
    }

    // Body is now 262 bits. Compute CRC24Q over body
    uint32_t crc = compute_crc24q(bits.data(), NAVIC_SUBFRAME_BODY_BITS);

    // 9. CRC24Q (24 bits)
    append_bits(bits, crc, 24);

    // 10. Tail (6 bits)
    for (int i = 0; i < 6; ++i) {
        bits.push_back(0);
    }

    return bits;
}

DualBandIonoResult TelemetryDecoder::calculate_ionospheric_correction(double rho_l5, double rho_s) {
    DualBandIonoResult res;
    res.rho_l5_m = rho_l5;
    res.rho_s_m = rho_s;

    double f_l5_sq = FREQ_NAVIC_L5_HZ * FREQ_NAVIC_L5_HZ;
    double f_s_sq = FREQ_NAVIC_S_BAND_HZ * FREQ_NAVIC_S_BAND_HZ;

    double gamma = f_s_sq / f_l5_sq; // ~4.4883556
    double denom = gamma - 1.0;

    // Delta rho_iono,L5 = (gamma / (gamma - 1)) * (rho_L5 - rho_S)
    res.delta_rho_iono_l5_m = (gamma / denom) * (rho_l5 - rho_s);

    // rho_IF = rho_L5 - Delta rho_iono,L5 = (gamma * rho_S - rho_L5) / (gamma - 1)
    res.rho_if_m = (gamma * rho_s - rho_l5) / denom;

    // Physical upper bound verification on ionospheric delay
    if (std::abs(res.delta_rho_iono_l5_m) > MAX_PHYSICAL_IONO_DELAY_M) {
        res.is_spoofed_anomaly = true;
        res.status = "ANOMALY_SINGLE_BAND_SPOOFING (Ionospheric delay exceeds physical threshold of 65.0m)";
    } else {
        res.is_spoofed_anomaly = false;
        res.status = "CONSISTENT_AUTHENTIC (Dual-band ionospheric delay physically consistent)";
    }

    return res;
}

} // namespace navic
} // namespace logihertz
