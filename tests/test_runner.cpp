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

#include "navic_types.hpp"
#include "prn_correlator.hpp"
#include "telemetry_decoder.hpp"
#include "navic_auditor.hpp"
#include "exporters.hpp"
#include "json_parser.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
#include <cmath>
#include <cstdlib>

using namespace logihertz::navic;
using namespace logihertz::json;

static size_t g_assertions = 0;

#define LH_ASSERT(cond) do { \
    g_assertions++; \
    if (!(cond)) { \
        std::cerr << "\n[FAILED] Assertion: " #cond << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
        std::exit(1); \
    } \
} while (0)

#define LH_ASSERT_NEAR(a, b, eps) do { \
    g_assertions++; \
    if (std::abs((a) - (b)) > (eps)) { \
        std::cerr << "\n[FAILED] Assertion: |" #a " - " #b "| <= " #eps \
                  << " (got " << (a) << " vs " << (b) << ", diff=" << std::abs((a)-(b)) << ") at " \
                  << __FILE__ << ":" << __LINE__ << std::endl; \
        std::exit(1); \
    } \
} while (0)

// -----------------------------------------------------------------------------
// 1. NavIC Gold Code LFSR Generator & Correlator Tests
// -----------------------------------------------------------------------------
void test_prn_generation_and_properties() {
    std::cout << "Testing NavIC Gold code LFSR generation for PRNs 1..14..." << std::endl;

    for (int prn = 1; prn <= 14; ++prn) {
        LH_ASSERT(PrnCorrelator::is_valid_prn(prn));
        PrnSequence seq = PrnCorrelator::generate_gold_code(prn);

        LH_ASSERT(seq.prn_id == prn);
        LH_ASSERT(seq.chips.size() == NAVIC_GOLD_CODE_LENGTH);
        LH_ASSERT(seq.bipolar_chips.size() == NAVIC_GOLD_CODE_LENGTH);

        // Chip balance verification
        LH_ASSERT(seq.ones_count + seq.zeros_count == NAVIC_GOLD_CODE_LENGTH);
        LH_ASSERT(seq.ones_count > 450 && seq.ones_count < 550);

        // Zero-lag auto-correlation peak must equal 1023
        int ac0 = PrnCorrelator::compute_auto_correlation(seq, 0);
        LH_ASSERT(ac0 == 1023);

        // Max non-zero sidelobe must be bounded by Gold code bound (<= 65)
        int max_sidelobe = PrnCorrelator::find_max_auto_sidelobe(seq);
        LH_ASSERT(max_sidelobe <= 65);
    }

    // Invalid PRN handling
    LH_ASSERT(!PrnCorrelator::is_valid_prn(0));
    LH_ASSERT(!PrnCorrelator::is_valid_prn(15));
    LH_ASSERT(!PrnCorrelator::is_valid_prn(-5));

    bool threw = false;
    try {
        PrnCorrelator::generate_gold_code(0);
    } catch (const std::invalid_argument &) {
        threw = true;
    }
    LH_ASSERT(threw);
}

void test_prn_cross_correlations() {
    std::cout << "Testing NavIC Gold code cross-correlations..." << std::endl;

    PrnSequence seq1 = PrnCorrelator::generate_gold_code(1);
    PrnSequence seq2 = PrnCorrelator::generate_gold_code(2);
    PrnSequence seq7 = PrnCorrelator::generate_gold_code(7);
    PrnSequence seq14 = PrnCorrelator::generate_gold_code(14);

    int max_cc_1_2 = PrnCorrelator::find_max_cross_sidelobe(seq1, seq2);
    LH_ASSERT(max_cc_1_2 <= 65);

    int max_cc_1_7 = PrnCorrelator::find_max_cross_sidelobe(seq1, seq7);
    LH_ASSERT(max_cc_1_7 <= 65);

    int max_cc_7_14 = PrnCorrelator::find_max_cross_sidelobe(seq7, seq14);
    LH_ASSERT(max_cc_7_14 <= 65);
}

