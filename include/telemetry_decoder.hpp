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

#ifndef TELEMETRY_DECODER_HPP
#define TELEMETRY_DECODER_HPP

#include "navic_types.hpp"

namespace logihertz {
namespace navic {

/// NavIC 292-Bit Navigation Subframe Telemetry Unpacker, CRC-24Q Engine & Dual-Band Iono Math.
class TelemetryDecoder {
public:
    /// Computes Qualcomm CRC-24Q (0x1864CFB) over a sequence of bits.
    static uint32_t compute_crc24q(const uint8_t *bits, size_t bit_len);

    /// Computes Qualcomm CRC-24Q over a bit vector.
    static uint32_t compute_crc24q(const std::vector<uint8_t> &bits);

    /// Converts a hexadecimal string into an unpacked bit vector (MSB first).
    static std::vector<uint8_t> hex_to_bits(const std::string &hex_str);

    /// Converts an unpacked bit vector into a hexadecimal string.
    static std::string bits_to_hex(const std::vector<uint8_t> &bits);

    /// Decodes a 292-bit raw NavIC navigation subframe.
    static DecodedSubframe decode_subframe_bits(const std::vector<uint8_t> &raw_bits_292);

    /// Decodes a 292-bit raw NavIC navigation subframe from a hexadecimal string.
    static DecodedSubframe decode_subframe_hex(const std::string &hex_str);

    /// Constructs an authentic 292-bit subframe bitstream with valid CRC-24Q and tail.
    static std::vector<uint8_t> construct_synthetic_subframe(
        uint8_t tlmh,
        uint32_t towc,
        bool alert,
        bool autonav,
        uint8_t subframe_id,
        const std::vector<uint8_t> &payload_216);

    /// Computes dual-band L5/S ionospheric delay and iono-free pseudorange.
    static DualBandIonoResult calculate_ionospheric_correction(double rho_l5, double rho_s);
};

} // namespace navic
} // namespace logihertz

#endif // TELEMETRY_DECODER_HPP
