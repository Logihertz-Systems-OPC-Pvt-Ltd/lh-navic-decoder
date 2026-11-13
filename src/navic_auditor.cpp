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

#include "navic_auditor.hpp"
#include <algorithm>

namespace logihertz {
namespace navic {

PostureReport NavicAuditor::audit(const NavicReceiverPolicy &policy) {
    PostureReport report;
    report.findings.reserve(10);
    int total_deductions = 0;

    // Rule 1: LH-NAVIC-001 (CRITICAL, 25, CWE-345)
    {
        PostureRuleFinding f;
        f.rule_id = "LH-NAVIC-001";
        f.title = "Dual-Frequency NavIC L5 + S-Band Ionospheric-Free Diversity";
        f.severity = "CRITICAL";
        f.deduction = 25;
        f.cwe = "CWE-345";
        f.remediation = "Enable simultaneous tracking and cross-correlation of both NavIC L5 (1176.45 MHz) and S-band (2492.028 MHz) carriers.";
        if (policy.dual_band_l5_s_enabled) {
            f.passed = true;
            f.message = "Dual-frequency L5 and S-band ionospheric-free pseudorange combination active.";
        } else {
            f.passed = false;
            f.message = "Single-frequency receiver mode active; vulnerable to synthetic ionospheric delays and selective single-band spoofing.";
            total_deductions += f.deduction;
        }
        report.findings.push_back(f);
    }

    // Rule 2: LH-NAVIC-002 (CRITICAL, 25, CWE-353)
    {
        PostureRuleFinding f;
        f.rule_id = "LH-NAVIC-002";
        f.title = "Subframe Qualcomm CRC-24Q & Preamble Sync Word Verification";
        f.severity = "CRITICAL";
        f.deduction = 25;
        f.cwe = "CWE-353";
        f.remediation = "Enforce strict 16-bit preamble sync word (0xEB90/0x146F) check and full 24-bit CRC-24Q polynomial integrity validation.";
        if (policy.verify_crc24q_integrity && policy.strict_sync_word_check) {
            f.passed = true;
            f.message = "Strict preamble synchronization and 24-bit CRC-24Q subframe integrity validation active.";
        } else {
            f.passed = false;
            f.message = "Subframe CRC-24Q verification or preamble synchronization check disabled; susceptible to corrupted or injected telemetry.";
            total_deductions += f.deduction;
        }
        report.findings.push_back(f);
    }

    // Rule 3: LH-NAVIC-003 (CRITICAL, 25, CWE-290)
    {
        PostureRuleFinding f;
        f.rule_id = "LH-NAVIC-003";
        f.title = "Satellite Alert Flag & User Range Accuracy (URA) Alarm Honor";
        f.severity = "CRITICAL";
        f.deduction = 25;
        f.cwe = "CWE-290";
        f.remediation = "Immediately reject space vehicles asserting the navigation Alert flag and bound maximum acceptable URA index to <= 7.";
        if (policy.honor_satellite_alert_flag && policy.max_allowed_ura_index <= 7) {
            f.passed = true;
            f.message = "Satellite Alert flag honor active and URA index bounded to nominal threshold (<= 7).";
        } else {
            f.passed = false;
            f.message = "Satellite Alert flag ignored or excessive URA index permitted (> 7); risks tracking degraded/unhealthy space vehicles.";
            total_deductions += f.deduction;
        }
        report.findings.push_back(f);
    }

    // Rule 4: LH-NAVIC-004 (HIGH, 10, CWE-682)
    {
        PostureRuleFinding f;
        f.rule_id = "LH-NAVIC-004";
        f.title = "Early-Prompt-Late Peak Symmetry & SQM Multipath/Spoof Guard";
        f.severity = "HIGH";
        f.deduction = 10;
        f.cwe = "CWE-682";
        f.remediation = "Activate EPL correlation peak symmetry monitoring and flag baseband Signal Quality Monitoring asymmetry > 0.20.";
        if (policy.epl_symmetry_sqm_enabled && policy.max_epl_asymmetry_ratio <= 0.20) {
            f.passed = true;
            f.message = "Early-Prompt-Late correlation symmetry and SQM evil-waveform distortion monitoring active (threshold <= 0.20).";
        } else {
            f.passed = false;
            f.message = "EPL peak symmetry monitor disabled or SQM asymmetry threshold > 0.20; vulnerable to correlation hook spoofing.";
            total_deductions += f.deduction;
        }
        report.findings.push_back(f);
    }

    // Rule 5: LH-NAVIC-005 (HIGH, 10, CWE-345)
    {
        PostureRuleFinding f;
        f.rule_id = "LH-NAVIC-005";
        f.title = "NavIC Navigation Message Authentication (NMA) Verification";
        f.severity = "HIGH";
        f.deduction = 10;
        f.cwe = "CWE-345";
        f.remediation = "Enable cryptographic digital signature verification on broadcast NavIC ephemeris and clock correction parameters.";
        if (policy.navic_nma_verification) {
            f.passed = true;
            f.message = "NavIC Navigation Message Authentication (NMA) cryptographic signature verification active.";
        } else {
            f.passed = false;
            f.message = "NavIC NMA cryptographic signature verification disabled; unauthenticated ephemeris accepted.";
            total_deductions += f.deduction;
        }
        report.findings.push_back(f);
    }

    // Rule 6: LH-NAVIC-006 (HIGH, 10, CWE-682)
    {
        PostureRuleFinding f;
        f.rule_id = "LH-NAVIC-006";
        f.title = "Time-of-Week Count (TOWC) Monotonicity & Rollback Protection";
        f.severity = "HIGH";
        f.deduction = 10;
        f.cwe = "CWE-682";
        f.remediation = "Enforce strict sequential increment of TOWC epochs across consecutive subframes and protect against week-number rollbacks.";
        if (policy.towc_monotonicity_check && policy.wn_rollover_guard_enabled) {
            f.passed = true;
            f.message = "TOWC 12-second subframe monotonicity and week-number rollover defense enabled.";
        } else {
            f.passed = false;
            f.message = "TOWC monotonicity check or week-number rollover guard disabled; vulnerable to time-replay attacks.";
            total_deductions += f.deduction;
        }
        report.findings.push_back(f);
    }

    // Rule 7: LH-NAVIC-007 (HIGH, 10, CWE-20)
    {
        PostureRuleFinding f;
        f.rule_id = "LH-NAVIC-007";
        f.title = "Indian Regional GSO/GEO Subsatellite Bounding Box Validation";
        f.severity = "HIGH";
        f.deduction = 10;
        f.cwe = "CWE-20";
        f.remediation = "Validate broadcast ephemeris against physical Indian Ocean regional coverage bounds for geostationary and geosynchronous orbits.";
        if (policy.verify_gso_geo_subsatellite_bounds) {
            f.passed = true;
            f.message = "Indian Ocean regional GSO/GEO subsatellite coordinate bounds checking active.";
        } else {
            f.passed = false;
            f.message = "Regional GSO/GEO orbital bounds verification disabled; unable to detect non-geosynchronous spoofed ephemeris.";
            total_deductions += f.deduction;
        }
        report.findings.push_back(f);
    }

    // Rule 8: LH-NAVIC-008 (MEDIUM, 4, CWE-682)
    {
        PostureRuleFinding f;
        f.rule_id = "LH-NAVIC-008";
        f.title = "Delay/Phase Lock Loop Bandwidth Tightening Against Pull-Off";
        f.severity = "MEDIUM";
        f.deduction = 4;
        f.cwe = "CWE-682";
        f.remediation = "Constrain baseband DLL bandwidth <= 2.0 Hz and Costas PLL bandwidth <= 18.0 Hz to prevent hostile frequency pull-off.";
        if (policy.dll_bandwidth_hz <= 2.0 && policy.pll_bandwidth_hz <= 18.0) {
            f.passed = true;
            f.message = "Baseband tracking loop bandwidths safely constrained (DLL <= 2.0 Hz, PLL <= 18.0 Hz).";
        } else {
            f.passed = false;
            f.message = "Excessive loop bandwidth configured (DLL > 2.0 Hz or PLL > 18.0 Hz); susceptible to carrier pull-off hijacking.";
            total_deductions += f.deduction;
        }
        report.findings.push_back(f);
    }

    // Rule 9: LH-NAVIC-009 (MEDIUM, 4, CWE-306)
    {
        PostureRuleFinding f;
        f.rule_id = "LH-NAVIC-009";
        f.title = "Authenticated Loopback Binding for Raw Subframe & RTCM Streams";
        f.severity = "MEDIUM";
        f.deduction = 4;
        f.cwe = "CWE-306";
        f.remediation = "Isolate raw telemetry ports to loopback (127.0.0.1) and require mutual cryptographic authentication for client connections.";
        if (policy.telemetry_bind_address == "127.0.0.1" && policy.require_client_auth) {
            f.passed = true;
            f.message = "Raw telemetry interface bound strictly to loopback (127.0.0.1) with client authentication enforced.";
        } else {
            f.passed = false;
            f.message = "Raw subframe telemetry bound to public/untrusted socket or missing client authentication.";
            total_deductions += f.deduction;
        }
        report.findings.push_back(f);
    }

    // Rule 10: LH-NAVIC-010 (LOW, 1, CWE-778)
    {
        PostureRuleFinding f;
        f.rule_id = "LH-NAVIC-010";
        f.title = "Autonomous Navigation Mode (AutoNav) Limit & Audit Logging";
        f.severity = "LOW";
        f.deduction = 1;
        f.cwe = "CWE-778";
        f.remediation = "Limit space vehicle operation in autonomous mode (AutoNav) to <= 168 hours (7 days) and log baseband anomaly metrics.";
        if (policy.autonav_max_hours <= 168 && policy.log_baseband_anomalies) {
            f.passed = true;
            f.message = "AutoNav operational threshold safely bounded (<= 168h) and baseband anomaly telemetry logging active.";
        } else {
            f.passed = false;
            f.message = "AutoNav duration limit exceeds 168 hours or baseband anomaly logging is disabled.";
            total_deductions += f.deduction;
        }
        report.findings.push_back(f);
    }

    report.total_deductions = total_deductions;
    report.score = std::max(0, 100 - total_deductions);

    int passed_count = 0;
    for (const auto &f : report.findings) {
        if (f.passed) passed_count++;
    }
    report.passed_count = passed_count;
    report.failed_count = static_cast<int>(report.findings.size()) - passed_count;

    if (report.score >= 90) {
        report.verdict = "HARDENED (Sovereign PNT Resilient)";
    } else if (report.score >= 60) {
        report.verdict = "DEFICIENT (Elevated Baseband Exploitation Risk)";
    } else {
        report.verdict = "VULNERABLE (Critical Baseband Exploitation Risk)";
    }

    return report;
}

NavicReceiverPolicy NavicAuditor::parse_policy_json(const std::string &json_str) {
    json::JsonValue root = json::JsonParser::parse(json_str);
    NavicReceiverPolicy policy;

    if (root.contains("dual_band_l5_s_enabled")) {
        policy.dual_band_l5_s_enabled = root["dual_band_l5_s_enabled"].as_bool();
    }
    if (root.contains("verify_crc24q_integrity")) {
        policy.verify_crc24q_integrity = root["verify_crc24q_integrity"].as_bool();
    }
    if (root.contains("strict_sync_word_check")) {
        policy.strict_sync_word_check = root["strict_sync_word_check"].as_bool();
    }
    if (root.contains("honor_satellite_alert_flag")) {
        policy.honor_satellite_alert_flag = root["honor_satellite_alert_flag"].as_bool();
    }
    if (root.contains("max_allowed_ura_index")) {
        policy.max_allowed_ura_index = root["max_allowed_ura_index"].as_int();
    }
    if (root.contains("epl_symmetry_sqm_enabled")) {
        policy.epl_symmetry_sqm_enabled = root["epl_symmetry_sqm_enabled"].as_bool();
    }
    if (root.contains("max_epl_asymmetry_ratio")) {
        policy.max_epl_asymmetry_ratio = root["max_epl_asymmetry_ratio"].as_double();
    }
    if (root.contains("navic_nma_verification")) {
        policy.navic_nma_verification = root["navic_nma_verification"].as_bool();
    }
    if (root.contains("towc_monotonicity_check")) {
        policy.towc_monotonicity_check = root["towc_monotonicity_check"].as_bool();
    }
    if (root.contains("wn_rollover_guard_enabled")) {
        policy.wn_rollover_guard_enabled = root["wn_rollover_guard_enabled"].as_bool();
    }
    if (root.contains("verify_gso_geo_subsatellite_bounds")) {
        policy.verify_gso_geo_subsatellite_bounds = root["verify_gso_geo_subsatellite_bounds"].as_bool();
    }
    if (root.contains("dll_bandwidth_hz")) {
        policy.dll_bandwidth_hz = root["dll_bandwidth_hz"].as_double();
    }
    if (root.contains("pll_bandwidth_hz")) {
        policy.pll_bandwidth_hz = root["pll_bandwidth_hz"].as_double();
    }
    if (root.contains("telemetry_bind_address")) {
        policy.telemetry_bind_address = root["telemetry_bind_address"].as_string();
    }
    if (root.contains("require_client_auth")) {
        policy.require_client_auth = root["require_client_auth"].as_bool();
    }
    if (root.contains("autonav_max_hours")) {
        policy.autonav_max_hours = root["autonav_max_hours"].as_int();
    }
    if (root.contains("log_baseband_anomalies")) {
        policy.log_baseband_anomalies = root["log_baseband_anomalies"].as_bool();
    }

    return policy;
}

json::JsonValue NavicAuditor::report_to_json(const PostureReport &report) {
    json::JsonValue root;
    root["score"] = report.score;
    root["passed_count"] = report.passed_count;
    root["failed_count"] = report.failed_count;
    root["total_deductions"] = report.total_deductions;
    root["verdict"] = report.verdict;

    std::vector<json::JsonValue> findings_arr;
    findings_arr.reserve(report.findings.size());
    for (const auto &f : report.findings) {
        json::JsonValue obj;
        obj["rule_id"] = f.rule_id;
        obj["title"] = f.title;
        obj["severity"] = f.severity;
        obj["deduction"] = f.deduction;
        obj["cwe"] = f.cwe;
        obj["passed"] = f.passed;
        obj["message"] = f.message;
        obj["remediation"] = f.remediation;
        findings_arr.push_back(obj);
    }
    root["findings"] = findings_arr;
    return root;
}

} // namespace navic
} // namespace logihertz
