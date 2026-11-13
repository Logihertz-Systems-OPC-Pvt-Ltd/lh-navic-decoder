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

#include "prn_correlator.hpp"
#include <cmath>
#include <stdexcept>
#include <cstdlib>

namespace logihertz {
namespace navic {

bool PrnCorrelator::is_valid_prn(int prn_id) {
    return prn_id >= 1 && prn_id <= 14;
}

uint16_t PrnCorrelator::get_g2_initial_state(int prn_id) {
    // Canonical 10-bit G2 initial state vectors from ISRO IRNSS SIS ICD
    static const uint16_t G2_INIT_TABLE[15] = {
        0x000, // Index 0 (unused)
        0x3F6, // PRN 1:  1111110110
        0x3EC, // PRN 2:  1111101100
        0x3D8, // PRN 3:  1111011000
        0x3B0, // PRN 4:  1110110000
        0x360, // PRN 5:  1101100000
        0x2C1, // PRN 6:  1011000001
        0x183, // PRN 7:  0110000011
        0x307, // PRN 8:  1100000111
        0x20F, // PRN 9:  1000001111
        0x01F, // PRN 10: 0000011111
        0x03E, // PRN 11: 0000111110
        0x07D, // PRN 12: 0001111101
        0x0FA, // PRN 13: 0011111010
        0x1F5  // PRN 14: 0111110101
    };

    if (!is_valid_prn(prn_id)) {
        throw std::invalid_argument("Invalid NavIC PRN ID: must be between 1 and 14");
    }
    return G2_INIT_TABLE[prn_id];
}

PrnSequence PrnCorrelator::generate_gold_code(int prn_id) {
    if (!is_valid_prn(prn_id)) {
        throw std::invalid_argument("Invalid NavIC PRN ID: must be between 1 and 14");
    }

    uint16_t g2_init = get_g2_initial_state(prn_id);

    // G1: 10 stages, initial state 0x3FF (all ones)
    // G1 polynomial: 1 + X^3 + X^10 -> feedback from stage 3 and stage 10
    uint8_t g1[10];
    for (int i = 0; i < 10; ++i) {
        g1[i] = 1;
    }

    // G2: 10 stages, initial state from table
    // G2 polynomial: 1 + X^2 + X^3 + X^6 + X^8 + X^9 + X^10
    uint8_t g2[10];
    for (int i = 0; i < 10; ++i) {
        g2[i] = static_cast<uint8_t>((g2_init >> (9 - i)) & 1);
    }

    PrnSequence seq;
    seq.prn_id = prn_id;
    seq.chips.resize(NAVIC_GOLD_CODE_LENGTH);
    seq.bipolar_chips.resize(NAVIC_GOLD_CODE_LENGTH);
    seq.ones_count = 0;
    seq.zeros_count = 0;

    for (size_t n = 0; n < NAVIC_GOLD_CODE_LENGTH; ++n) {
        // Output chip is G1 stage 10 XOR G2 stage 10
        uint8_t out1 = g1[9];
        uint8_t out2 = g2[9];
        uint8_t chip = static_cast<uint8_t>(out1 ^ out2);

        seq.chips[n] = chip;
        seq.bipolar_chips[n] = static_cast<int8_t>(1 - 2 * static_cast<int>(chip));

        if (chip == 1) {
            seq.ones_count++;
        } else {
            seq.zeros_count++;
        }

        // G1 feedback: X^3 + X^10 -> g1[2] ^ g1[9]
        uint8_t fb1 = static_cast<uint8_t>(g1[2] ^ g1[9]);

        // G2 feedback: X^2 + X^3 + X^6 + X^8 + X^9 + X^10 -> g2[1] ^ g2[2] ^ g2[5] ^ g2[7] ^ g2[8] ^ g2[9]
        uint8_t fb2 = static_cast<uint8_t>(g2[1] ^ g2[2] ^ g2[5] ^ g2[7] ^ g2[8] ^ g2[9]);

        // Shift registers right
        for (int i = 9; i > 0; --i) {
            g1[i] = g1[i - 1];
            g2[i] = g2[i - 1];
        }
        g1[0] = fb1;
        g2[0] = fb2;
    }

    return seq;
}

int PrnCorrelator::compute_auto_correlation(const PrnSequence &seq, int lag) {
    int sum = 0;
    int n_chips = static_cast<int>(NAVIC_GOLD_CODE_LENGTH);
    int norm_lag = (lag % n_chips + n_chips) % n_chips;

    for (int i = 0; i < n_chips; ++i) {
        int j = (i + norm_lag) % n_chips;
        sum += static_cast<int>(seq.bipolar_chips[i]) * static_cast<int>(seq.bipolar_chips[j]);
    }
    return sum;
}

int PrnCorrelator::compute_cross_correlation(const PrnSequence &seq_k, const PrnSequence &seq_m, int lag) {
    int sum = 0;
    int n_chips = static_cast<int>(NAVIC_GOLD_CODE_LENGTH);
    int norm_lag = (lag % n_chips + n_chips) % n_chips;

    for (int i = 0; i < n_chips; ++i) {
        int j = (i + norm_lag) % n_chips;
        sum += static_cast<int>(seq_k.bipolar_chips[i]) * static_cast<int>(seq_m.bipolar_chips[j]);
    }
    return sum;
}

int PrnCorrelator::find_max_auto_sidelobe(const PrnSequence &seq) {
    int max_sidelobe = 0;
    int n_chips = static_cast<int>(NAVIC_GOLD_CODE_LENGTH);

    for (int tau = 1; tau < n_chips; ++tau) {
        int val = std::abs(compute_auto_correlation(seq, tau));
        if (val > max_sidelobe) {
            max_sidelobe = val;
        }
    }
    return max_sidelobe;
}

int PrnCorrelator::find_max_cross_sidelobe(const PrnSequence &seq_k, const PrnSequence &seq_m) {
    int max_sidelobe = 0;
    int n_chips = static_cast<int>(NAVIC_GOLD_CODE_LENGTH);

    for (int tau = 0; tau < n_chips; ++tau) {
        int val = std::abs(compute_cross_correlation(seq_k, seq_m, tau));
        if (val > max_sidelobe) {
            max_sidelobe = val;
        }
    }
    return max_sidelobe;
}

CorrelatorOutput PrnCorrelator::correlate_epl(
    const std::vector<int8_t> &baseband_samples,
    const PrnSequence &local_replica,
    double code_phase_chips,
    double chip_spacing,
    double prompt_i,
    double prompt_q)
{
    CorrelatorOutput out;
    size_t n_chips = local_replica.bipolar_chips.size();
    if (baseband_samples.empty() || n_chips == 0) {
        return out;
    }

    auto compute_corr_at = [&](double phase) -> double {
        double acc = 0.0;
        size_t n_samples = baseband_samples.size();
        for (size_t i = 0; i < n_samples; ++i) {
            double chip_idx_f = std::fmod(phase + static_cast<double>(i), static_cast<double>(n_chips));
            if (chip_idx_f < 0.0) {
                chip_idx_f += static_cast<double>(n_chips);
            }
            size_t idx0 = static_cast<size_t>(chip_idx_f) % n_chips;
            size_t idx1 = (idx0 + 1) % n_chips;
            double frac = chip_idx_f - std::floor(chip_idx_f);

            double rep_val = (1.0 - frac) * static_cast<double>(local_replica.bipolar_chips[idx0]) +
                             frac * static_cast<double>(local_replica.bipolar_chips[idx1]);

            acc += static_cast<double>(baseband_samples[i]) * rep_val;
        }
        return std::abs(acc);
    };

    out.early = compute_corr_at(code_phase_chips - chip_spacing);
    out.prompt = compute_corr_at(code_phase_chips);
    out.late = compute_corr_at(code_phase_chips + chip_spacing);

    // Non-coherent Early-Minus-Late DLL Discriminator: (E - L) / (E + L)
    double denom = out.early + out.late + 1e-9;
    out.dll_discriminator = (out.early - out.late) / denom;

    // Costas PLL carrier phase discriminator: atan2(Q_P, I_P)
    out.pll_discriminator = std::atan2(prompt_q, prompt_i);

    // Signal Quality Monitoring (SQM) peak asymmetry: |E - L| / (P + 1e-9)
    out.sqm_asymmetry = std::abs(out.early - out.late) / (out.prompt + 1e-9);
    out.sqm_anomaly = out.sqm_asymmetry > 0.20;

    return out;
}

} // namespace navic
} // namespace logihertz