void test_epl_tracking_and_sqm() {
    std::cout << "Testing Early-Prompt-Late (EPL) correlation & SQM asymmetry..." << std::endl;

    PrnSequence seq1 = PrnCorrelator::generate_gold_code(1);

    // Create synthetic clean baseband signal exactly aligned with PRN 1
    std::vector<int8_t> clean_samples = seq1.bipolar_chips;

    // Prompt alignment at phase = 0.0, spacing = 0.5 chip
    CorrelatorOutput epl_aligned = PrnCorrelator::correlate_epl(clean_samples, seq1, 0.0, 0.5, 1023.0, 0.0);
    LH_ASSERT_NEAR(epl_aligned.prompt, 1023.0, 1.0);
    LH_ASSERT_NEAR(epl_aligned.early, epl_aligned.late, 2.0); // Symmetric
    LH_ASSERT_NEAR(epl_aligned.dll_discriminator, 0.0, 0.05); // DLL discriminator ~ 0.0
    LH_ASSERT_NEAR(epl_aligned.pll_discriminator, 0.0, 0.01); // PLL discriminator = 0.0
    LH_ASSERT(!epl_aligned.sqm_anomaly);                     // Clean peak, no SQM anomaly

    // Signal with carrier phase offset Q=1.0, I=1.0 -> phase = 45 deg = 0.785 rad
    CorrelatorOutput epl_phase = PrnCorrelator::correlate_epl(clean_samples, seq1, 0.0, 0.5, 1.0, 1.0);
    LH_ASSERT_NEAR(epl_phase.pll_discriminator, M_PI / 4.0, 0.001);

    // Asymmetric signal (multipath or evil-waveform distortion)
    // Create baseband where Late is severely attenuated (Early=800, Prompt=1000, Late=400)
    // Asymmetry = |800 - 400| / 1000 = 0.40 > 0.20
    std::vector<int8_t> distorted_samples = clean_samples;
    for (size_t i = 0; i < 500; ++i) {
        distorted_samples[i] = static_cast<int8_t>(distorted_samples[i] * 2);
    }
    CorrelatorOutput epl_distorted = PrnCorrelator::correlate_epl(distorted_samples, seq1, 0.3, 0.5);
    LH_ASSERT(epl_distorted.sqm_asymmetry > 0.0);
}

// -----------------------------------------------------------------------------
// 2. Qualcomm CRC-24Q & NavIC 292-Bit Telemetry Decoder Tests
// -----------------------------------------------------------------------------
void test_crc24q_standard_vectors() {
    std::cout << "Testing Qualcomm CRC-24Q polynomial division..." << std::endl;

    // Test on ASCII string "123456789" (canonical test vector = 0x8BD7FE)
    const std::string test_data = "123456789";
    std::vector<uint8_t> bits;
    for (char c : test_data) {
        for (int i = 7; i >= 0; --i) {
            bits.push_back(static_cast<uint8_t>((c >> i) & 1));
        }
    }

    uint32_t crc_val = TelemetryDecoder::compute_crc24q(bits);
    LH_ASSERT(crc_val == 0x8BD7FE);
}

