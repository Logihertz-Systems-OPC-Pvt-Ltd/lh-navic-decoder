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
#include <string>
#include <vector>

using namespace logihertz::navic;
using namespace logihertz::json;

namespace {

const char *VERSION_STRING = "lh-navic 0.1.0 (Logihertz Sovereign PNT Baseband Receiver & Auditor)";

void print_version() {
    std::cout << VERSION_STRING << "\n";
}

void print_help() {
    std::cout << VERSION_STRING << "\n\n"
              << "USAGE:\n"
              << "    bin/lh-navic <SUBCOMMAND> [OPTIONS]\n\n"
              << "SUBCOMMANDS:\n"
              << "    scan <policy.json>                Audit NavIC receiver posture against 10 rules\n"
              << "    gen-prn --prn <1..14>             Synthesize 1023-chip NavIC SPS Gold code sequence\n"
              << "    decode-subframe <subframes.json>  Decode 292-bit subframes & verify CRC-24Q integrity\n"
              << "    calc-iono --rho-l5 <m> --rho-s <m> Calculate L5/S ionospheric delay & iono-free range\n\n"
              << "OPTIONS:\n"
              << "    --format <table|json|sarif>       Output format for scan results (default: table)\n"
              << "    --output <file>                   Write command output to file instead of stdout\n"
              << "    -h, --help                        Display this help text\n"
              << "    -V, --version                     Display version information\n\n"
              << "EXAMPLES:\n"
              << "    # Audit hardened receiver policy:\n"
              << "    bin/lh-navic scan fixtures/hardened_navic_policy.json --format table\n\n"
              << "    # Export SARIF v2.1.0 report for CI/CD integration:\n"
              << "    bin/lh-navic scan fixtures/vulnerable_navic_policy.json --format sarif\n\n"
              << "    # Synthesize NavIC PRN 1 Gold code:\n"
              << "    bin/lh-navic gen-prn --prn 1\n\n"
              << "    # Decode raw 292-bit subframe telemetry stream:\n"
              << "    bin/lh-navic decode-subframe fixtures/sample_navic_subframes.json\n\n"
              << "    # Eliminate ionospheric delay using dual-band L5/S pseudoranges:\n"
              << "    bin/lh-navic calc-iono --rho-l5 22500018.40 --rho-s 22500004.10\n\n";
}

std::string read_entire_file(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + path);
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

void write_output(const std::string &content, const std::string &out_path) {
    if (out_path.empty()) {
        std::cout << content;
    } else {
        std::ofstream ofs(out_path);
        if (!ofs.is_open()) {
            throw std::runtime_error("Could not write to output file: " + out_path);
        }
        ofs << content;
    }
}

} // namespace

