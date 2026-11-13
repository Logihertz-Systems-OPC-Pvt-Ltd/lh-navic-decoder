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

#ifndef PRN_CORRELATOR_HPP
#define PRN_CORRELATOR_HPP

#include "navic_types.hpp"

namespace logihertz {
namespace navic {

/// NavIC SPS 10-Stage LFSR Gold Code Generator & EPL Correlator.
class PrnCorrelator {
public:
    /// Returns true if PRN ID is in valid range [1..14].
    static bool is_valid_prn(int prn_id);

    /// Retrieves the ISRO SIS ICD canonical 10-bit initial state vector for G2.
    static uint16_t get_g2_initial_state(int prn_id);

    /// Synthesizes the exact 1023-chip NavIC SPS Gold code sequence for a given PRN.
    static PrnSequence generate_gold_code(int prn_id);

    /// Computes circular auto-correlation at specified integer chip lag (0..1022).
    static int compute_auto_correlation(const PrnSequence &seq, int lag);

    /// Computes circular cross-correlation between two PRNs at specified lag (0..1022).
    static int compute_cross_correlation(const PrnSequence &seq_k, const PrnSequence &seq_m, int lag);

    /// Scans all non-zero lags (1..1022) to determine the peak auto-correlation sidelobe.
    static int find_max_auto_sidelobe(const PrnSequence &seq);

    /// Scans all lags (0..1022) to determine the peak cross-correlation sidelobe.
    static int find_max_cross_sidelobe(const PrnSequence &seq_k, const PrnSequence &seq_m);

    /// Computes Early-Prompt-Late (EPL) correlator magnitudes and tracking discriminators.
    static CorrelatorOutput correlate_epl(
        const std::vector<int8_t> &baseband_samples,
        const PrnSequence &local_replica,
        double code_phase_chips,
        double chip_spacing = 0.5,
        double prompt_i = 1.0,
        double prompt_q = 0.0);
};

} // namespace navic
} // namespace logihertz

#endif // PRN_CORRELATOR_HPP