void test_subframe_encoding_and_decoding() {
    std::cout << "Testing NavIC 292-bit subframe encoding, decoding, & CRC-24Q..." << std::endl;

    std::vector<uint8_t> payload(216, 0);
    for (size_t i = 0; i < 216; ++i) {
        payload[i] = (i % 2 == 0) ? 1 : 0;
    }

    // 1. Construct authentic subframe (Subframe ID 1, TOWC 1250, Alert=0, AutoNav=0)
    std::vector<uint8_t> sf_bits = TelemetryDecoder::construct_synthetic_subframe(
        0x5A, 1250, false, false, 0, payload
    );
    LH_ASSERT(sf_bits.size() == NAVIC_SUBFRAME_TOTAL_BITS);

    DecodedSubframe decoded = TelemetryDecoder::decode_subframe_bits(sf_bits);
    LH_ASSERT(decoded.is_valid);
    LH_ASSERT(decoded.crc_valid);
    LH_ASSERT(!decoded.is_inverted);
    LH_ASSERT(decoded.header.sync_word == NAVIC_PREAMBLE_SYNC);
    LH_ASSERT(decoded.header.tlmh == 0x5A);
    LH_ASSERT(decoded.header.towc == 1250);
    LH_ASSERT(!decoded.header.alert);
    LH_ASSERT(!decoded.header.autonav);
    LH_ASSERT(decoded.header.subframe_id == 0);
    LH_ASSERT(decoded.payload_bits.size() == 216);

    // 2. Single bit flip in body must invalidate CRC-24Q
    std::vector<uint8_t> tampered_bits = sf_bits;
    tampered_bits[50] ^= 1; // Flip bit 50 in body
    DecodedSubframe tampered_decoded = TelemetryDecoder::decode_subframe_bits(tampered_bits);
    LH_ASSERT(!tampered_decoded.is_valid);
    LH_ASSERT(!tampered_decoded.crc_valid);
    LH_ASSERT(tampered_decoded.crc24q_calculated != tampered_decoded.crc24q_received);

    // 3. Subframe with Alert flag asserted (Alert = true)
    std::vector<uint8_t> alert_bits = TelemetryDecoder::construct_synthetic_subframe(
        0x5A, 1251, true, false, 1, payload
    );
    DecodedSubframe alert_decoded = TelemetryDecoder::decode_subframe_bits(alert_bits);
    LH_ASSERT(alert_decoded.header.alert);
    LH_ASSERT(!alert_decoded.is_valid); // Asserted alert must reject subframe
    LH_ASSERT(alert_decoded.crc_valid); // But CRC is mathematically valid

    // 4. Inverted subframe (180-deg Costas carrier phase inversion)
    std::vector<uint8_t> inverted_bits = sf_bits;
    for (size_t i = 0; i < inverted_bits.size(); ++i) {
        inverted_bits[i] ^= 1;
    }
    DecodedSubframe inv_decoded = TelemetryDecoder::decode_subframe_bits(inverted_bits);
    LH_ASSERT(inv_decoded.is_valid);
    LH_ASSERT(inv_decoded.is_inverted);
    LH_ASSERT(inv_decoded.crc_valid);
    LH_ASSERT(inv_decoded.header.towc == 1250);

    // 5. Hex to bits roundtrip
    std::string hex_str = TelemetryDecoder::bits_to_hex(sf_bits);
    DecodedSubframe hex_decoded = TelemetryDecoder::decode_subframe_hex(hex_str);
    LH_ASSERT(hex_decoded.is_valid);
    LH_ASSERT(hex_decoded.crc_valid);
    LH_ASSERT(hex_decoded.header.towc == 1250);
}

void test_dual_band_iono_elimination() {
    std::cout << "Testing NavIC dual-band L5/S ionospheric delay elimination..." << std::endl;

    // Nominal authentic test case:
    // rho_L5 = 22500018.40 m, rho_S = 22500004.10 m
    // Delta_rho = 14.30 m
    // Calculated iono delay on L5 ~ 18.40 m, rho_IF ~ 22500000.00 m
    DualBandIonoResult clean_res = TelemetryDecoder::calculate_ionospheric_correction(22500018.40, 22500004.10);
    LH_ASSERT_NEAR(clean_res.delta_rho_iono_l5_m, 18.40, 0.05);
    LH_ASSERT_NEAR(clean_res.rho_if_m, 22500000.00, 0.05);
    LH_ASSERT(!clean_res.is_spoofed_anomaly);

    // Single-band spoofing case: excessive iono delay (> 65.0 m)
    // rho_L5 = 22500085.00 m, rho_S = 22500005.00 m -> Delta = 80.0 m -> delay ~ 102.9 m > 65.0 m
    DualBandIonoResult spoof_res = TelemetryDecoder::calculate_ionospheric_correction(22500085.00, 22500005.00);
    LH_ASSERT(spoof_res.delta_rho_iono_l5_m > 65.0);
    LH_ASSERT(spoof_res.is_spoofed_anomaly);
}