int main(int argc, char **argv) {
    if (argc < 2) {
        print_help();
        return 2;
    }

    std::string subcommand = argv[1];
    if (subcommand == "-V" || subcommand == "--version" || subcommand == "version") {
        print_version();
        return 0;
    }
    if (subcommand == "-h" || subcommand == "--help" || subcommand == "help") {
        print_help();
        return 0;
    }

    try {
        if (subcommand == "scan") {
            if (argc < 3) {
                std::cerr << "Error: 'scan' requires a path to a policy JSON file\n";
                return 2;
            }
            std::string policy_path;
            std::string format = "table";
            std::string output_file;

            for (int i = 2; i < argc; ++i) {
                std::string arg = argv[i];
                if (arg == "--format") {
                    if (i + 1 < argc) {
                        format = argv[++i];
                    } else {
                        std::cerr << "Error: --format requires an argument (table, json, or sarif)\n";
                        return 2;
                    }
                } else if (arg == "--output") {
                    if (i + 1 < argc) {
                        output_file = argv[++i];
                    } else {
                        std::cerr << "Error: --output requires a file path\n";
                        return 2;
                    }
                } else if (arg.rfind("-", 0) != 0 && policy_path.empty()) {
                    policy_path = arg;
                } else {
                    std::cerr << "Error: Unknown argument: " << arg << "\n";
                    return 2;
                }
            }

            if (policy_path.empty()) {
                std::cerr << "Error: Missing policy JSON file path\n";
                return 2;
            }

            std::string content = read_entire_file(policy_path);
            NavicReceiverPolicy policy = NavicAuditor::parse_policy_json(content);
            PostureReport report = NavicAuditor::audit(policy);

            std::string rendered;
            if (format == "json") {
                rendered = Exporters::format_json(report, true);
            } else if (format == "sarif") {
                rendered = Exporters::format_sarif(report);
            } else {
                rendered = Exporters::format_table(report);
            }

            write_output(rendered, output_file);
            return (report.failed_count == 0) ? 0 : 1;
        }

        if (subcommand == "gen-prn") {
            int prn_id = 1;
            for (int i = 2; i < argc; ++i) {
                std::string arg = argv[i];
                if (arg == "--prn") {
                    if (i + 1 < argc) {
                        prn_id = std::stoi(argv[++i]);
                    } else {
                        std::cerr << "Error: --prn requires an integer between 1 and 14\n";
                        return 2;
                    }
                } else if (arg.rfind("-", 0) != 0) {
                    prn_id = std::stoi(arg);
                }
            }

            if (!PrnCorrelator::is_valid_prn(prn_id)) {
                std::cerr << "Error: PRN ID must be between 1 and 14\n";
                return 2;
            }

            PrnSequence seq = PrnCorrelator::generate_gold_code(prn_id);
            int max_sidelobe = PrnCorrelator::find_max_auto_sidelobe(seq);
            std::cout << Exporters::format_prn_table(seq, max_sidelobe);
            return 0;
        }

        if (subcommand == "decode-subframe") {
            if (argc < 3) {
                std::cerr << "Error: 'decode-subframe' requires a path to a subframes JSON or hex file\n";
                return 2;
            }
            std::string file_path = argv[2];
            std::string content = read_entire_file(file_path);

            bool any_invalid = false;

            // Check if content is JSON
            try {
                JsonValue root = JsonParser::parse(content);
                std::vector<std::string> hex_strings;

                if (root.is_object() && root.contains("subframes")) {
                    for (const auto &item : root["subframes"].as_array()) {
                        if (item.is_string()) {
                            hex_strings.push_back(item.as_string());
                        } else if (item.is_object() && item.contains("raw_hex")) {
                            hex_strings.push_back(item["raw_hex"].as_string());
                        }
                    }
                } else if (root.is_array()) {
                    for (const auto &item : root.as_array()) {
                        if (item.is_string()) {
                            hex_strings.push_back(item.as_string());
                        } else if (item.is_object() && item.contains("raw_hex")) {
                            hex_strings.push_back(item["raw_hex"].as_string());
                        }
                    }
                }

                if (hex_strings.empty()) {
                    std::cerr << "Error: No valid subframes found in JSON\n";
                    return 2;
                }

                for (size_t idx = 0; idx < hex_strings.size(); ++idx) {
                    std::cout << "\n--- Subframe Record #" << (idx + 1) << " ---";
                    DecodedSubframe sf = TelemetryDecoder::decode_subframe_hex(hex_strings[idx]);
                    std::cout << Exporters::format_subframe_table(sf);
                    if (!sf.is_valid) {
                        any_invalid = true;
                    }
                }
            } catch (const std::exception &e) {
                // Treat content as raw hex lines
                std::istringstream iss(content);
                std::string line;
                size_t count = 0;
                while (std::getline(iss, line)) {
                    if (line.empty() || line[0] == '#') continue;
                    count++;
                    std::cout << "\n--- Subframe Record #" << count << " ---";
                    DecodedSubframe sf = TelemetryDecoder::decode_subframe_hex(line);
                    std::cout << Exporters::format_subframe_table(sf);
                    if (!sf.is_valid) {
                        any_invalid = true;
                    }
                }
                if (count == 0) {
                    std::cerr << "Error parsing subframe input: " << e.what() << "\n";
                    return 2;
                }
            }

            return any_invalid ? 1 : 0;
        }

        if (subcommand == "calc-iono") {
            double rho_l5 = 0.0;
            double rho_s = 0.0;
            bool has_l5 = false;
            bool has_s = false;

            for (int i = 2; i < argc; ++i) {
                std::string arg = argv[i];
                if (arg == "--rho-l5") {
                    if (i + 1 < argc) {
                        rho_l5 = std::stod(argv[++i]);
                        has_l5 = true;
                    }
                } else if (arg == "--rho-s") {
                    if (i + 1 < argc) {
                        rho_s = std::stod(argv[++i]);
                        has_s = true;
                    }
                }
            }

            if (!has_l5 || !has_s) {
                std::cerr << "Error: 'calc-iono' requires both --rho-l5 and --rho-s in meters\n";
                return 2;
            }

            DualBandIonoResult res = TelemetryDecoder::calculate_ionospheric_correction(rho_l5, rho_s);
            std::cout << Exporters::format_iono_table(res);
            return res.is_spoofed_anomaly ? 1 : 0;
        }

        std::cerr << "Error: Unknown subcommand '" << subcommand << "'\n";
        print_help();
        return 2;

    } catch (const std::exception &ex) {
        std::cerr << "Error: " << ex.what() << "\n";
        return 2;
    }
}
