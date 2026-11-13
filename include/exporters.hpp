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

#ifndef EXPORTERS_HPP
#define EXPORTERS_HPP

#include "navic_types.hpp"
#include <string>

namespace logihertz {
namespace navic {

/// Multi-format report rendering engine (ANSI Table, Canonical JSON, OASIS SARIF v2.1.0).
class Exporters {
public:
    /// Formats a posture audit report as an ANSI terminal box table.
    static std::string format_table(const PostureReport &report);

    /// Formats a posture audit report as standard JSON.
    static std::string format_json(const PostureReport &report, bool pretty = true);

    /// Formats a posture audit report as an OASIS SARIF v2.1.0 document.
    static std::string format_sarif(const PostureReport &report);

    /// Formats generated PRN Gold code characteristics into an ANSI table.
    static std::string format_prn_table(const PrnSequence &prn, int max_sidelobe);

    /// Formats decoded subframe telemetry into an ANSI table.
    static std::string format_subframe_table(const DecodedSubframe &sf);

    /// Formats dual-band L5/S ionospheric correction output into an ANSI table.
    static std::string format_iono_table(const DualBandIonoResult &iono);
};

} // namespace navic
} // namespace logihertz

#endif // EXPORTERS_HPP
