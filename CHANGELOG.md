# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.0] - 2026-11-13

### Added
- **ISRO NavIC 10-Stage LFSR Gold Code Generator**:
  - Implemented generator polynomials $G_1(X) = 1 + X^3 + X^{10}$ and $G_2(X) = 1 + X^2 + X^3 + X^6 + X^8 + X^9 + X^{10}$ for PRNs $1 \dots 14$.
  - Exact 1023-chip BPSK ranging sequence synthesis with balanced chip distributions.
  - Zero-lag auto-correlation peak ($R_{k,k}(0) = 1023$) and 3-valued cross-correlation sidelobe bounds ($|R_{k,m}(\tau)| \le 65$).
- **Early-Prompt-Late (EPL) Correlator & SQM Monitoring**:
  - Normalized Non-Coherent Early-Minus-Late DLL discriminator ($D_{\text{DLL}} = (E - L)/(E + L)$).
  - Costas PLL carrier phase discriminator ($D_{\text{PLL}} = \text{atan2}(Q_P, I_P)$).
  - Signal Quality Monitoring (SQM) correlation peak asymmetry metric ($\mathcal{A}_{\text{SQM}} = |E - L| / P$) for detecting evil-waveform spoofing hooks.
- **292-Bit NavIC Subframe Unpacker & Telemetry Decoder**:
  - 16-bit Preamble Synchronization detector (`0xEB90` and Costas $180^\circ$ phase-inverted `0x146F`).
  - Bit-accurate field unpacking: `TLMH` (8 bits), `TOWC` (17 bits), `Alert` (1 bit), `AutoNav` (1 bit), `SubframeID` (2 bits), `Spare` (1 bit), and `Payload` (216 bits).
  - Qualcomm CRC-24Q (`0x1864CFB`) mathematical division and verification across the 262-bit subframe body.
- **Dual-Frequency L5 & S-Band Ionospheric Delay Elimination**:
  - Exact dual-band ionospheric delay calculation: $\Delta\rho_{\text{iono}, L5} = \frac{f_S^2}{f_S^2 - f_{L5}^2} (\rho_{L5} - \rho_S)$ with $f_{L5} = 1176.45\text{ MHz}$ and $f_S = 2492.028\text{ MHz}$.
  - Ionospheric-free pseudorange combination: $\rho_{\text{IF}} = \frac{f_S^2 \rho_S - f_{L5}^2 \rho_{L5}}{f_S^2 - f_{L5}^2}$.
  - Single-band spoofing detection when computed ionospheric delay exceeds physical limits ($|\Delta\rho_{\text{iono}, L5}| > 65.0\text{ m}$).
- **10-Rule Sovereign PNT Security Posture Auditor**:
  - Implemented rules `LH-NAVIC-001` through `LH-NAVIC-010` mapping to CWE standards.
  - Standardized Posture Score computation: $S_{\text{posture}} = \max(0, 100 - \sum \text{Deductions})$.
  - Multi-format exporters: ANSI Terminal Box Table, Canonical JSON, and OASIS SARIF v2.1.0.
- **Production CLI (`bin/lh-navic`)**:
  - Subcommands: `scan`, `gen-prn`, `decode-subframe`, `calc-iono`, `--version`, and `--help`.
- **Zero-Dependency C++17 Architecture**:
  - Built exclusively with the C++17 Standard Library (`std`).
- **Test Runner & Verification Suite**:
  - 85+ comprehensive assertions in `tests/test_runner.cpp`.