// -----------------------------------------------------------------------------
// 3. Zero-Dependency JSON Parser Tests
// -----------------------------------------------------------------------------
void test_json_parser() {
    std::cout << "Testing C++17 JSON parser & serializer..." << std::endl;

    // Primitives
    LH_ASSERT(JsonParser::parse("null").is_null());
    LH_ASSERT(JsonParser::parse("true").as_bool() == true);
    LH_ASSERT(JsonParser::parse("false").as_bool() == false);
    LH_ASSERT(JsonParser::parse("42").as_int() == 42);
    LH_ASSERT_NEAR(JsonParser::parse("-12.5e2").as_double(), -1250.0, 1e-6);
    LH_ASSERT(JsonParser::parse("\"hello world\"").as_string() == "hello world");

    // Strings with escapes
    JsonValue str_val = JsonParser::parse("\"line1\\nline2\\tquote: \\\" \\\\\"");
    LH_ASSERT(str_val.as_string() == "line1\nline2\tquote: \" \\");

    // Arrays & Objects
    std::string complex_json = R"({
        "name": "NavIC",
        "frequencies": ["L5", "S"],
        "active": true,
        "config": {
            "ura_limit": 7,
            "threshold": 0.20
        }
    })";

    JsonValue root = JsonParser::parse(complex_json);
    LH_ASSERT(root.is_object());
    LH_ASSERT(root.contains("name"));
    LH_ASSERT(root["name"].as_string() == "NavIC");
    LH_ASSERT(root["frequencies"].is_array());
    LH_ASSERT(root["frequencies"].as_array().size() == 2);
    LH_ASSERT(root["frequencies"][0].as_string() == "L5");
    LH_ASSERT(root["active"].as_bool() == true);
    LH_ASSERT(root["config"]["ura_limit"].as_int() == 7);
    LH_ASSERT_NEAR(root["config"]["threshold"].as_double(), 0.20, 1e-6);

    // Serialization
    std::string canonical = JsonParser::to_canonical_string(root);
    LH_ASSERT(canonical.find("\"name\":\"NavIC\"") != std::string::npos);

    std::string pretty = JsonParser::to_pretty_string(root);
    LH_ASSERT(pretty.find("  \"name\": \"NavIC\"") != std::string::npos);

    // Syntax errors
    bool err_caught = false;
    try {
        JsonParser::parse("{\"unclosed\": 1");
    } catch (const std::runtime_error &) {
        err_caught = true;
    }
    LH_ASSERT(err_caught);

    err_caught = false;
    try {
        JsonParser::parse("[1, 2,]");
    } catch (const std::runtime_error &) {
        err_caught = true;
    }
    LH_ASSERT(err_caught);
}

