# lh-navic-decoder

[![CI](https://github.com/Logihertz-Systems-OPC-Pvt-Ltd/lh-navic-decoder/actions/workflows/ci.yml/badge.svg)](https://github.com/Logihertz-Systems-OPC-Pvt-Ltd/lh-navic-decoder/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Standard](https://img.shields.io/badge/Standard-C%2B%2B17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Specification](https://img.shields.io/badge/ISRO-NavIC%20SIS%20ICD%20v1.4-orange.svg)](https://www.isro.gov.in)
[![Dependencies](https://img.shields.io/badge/Dependencies-Zero%20(std%20only)-brightgreen.svg)](#dependencies)

**Production-grade, zero-dependency C++17 India NavIC (IRNSS) L5 & S-Band Software Baseband Receiver, Gold Code PRN Correlator, CRC-24Q Subframe Telemetry Decoder, Dual-Band Ionospheric Delay Eliminator, and 10-Rule Sovereign PNT Receiver Posture Auditor.**

`lh-navic-decoder` is an industrial-strength baseband processing and security auditing utility engineered for autonomous robotics, defense systems, sovereign infrastructure, and aerospace positioning platforms operating within India and the extended South Asian primary coverage region. Engineered in pure C++17 with strict zero external library dependencies, it delivers sub-microsecond PRN code correlation, cycle-accurate Early-Prompt-Late (EPL) delay tracking, bit-level 292-bit subframe unpacking with Qualcomm CRC-24Q parity validation, dual-band ionospheric error elimination, and comprehensive posture auditing across 10 security rules.

---

## Key Capabilities

- **10-Stage LFSR Gold Code Generator**: Generates ISRO NavIC SPS Gold code pseudo-random noise (PRN) sequences (1023 chips, 1.023 Mchips/s) for PRN 1 through 14 using linear feedback shift registers ($G_1(X) = 1 + X^3 + X^{10}$, $G_2(X) = 1 + X^2 + X^3 + X^6 + X^8 + X^9 + X^{10}$) with exact phase initializations.
- **Early-Prompt-Late (EPL) Correlation & SQM**: Implements Early-Prompt-Late code correlation with configurable correlator spacing ($\Delta = 0.5$ chip), calculating normalized Delay Lock Loop ($D_{\text{DLL}}$) and Phase Lock Loop ($D_{\text{PLL}}$) discriminators and Signal Quality Monitoring (SQM) peak asymmetry $\mathcal{A}_{\text{SQM}} = \frac{|E| - |L|}{|P|}$ to detect multipath and evil-waveform spoofing.
- **Qualcomm CRC-24Q Subframe Telemetry Decoder**: Computes 24-bit CRC-24Q cyclic redundancy checks over the 262-bit subframe telemetry data with generator polynomial $P(X) = X^{24} + X^{23} + X^{18} + X^{17} + X^{14} + X^{11} + X^{10} + X^7 + X^6 + X^5 + X^4 + X^3 + X + 1$ (`0x1864CFB`), validating 16-bit synchronization words (`0xEB90` or inverted `0x146F`) and unpacking telemetry fields (TLM, TOWC, Alert, AutoNav, Subframe ID).
- **Dual-Band L5/S Ionospheric Delay Eliminator**: Combines dual-frequency NavIC L5 ($1176.45\text{ MHz}$) and S-band ($2492.028\text{ MHz}$) pseudorange observations to synthesize the ionosphere-free pseudorange $\rho_{\text{IF}}$ and compute slant ionospheric delays $\Delta\rho_{\text{iono}, L5}$, alerting on synthetic single-band spoofing jumps exceeding $65.0\text{ m}$.
- **10-Rule Sovereign Receiver Posture Auditor**: Audits receiver runtime configurations against critical PNT integrity standards (`LH-NAVIC-001` through `LH-NAVIC-010`), scoring configurations out of 100 and emitting findings in ANSI Table, Canonical JSON, or OASIS SARIF v2.1.0 formats.
- **Zero External Dependencies**: Implemented strictly with the C++17 Standard Library (`std`). Zero Boost, zero external JSON parsers, zero third-party crypto libraries.

---

## Architectural Block Diagram

```
+----------------------------------------------------------------------------------------------------+
|                                    lh-navic Software Engine                                        |
+----------------------------------------------------------------------------------------------------+
                                                  |
  +-----------------------------------------------+-----------------------------------------------+
  |                               |                               |                               |
  v                               v                               v                               v
[PRN Correlator]        [Telemetry Decoder]             [Dual-Band Iono Engine]          [Receiver Auditor]
  * G1/G2 LFSR 1023       * 16-bit Sync Word (0xEB90)     * L5 (1176.45 MHz)              * 10 Security Rules
  * Auto/Cross Corr       * 262-bit Payload Extract       * S-Band (2492.028 MHz)         * Weight Deductions
  * Early-Prompt-Late     * CRC-24Q (0x1864CFB)           * Gamma Factor (~4.488)         * Score Computation
  * SQM Asymmetry Check   * Subframe ID, TOWC, Alert      * Spoofing Detector (>65m)      * Table/JSON/SARIF
```

---

## Mathematical Formulations

### 1. NavIC Gold Code LFSR Generator

In accordance with the ISRO IRNSS SIS ICD, the Standard Positioning Service (SPS) code sequence $C_i(t)$ for satellite PRN $i$ is generated by the modulo-2 addition of two maximal-length sequences $G_1$ and $G_2$:

$$C_i(n) = G_1(n) \oplus G_{2, i}(n), \quad n = 0, 1, \dots, 1022$$

The feedback polynomials defining the shift register taps are:

$$G_1(X) = 1 + X^3 + X^{10}$$

$$G_2(X) = 1 + X^2 + X^3 + X^6 + X^8 + X^9 + X^{10}$$

- **$G_1$ Shift Register**: 10-stage LFSR with feedback taps at stages 3 and 10, initialized to all ones (`0x3FF`).
- **$G_2$ Shift Register**: 10-stage LFSR with feedback taps at stages 2, 3, 6, 8, 9, 10, initialized with satellite-specific initial states:

| PRN | $G_2$ Initial State (Hex) | PRN | $G_2$ Initial State (Hex) |
|:---:|:-------------------------:|:---:|:-------------------------:|
| 1   | `0x3F6`                   | 8   | `0x307`                   |
| 2   | `0x3EC`                   | 9   | `0x20F`                   |
| 3   | `0x3D8`                   | 10  | `0x01F`                   |
| 4   | `0x3B0`                   | 11  | `0x03E`                   |
| 5   | `0x360`                   | 12  | `0x07D`                   |
| 6   | `0x2C1`                   | 13  | `0x0FA`                   |
| 7   | `0x183`                   | 14  | `0x1F5`                   |

### 2. Early-Prompt-Late (EPL) Tracking & SQM Asymmetry

The receiver correlator samples incoming baseband complex I/Q samples against local replicas at Early ($-\Delta/2$), Prompt ($0$), and Late ($+\Delta/2$) offsets:

$$D_{\text{DLL}} = \frac{|E| - |L|}{|E| + |L|}$$

$$D_{\text{PLL}} = \text{atan2}(Q_P, I_P)$$

$$\mathcal{A}_{\text{SQM}} = \frac{|E| - |L|}{|P|}$$

When evil-waveform spoofing or sharp asymmetric multipath occurs, $\mathcal{A}_{\text{SQM}}$ deviates significantly from zero. A threshold of $\mathcal{A}_{\text{SQM}} > 0.20$ flags anomalous tracking conditions.

### 3. Qualcomm CRC-24Q Parity Calculation

Each 292-bit NavIC subframe protects 262 bits of telemetry data using a 24-bit cyclic redundancy check (CRC-24Q):

$$P(X) = X^{24} + X^{23} + X^{18} + X^{17} + X^{14} + X^{11} + X^{10} + X^7 + X^6 + X^5 + X^4 + X^3 + X + 1$$

Represented in hexadecimal as `0x1864CFB` (or `0x864CFB` for lower 24 bits). Standard validation vector: the 9-byte ASCII string `"123456789"` computes to CRC-24Q checksum `0x8BD7FE`.

### 4. Dual-Band L5/S Ionospheric Delay Elimination

Dual-frequency observations eliminate first-order ionospheric dispersion ($1/f^2$ delay). Given carrier frequencies $f_{L5} = 1176.45\text{ MHz}$ and $f_S = 2492.028\text{ MHz}$:

$$\gamma = \left(\frac{f_S}{f_{L5}}\right)^2 = \left(\frac{2492.028}{1176.45}\right)^2 \approx 4.488355609$$

The L5 slant ionospheric delay $\Delta\rho_{\text{iono}, L5}$ is:

$$\Delta\rho_{\text{iono}, L5} = \frac{\gamma}{\gamma - 1} (\rho_{L5} - \rho_S)$$

The ionosphere-free pseudorange $\rho_{\text{IF}}$ is:

$$\rho_{\text{IF}} = \frac{\gamma \rho_S - \rho_{L5}}{\gamma - 1} = \rho_{L5} - \Delta\rho_{\text{iono}, L5}$$

Under nominal space weather conditions, slant ionospheric delay does not exceed $65.0\text{ m}$. An ionospheric delay $\Delta\rho_{\text{iono}, L5} > 65.0\text{ m}$ indicates unsynchronized single-band RF replay or selective pseudorange injection.

---

## 10-Rule Sovereign Receiver Posture Audit Matrix

| Rule ID | Severity | Weight | CWE | Title | Condition Triggered | Remediation |
|:---|:---:|:---:|:---:|:---|:---|:---|
| `LH-NAVIC-001` | CRITICAL | 25 | CWE-345 | Dual-Frequency L5+S Ionospheric Elimination Disabled | `dual_band_iono_elimination == false` | Enable dual-band L5+S ionospheric delay compensation in tracking loops. |
| `LH-NAVIC-002` | CRITICAL | 25 | CWE-353 | Telemetry CRC-24Q / Sync Word Parity Verification Disabled | `crc24q_verification == false` | Enforce strict CRC-24Q subframe parity and 16-bit synchronization validation. |
| `LH-NAVIC-003` | CRITICAL | 25 | CWE-290 | Satellite Alert Flag Ignored or Permissive URA Masking | `alert_flag_check == false` or `max_ura_index > 7` | Enforce immediate satellite exclusion on Alert flag or URA index $> 7$. |
| `LH-NAVIC-004` | HIGH | 10 | CWE-682 | EPL Peak Symmetry / Signal Quality Monitoring Disabled | `sqm_monitoring == false` or `max_sqm_asymmetry > 0.20` | Activate SQM peak asymmetry metrics with threshold $\le 0.20$. |
| `LH-NAVIC-005` | HIGH | 10 | CWE-345 | Sovereign Navigation Message Authentication (NMA) Disabled | `nma_authentication == false` | Enable NavIC ECDSA/TESLA Navigation Message Authentication. |
| `LH-NAVIC-006` | HIGH | 10 | CWE-682 | TOWC Monotonicity and Subframe Rollover Guard Disabled | `towc_monotonicity_check == false` | Enforce strict TOWC delta increments and week-rollover boundary checks. |
| `LH-NAVIC-007` | HIGH | 10 | CWE-20 | Subsatellite Regional Geostationary Orbital Bounding Box Disabled | `gso_bounding_box_check == false` | Verify satellite sub-points remain within Indian regional GSO/GEO bounds. |
| `LH-NAVIC-008` | MEDIUM | 4 | CWE-682 | Excessive Tracking Loop Bandwidth Vulnerable to Noise/Spoofing | `dll_bandwidth_hz > 2.0` or `pll_bandwidth_hz > 18.0` | Restrict DLL tracking bandwidth $\le 2.0\text{ Hz}$ and PLL $\le 18.0\text{ Hz}$. |
| `LH-NAVIC-009` | MEDIUM | 4 | CWE-306 | Unauthenticated Raw Telemetry or Subframe Feed Socket Binding | Non-loopback `0.0.0.0` binding without TLS/mTLS authentication | Bind telemetry export sockets to `127.0.0.1` or mandate mTLS. |
| `LH-NAVIC-010` | LOW | 1 | CWE-778 | Unconstrained Autonomous Orbit (AutoNav) Duration or Missing Logs | `max_autonav_duration_hours > 168` or `anomaly_logging == false` | Limit AutoNav mode to $\le 168\text{ h}$ and enforce audit logging. |

The receiver posture score is calculated monotonically:

$$S_{\text{posture}} = \max\left(0, 100 - \sum_{\text{findings}} W_i\right)$$

- $S_{\text{posture}} \ge 90$: **EXCELLENT** (Zero critical/high vulnerabilities; sovereign operational certification).
- $70 \le S_{\text{posture}} < 90$: **ADEQUATE** (Minor warnings; dual-band and CRC active).
- $50 \le S_{\text{posture}} < 70$: **VULNERABLE** (Missing SQM, loose loop bandwidths, or missing NMA).
- $S_{\text{posture}} < 50$: **CRITICAL** (Single-frequency mode, disabled parity checks, or permissive alert masks).

---

## Building and Installation

### Prerequisites

- Modern Linux environment (Ubuntu 20.04+, Debian 11+, RHEL 8+, Arch Linux).
- C++17 compliant compiler (`g++` 8.0+ or `clang++` 7.0+).
- GNU Make 4.0+.
- Standard C++ Library (`libstdc++` or `libc++`).

### Build from Source

```bash
# Clone the repository
git clone https://github.com/Logihertz-Systems-OPC-Pvt-Ltd/lh-navic-decoder.git
cd lh-navic-decoder

# Compile release binary and run test suite
make all
make test

# Verify installed binary
./bin/lh-navic --version
```

---

## CLI Command Reference

### 1. Security Posture Audit (`scan`)

Audits a NavIC receiver configuration policy against all 10 security rules:

```bash
# ANSI Terminal Box Table format (default)
./bin/lh-navic scan fixtures/hardened_navic_policy.json

# Machine-readable Canonical JSON format
./bin/lh-navic scan fixtures/hardened_navic_policy.json --format json

# OASIS SARIF v2.1.0 format for GitHub Security integration
./bin/lh-navic scan fixtures/vulnerable_navic_policy.json --format sarif --output audit_report.sarif
```

### 2. PRN Code Generator & Correlation (`gen-prn`)

Generates ISRO NavIC SPS Gold code sequences for specified satellite PRNs (1 through 14):

```bash
# Generate PRN 1 Gold code sequence
./bin/lh-navic gen-prn --prn 1

# Generate PRN and output raw comma-separated chips
./bin/lh-navic gen-prn --prn 5 --raw

# Compute auto-correlation and cross-correlation against another PRN
./bin/lh-navic gen-prn --prn 1 --correlate-with 2
```

### 3. Subframe Telemetry Decoding (`decode-subframe`)

Decodes and validates 292-bit NavIC subframe vectors:

```bash
# Decode raw subframe hexadecimal string
./bin/lh-navic decode-subframe --hex "EB904100000000000000000000000000000000000000000000000000000000000000000008892400"

# Decode a JSON fixture file containing multiple subframe captures
./bin/lh-navic decode-subframe fixtures/sample_navic_subframes.json
```

### 4. Dual-Band Ionospheric Delay Elimination (`calc-iono`)

Calculates L5 slant ionospheric delay and ionosphere-free pseudorange from dual-frequency observations:

```bash
# Nominal atmospheric conditions
./bin/lh-navic calc-iono --rho-l5 22500018.40 --rho-s 22500004.10

# Single-band spoofing detection simulation
./bin/lh-navic calc-iono --rho-l5 22500120.00 --rho-s 22500010.00
```

---

## Policy Configuration Format

Receiver policies are written in standard JSON:

```json
{
  "receiver_id": "NAVIC-RX-BHARAT-01",
  "firmware_version": "3.4.1-defense",
  "dual_band_iono_elimination": true,
  "crc24q_verification": true,
  "alert_flag_check": true,
  "max_ura_index": 3,
  "sqm_monitoring": true,
  "max_sqm_asymmetry": 0.12,
  "nma_authentication": true,
  "towc_monotonicity_check": true,
  "gso_bounding_box_check": true,
  "dll_bandwidth_hz": 1.5,
  "pll_bandwidth_hz": 12.0,
  "raw_telemetry_socket_bind": "127.0.0.1:9050",
  "socket_requires_auth": true,
  "max_autonav_duration_hours": 72,
  "anomaly_logging": true
}
```

---

## Test Suite & Verification

The built-in verification suite evaluates 85+ rigorous assertions covering:
- Exact 1023-chip Gold code sequence generation across all 14 NavIC PRNs.
- Auto-correlation peak ($1023$) and sidelobe bounds ($\le 65$).
- Qualcomm CRC-24Q RFC/SIS test vector calculation.
- 16-bit sync word detection and telemetry field bit unpacking.
- Dual-band L5/S ionospheric delay math and spoofing threshold validation.
- JSON parser schema validation and edge case handling.
- Full 10-rule posture evaluation with hardened and vulnerable fixtures.

Run the test suite with:

```bash
make test
```

---

## Security Disclosure

For security vulnerabilities, refer to [SECURITY.md](SECURITY.md). Please report issues to `security@logihertz.com`.

---

## Contributing

Review [CONTRIBUTING.md](CONTRIBUTING.md) and [CODE_OF_CONDUCT.md](CODE_OF_CONDUCT.md) for coding standards, pull request procedures, and community guidelines.

---

## License & Attribution

Copyright (c) 2026 Logihertz Systems (OPC) Private Limited & Sameer Tawade.

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at [http://www.apache.org/licenses/LICENSE-2.0](http://www.apache.org/licenses/LICENSE-2.0).
