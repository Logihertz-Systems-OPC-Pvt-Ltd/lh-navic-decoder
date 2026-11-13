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

#include "exporters.hpp"
#include "json_parser.hpp"
#include "navic_auditor.hpp"
#include <sstream>
#include <iomanip>

namespace logihertz {
namespace navic {

namespace {
    // ANSI Terminal Styles
    constexpr const char *RESET = "\033[0m";
    constexpr const char *BOLD = "\033[1m";
    constexpr const char *RED = "\033[31m";
    constexpr const char *GREEN = "\033[32m";
    constexpr const char *YELLOW = "\033[33m";
    constexpr const char *BLUE = "\033[34m";
    constexpr const char *CYAN = "\033[36m";
    constexpr const char *BG_RED = "\033[41;37;1m";
    constexpr const char *BG_GREEN = "\033[42;30;1m";
} // namespace

std::string Exporters::format_table(const PostureReport &report) {
    std::ostringstream oss;

    const char *score_bg = (report.score >= 90) ? BG_GREEN : (report.score >= 60 ? YELLOW : BG_RED);
    const char *score_fg = (report.score >= 90) ? GREEN : (report.score >= 60 ? YELLOW : RED);

    oss << "\n" << BOLD << CYAN
        << "========================================================================================\n"
        << "  LOGIHERTZ NAVIC (IRNSS) SOVEREIGN RECEIVER POSTURE AUDITOR (lh-navic v0.1.0)\n"
        << "========================================================================================\n"
        << RESET;

    oss << "  Overall Posture Score: " << score_bg << " " << report.score << " / 100 " << RESET
        << "   Verdict: " << score_fg << report.verdict << RESET << "\n";
    oss << "  Rules Passed: " << GREEN << report.passed_count << "/10" << RESET
        << "  |  Rules Failed: " << (report.failed_count > 0 ? RED : GREEN) << report.failed_count << "/10" << RESET
        << "  |  Total Deductions: -" << report.total_deductions << "\n\n";

    oss << "┌──────────────┬──────────┬────────┬───────┬─────────────────────────────────────────────┐\n";
    oss << "│ Rule ID      │ Severity │ Status │ Deduct│ Rule Description                            │\n";
    oss << "├──────────────┼──────────┼────────┼───────┼─────────────────────────────────────────────┤\n";

    for (const auto &f : report.findings) {
        const char *status_str = f.passed ? " PASS " : " FAIL ";
        const char *status_col = f.passed ? GREEN : RED;

        const char *sev_col = CYAN;
        if (f.severity == "CRITICAL") sev_col = RED;
        else if (f.severity == "HIGH") sev_col = YELLOW;
        else if (f.severity == "LOW") sev_col = BLUE;

        std::string title_sub = f.title;
        if (title_sub.size() > 43) {
            title_sub = title_sub.substr(0, 40) + "...";
        }

        oss << "│ " << std::left << std::setw(12) << f.rule_id
            << " │ " << sev_col << std::setw(8) << f.severity << RESET
            << " │ " << status_col << status_str << RESET
            << " │ -" << std::setw(4) << (f.passed ? 0 : f.deduction)
            << " │ " << std::setw(43) << title_sub << " │\n";

        if (!f.passed) {
            std::string rem_sub = f.remediation;
            if (rem_sub.size() > 56) {
                rem_sub = rem_sub.substr(0, 53) + "...";
            }
            oss << "│              └─► " << BOLD << "Remediation: " << RESET
                << std::setw(56) << rem_sub << " │\n";
        }
    }

    oss << "└──────────────┴──────────┴────────┴───────┴─────────────────────────────────────────────┘\n\n";
    return oss.str();
}

std::string Exporters::format_json(const PostureReport &report, bool pretty) {
    json::JsonValue root = NavicAuditor::report_to_json(report);
    if (pretty) {
        return json::JsonParser::to_pretty_string(root);
    }
    return json::JsonParser::to_canonical_string(root);
}

std::string Exporters::format_sarif(const PostureReport &report) {
    json::JsonValue root;
    root["$schema"] = "https://raw.githubusercontent.com/oasis-tcs/sarif-spec/master/Schemata/sarif-schema-2.1.0.json";
    root["version"] = "2.1.0";

    json::JsonValue driver;
    driver["name"] = "lh-navic";
    driver["version"] = "0.1.0";
    driver["informationUri"] = "https://github.com/Logihertz-Systems-OPC-Pvt-Ltd/lh-navic-decoder";

    std::vector<json::JsonValue> rules_arr;
    for (const auto &f : report.findings) {
        json::JsonValue r;
        r["id"] = f.rule_id;
        r["name"] = f.title;

        json::JsonValue short_desc;
        short_desc["text"] = f.title;
        r["shortDescription"] = short_desc;

        json::JsonValue help;
        help["text"] = "CWE: " + f.cwe + ". Remediation: " + f.remediation;
        r["help"] = help;

        rules_arr.push_back(r);
    }
    driver["rules"] = rules_arr;

    json::JsonValue tool;
    tool["driver"] = driver;

    std::vector<json::JsonValue> results_arr;
    for (const auto &f : report.findings) {
        if (!f.passed) {
            json::JsonValue res;
            res["ruleId"] = f.rule_id;

            std::string level = "error";
            if (f.severity == "MEDIUM") level = "warning";
            else if (f.severity == "LOW") level = "note";
            res["level"] = level;

            json::JsonValue msg;
            msg["text"] = f.message;
            res["message"] = msg;

            results_arr.push_back(res);
        }
    }

    json::JsonValue run;
    run["tool"] = tool;
    run["results"] = results_arr;

    root["runs"] = std::vector<json::JsonValue>{run};
    return json::JsonParser::to_pretty_string(root);
}

std::string Exporters::format_prn_table(const PrnSequence &prn, int max_sidelobe) {
    std::ostringstream oss;

    oss << "\n" << BOLD << CYAN
        << "========================================================================================\n"
        << "  ISRO NAVIC (IRNSS) GOLD CODE PRN SYNTHESIS REPORT (PRN ID " << prn.prn_id << ")\n"
        << "========================================================================================\n"
        << RESET;

    oss << "  Sequence Length: " << prn.chips.size() << " chips (1.023 Mcps)\n";
    oss << "  Chip Distribution: " << prn.ones_count << " ones / " << prn.zeros_count << " zeros\n";
    oss << "  Zero-Lag Auto-Correlation Peak: " << GREEN << "1023" << RESET << " (Theoretical: 1023)\n";
    oss << "  Max Non-Zero Sidelobe: " << (max_sidelobe <= 65 ? GREEN : RED) << max_sidelobe << RESET
        << " (ISRO Gold Code Bound: <= 65)\n";

    oss << "  First 32 Chips: ";
    for (size_t i = 0; i < 32 && i < prn.chips.size(); ++i) {
        oss << static_cast<int>(prn.chips[i]);
    }
    oss << "\n\n";

    return oss.str();
}

std::string Exporters::format_subframe_table(const DecodedSubframe &sf) {
    std::ostringstream oss;

    oss << "\n" << BOLD << CYAN
        << "========================================================================================\n"
        << "  NAVIC 292-BIT NAVIGATION SUBFRAME TELEMETRY DECODING REPORT\n"
        << "========================================================================================\n"
        << RESET;

    const char *status_col = sf.is_valid ? GREEN : RED;
    const char *status_str = sf.is_valid ? "VALID & AUTHENTIC" : "INVALID / CORRUPTED";

    oss << "  Verdict: " << status_col << status_str << RESET << "\n";
    if (!sf.error_message.empty()) {
        oss << "  Reason: " << RED << sf.error_message << RESET << "\n";
    }

    oss << "┌────────────────────────────────┬──────────────────────────────────────────────────────┐\n";
    oss << "│ Telemetry Field                │ Decoded Value                                        │\n";
    oss << "├────────────────────────────────┼──────────────────────────────────────────────────────┤\n";

    std::ostringstream sync_hex;
    sync_hex << "0x" << std::uppercase << std::hex << sf.header.sync_word
             << (sf.is_inverted ? " (Costas Inverted 0x146F Resolved)" : " (Standard)");
    oss << "│ Preamble Sync Word             │ " << std::left << std::setw(52) << sync_hex.str() << " │\n";

    oss << "│ TLMH (Telemetry Message Header)│ " << std::left << std::setw(52) << static_cast<int>(sf.header.tlmh) << " │\n";

    std::ostringstream towc_str;
    towc_str << sf.header.towc << " (" << (sf.header.towc * 12) << "s into NavIC week)";
    oss << "│ Time-of-Week Count (TOWC)      │ " << std::left << std::setw(52) << towc_str.str() << " │\n";

    oss << "│ Alert Flag                     │ " << std::left << std::setw(52)
        << (sf.header.alert ? "1 (ALERT - SATELLITE UNHEALTHY)" : "0 (Nominal Operation)") << " │\n";

    oss << "│ AutoNav Flag                   │ " << std::left << std::setw(52)
        << (sf.header.autonav ? "1 (Autonomous Mode)" : "0 (Ground Uplinked Mode)") << " │\n";

    std::ostringstream sf_str;
    sf_str << static_cast<int>(sf.header.subframe_id + 1) << " (Subframe " << static_cast<int>(sf.header.subframe_id + 1) << ")";
    oss << "│ Subframe ID                    │ " << std::left << std::setw(52) << sf_str.str() << " │\n";

    std::ostringstream crc_str;
    crc_str << "0x" << std::uppercase << std::hex << sf.crc24q_received
            << " (Calculated: 0x" << sf.crc24q_calculated
            << (sf.crc_valid ? " - MATCH)" : " - MISMATCH)");
    oss << "│ Qualcomm CRC-24Q (0x1864CFB)   │ " << std::left << std::setw(52) << crc_str.str() << " │\n";

    oss << "│ Payload Bit Length             │ " << std::left << std::setw(52) << sf.payload_bits.size() << " │\n";

    oss << "└────────────────────────────────┴──────────────────────────────────────────────────────┘\n\n";
    return oss.str();
}

std::string Exporters::format_iono_table(const DualBandIonoResult &iono) {
    std::ostringstream oss;

    oss << "\n" << BOLD << CYAN
        << "========================================================================================\n"
        << "  NAVIC DUAL-BAND L5 & S-BAND IONOSPHERIC DELAY ELIMINATION REPORT\n"
        << "========================================================================================\n"
        << RESET;

    const char *status_col = (!iono.is_spoofed_anomaly) ? GREEN : RED;
    oss << "  Status: " << status_col << iono.status << RESET << "\n\n";

    oss << "┌──────────────────────────────────────────┬────────────────────────────────────────────┐\n";
    oss << "│ Parameter Description                    │ Measured / Computed Value                  │\n";
    oss << "├──────────────────────────────────────────┼────────────────────────────────────────────┤\n";

    oss << "│ NavIC L5 Measured Pseudorange (1176.45M) │ " << std::left << std::fixed << std::setprecision(2)
        << std::setw(39) << iono.rho_l5_m << " m │\n";

    oss << "│ NavIC S-Band Measured Pseudorange(2492M) │ " << std::left << std::fixed << std::setprecision(2)
        << std::setw(39) << iono.rho_s_m << " m │\n";

    oss << "│ Calculated L5 Ionospheric Delay          │ " << std::left << std::fixed << std::setprecision(2)
        << std::setw(39) << iono.delta_rho_iono_l5_m << " m │\n";

    oss << "│ Ionospheric-Free Combined Pseudorange    │ " << std::left << std::fixed << std::setprecision(2)
        << std::setw(39) << iono.rho_if_m << " m │\n";

    oss << "│ Single-Band Spoofing Anomaly Flag        │ " << std::left
        << std::setw(42) << (iono.is_spoofed_anomaly ? "YES (DETECTED)" : "NO (Clean)") << " │\n";

    oss << "└──────────────────────────────────────────┴────────────────────────────────────────────┘\n\n";
    return oss.str();
}

} // namespace navic
} // namespace logihertz