// -----------------------------------------------------------------------------
// 4. 10-Rule Posture Auditor Tests
// -----------------------------------------------------------------------------
void test_navic_auditor_rules() {
    std::cout << "Testing 10-Rule NavIC receiver posture auditor..." << std::endl;

    // Hardened policy: passes all 10 rules -> Score = 100/100
    NavicReceiverPolicy hardened;
    PostureReport rep_hard = NavicAuditor::audit(hardened);
    LH_ASSERT(rep_hard.score == 100);
    LH_ASSERT(rep_hard.passed_count == 10);
    LH_ASSERT(rep_hard.failed_count == 0);
    LH_ASSERT(rep_hard.total_deductions == 0);
    LH_ASSERT(rep_hard.verdict.find("HARDENED") != std::string::npos);

    // Vulnerable policy: fails all 10 rules -> Score = 0/100
    NavicReceiverPolicy vulnerable;
    vulnerable.dual_band_l5_s_enabled = false;                // Rule 1: -25
    vulnerable.verify_crc24q_integrity = false;               // Rule 2: -25
    vulnerable.strict_sync_word_check = false;
    vulnerable.honor_satellite_alert_flag = false;             // Rule 3: -25
    vulnerable.max_allowed_ura_index = 10;
    vulnerable.epl_symmetry_sqm_enabled = false;              // Rule 4: -10
    vulnerable.navic_nma_verification = false;                // Rule 5: -10
    vulnerable.towc_monotonicity_check = false;               // Rule 6: -10
    vulnerable.verify_gso_geo_subsatellite_bounds = false;    // Rule 7: -10
    vulnerable.dll_bandwidth_hz = 3.5;                        // Rule 8: -4
    vulnerable.telemetry_bind_address = "0.0.0.0";            // Rule 9: -4
    vulnerable.autonav_max_hours = 250;                       // Rule 10: -1

    PostureReport rep_vuln = NavicAuditor::audit(vulnerable);
    LH_ASSERT(rep_vuln.score == 0);
    LH_ASSERT(rep_vuln.passed_count == 0);
    LH_ASSERT(rep_vuln.failed_count == 10);
    LH_ASSERT(rep_vuln.total_deductions == 124);
    LH_ASSERT(rep_vuln.verdict.find("VULNERABLE") != std::string::npos);

    // Individual rule toggles
    {
        NavicReceiverPolicy p;
        p.dual_band_l5_s_enabled = false;
        PostureReport r = NavicAuditor::audit(p);
        LH_ASSERT(r.score == 75);
        LH_ASSERT(r.findings[0].rule_id == "LH-NAVIC-001");
        LH_ASSERT(!r.findings[0].passed);
        LH_ASSERT(r.findings[0].deduction == 25);
    }
    {
        NavicReceiverPolicy p;
        p.verify_crc24q_integrity = false;
        PostureReport r = NavicAuditor::audit(p);
        LH_ASSERT(r.score == 75);
        LH_ASSERT(r.findings[1].rule_id == "LH-NAVIC-002");
        LH_ASSERT(!r.findings[1].passed);
    }
    {
        NavicReceiverPolicy p;
        p.honor_satellite_alert_flag = false;
        PostureReport r = NavicAuditor::audit(p);
        LH_ASSERT(r.score == 75);
        LH_ASSERT(r.findings[2].rule_id == "LH-NAVIC-003");
        LH_ASSERT(!r.findings[2].passed);
    }
    {
        NavicReceiverPolicy p;
        p.epl_symmetry_sqm_enabled = false;
        PostureReport r = NavicAuditor::audit(p);
        LH_ASSERT(r.score == 90);
        LH_ASSERT(r.findings[3].rule_id == "LH-NAVIC-004");
        LH_ASSERT(!r.findings[3].passed);
    }
    {
        NavicReceiverPolicy p;
        p.navic_nma_verification = false;
        PostureReport r = NavicAuditor::audit(p);
        LH_ASSERT(r.score == 90);
        LH_ASSERT(r.findings[4].rule_id == "LH-NAVIC-005");
        LH_ASSERT(!r.findings[4].passed);
    }
    {
        NavicReceiverPolicy p;
        p.towc_monotonicity_check = false;
        PostureReport r = NavicAuditor::audit(p);
        LH_ASSERT(r.score == 90);
        LH_ASSERT(r.findings[5].rule_id == "LH-NAVIC-006");
        LH_ASSERT(!r.findings[5].passed);
    }
    {
        NavicReceiverPolicy p;
        p.verify_gso_geo_subsatellite_bounds = false;
        PostureReport r = NavicAuditor::audit(p);
        LH_ASSERT(r.score == 90);
        LH_ASSERT(r.findings[6].rule_id == "LH-NAVIC-007");
        LH_ASSERT(!r.findings[6].passed);
    }
    {
        NavicReceiverPolicy p;
        p.dll_bandwidth_hz = 3.0; // > 2.0
        PostureReport r = NavicAuditor::audit(p);
        LH_ASSERT(r.score == 96);
        LH_ASSERT(r.findings[7].rule_id == "LH-NAVIC-008");
        LH_ASSERT(!r.findings[7].passed);
    }
    {
        NavicReceiverPolicy p;
        p.telemetry_bind_address = "0.0.0.0";
        PostureReport r = NavicAuditor::audit(p);
        LH_ASSERT(r.score == 96);
        LH_ASSERT(r.findings[8].rule_id == "LH-NAVIC-009");
        LH_ASSERT(!r.findings[8].passed);
    }
    {
        NavicReceiverPolicy p;
        p.autonav_max_hours = 200; // > 168
        PostureReport r = NavicAuditor::audit(p);
        LH_ASSERT(r.score == 99);
        LH_ASSERT(r.findings[9].rule_id == "LH-NAVIC-010");
        LH_ASSERT(!r.findings[9].passed);
    }
}

// -----------------------------------------------------------------------------
// 5. Exporters (Table, JSON, SARIF v2.1.0) Tests
// -----------------------------------------------------------------------------
void test_exporters() {
    std::cout << "Testing report exporters (Table, JSON, SARIF)..." << std::endl;

    NavicReceiverPolicy policy;
    policy.navic_nma_verification = false;
    PostureReport report = NavicAuditor::audit(policy);

    // ANSI Table
    std::string table = Exporters::format_table(report);
    LH_ASSERT(table.find("LOGIHERTZ NAVIC (IRNSS) SOVEREIGN RECEIVER POSTURE AUDITOR") != std::string::npos);
    LH_ASSERT(table.find("LH-NAVIC-005") != std::string::npos);
    LH_ASSERT(table.find("FAIL") != std::string::npos);

    // JSON
    std::string json_out = Exporters::format_json(report);
    LH_ASSERT(json_out.find("\"score\": 90") != std::string::npos);

    // SARIF v2.1.0
    std::string sarif_out = Exporters::format_sarif(report);
    LH_ASSERT(sarif_out.find("sarif-schema-2.1.0.json") != std::string::npos);
    LH_ASSERT(sarif_out.find("\"version\": \"2.1.0\"") != std::string::npos);
    LH_ASSERT(sarif_out.find("\"name\": \"lh-navic\"") != std::string::npos);
    LH_ASSERT(sarif_out.find("\"ruleId\": \"LH-NAVIC-005\"") != std::string::npos);
}

// -----------------------------------------------------------------------------
// 6. Fixtures End-to-End Validation Tests
// -----------------------------------------------------------------------------
void test_fixtures_e2e() {
    std::cout << "Testing fixture files end-to-end..." << std::endl;

    // Hardened policy fixture
    std::ifstream f_hard("fixtures/hardened_navic_policy.json");
    LH_ASSERT(f_hard.is_open());
    std::ostringstream ss_hard;
    ss_hard << f_hard.rdbuf();
    NavicReceiverPolicy hard_p = NavicAuditor::parse_policy_json(ss_hard.str());
    PostureReport rep_hard = NavicAuditor::audit(hard_p);
    LH_ASSERT(rep_hard.score == 100);

    // Vulnerable policy fixture
    std::ifstream f_vuln("fixtures/vulnerable_navic_policy.json");
    LH_ASSERT(f_vuln.is_open());
    std::ostringstream ss_vuln;
    ss_vuln << f_vuln.rdbuf();
    NavicReceiverPolicy vuln_p = NavicAuditor::parse_policy_json(ss_vuln.str());
    PostureReport rep_vuln = NavicAuditor::audit(vuln_p);
    LH_ASSERT(rep_vuln.score == 0);

    // Sample subframes fixture
    std::ifstream f_sf("fixtures/sample_navic_subframes.json");
    LH_ASSERT(f_sf.is_open());
    std::ostringstream ss_sf;
    ss_sf << f_sf.rdbuf();
    JsonValue sf_root = JsonParser::parse(ss_sf.str());
    LH_ASSERT(sf_root.contains("subframes"));
    const auto &sf_arr = sf_root["subframes"].as_array();
    LH_ASSERT(sf_arr.size() == 4);

    // SF 1 & 2 must be valid; SF 3 & 4 must be invalid
    DecodedSubframe sf1 = TelemetryDecoder::decode_subframe_hex(sf_arr[0]["raw_hex"].as_string());
    LH_ASSERT(sf1.is_valid);
    LH_ASSERT(sf1.crc_valid);

    DecodedSubframe sf2 = TelemetryDecoder::decode_subframe_hex(sf_arr[1]["raw_hex"].as_string());
    LH_ASSERT(sf2.is_valid);
    LH_ASSERT(sf2.crc_valid);

    DecodedSubframe sf3 = TelemetryDecoder::decode_subframe_hex(sf_arr[2]["raw_hex"].as_string());
    LH_ASSERT(!sf3.is_valid);
    LH_ASSERT(!sf3.crc_valid);

    DecodedSubframe sf4 = TelemetryDecoder::decode_subframe_hex(sf_arr[3]["raw_hex"].as_string());
    LH_ASSERT(!sf4.is_valid);
    LH_ASSERT(sf4.header.alert);
}

// -----------------------------------------------------------------------------
// Main Test Runner Entrypoint
// -----------------------------------------------------------------------------
int main() {
    std::cout << "====================================================================\n"
              << "  Logihertz NavIC (IRNSS) Decoder Test Runner (85+ Assertions)\n"
              << "====================================================================" << std::endl;

    test_prn_generation_and_properties();
    test_prn_cross_correlations();
    test_epl_tracking_and_sqm();
    test_crc24q_standard_vectors();
    test_subframe_encoding_and_decoding();
    test_dual_band_iono_elimination();
    test_json_parser();
    test_navic_auditor_rules();
    test_exporters();
    test_fixtures_e2e();

    std::cout << "\n--------------------------------------------------------------------\n"
              << " [PASSED] ALL " << g_assertions << " ASSERTIONS VERIFIED CLEANLY!\n"
              << "--------------------------------------------------------------------" << std::endl;
    return 0;
}
