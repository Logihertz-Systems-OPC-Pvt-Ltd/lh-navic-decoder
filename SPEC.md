# Engineering & Technical Architecture Specification

**Specification ID**: `LH-SPEC-2026-049`  
**Repository**: `lh-navic-decoder`  
**CLI Executable**: `lh-navic`  
**Standard Release Version**: `0.1.0`  
**Standard License**: `Apache-2.0`  
**Classification**: `Industrial Sovereign Cybersecurity & Critical Infrastructure Telemetry`

---

## Section 1: Document Control & Specification Metadata

| Metadata Attribute | Value |
| :--- | :--- |
| **Document Identification** | `LH-SPEC-2026-049` |
| **System Name** | `Logihertz Systems Sovereign Ecosystem` |
| **Target Component** | `lh-navic-decoder` (`lh-navic`) |
| **Target Language / Platform** | `C++` |
| **Legal Entity** | Logihertz Systems (OPC) Private Limited |
| **Corporate Identity (CIN)** | `U72100MH2025OPC463857` |
| **GSTIN Registration** | `27AAGCL6394P1ZL` |
| **Principal Architect** | Sameer Tawade (`ops@logihertz.com` / `security@logihertz.com`) |
| **Headquarters & Registered Office** | Mumbai, Maharashtra, Republic of India |
| **Official Portal** | [https://www.logihertz.com](https://www.logihertz.com) |
| **Repository URL** | [https://github.com/Logihertz-Systems-OPC-Pvt-Ltd/lh-navic-decoder](https://github.com/Logihertz-Systems-OPC-Pvt-Ltd/lh-navic-decoder) |
| **Document Status** | `APPROVED FOR PRODUCTION & INDUSTRIAL CERTIFICATION` |
| **Effective Release Date** | `2026-11-16` |

---

## Section 2: Normative RFC 2119 Requirements & System Invariants

This technical architecture specification enforces strict deterministic execution invariants in accordance with **RFC 2119** (`MUST`, `MUST NOT`, `REQUIRED`, `SHALL`, `SHALL NOT`, `SHOULD`, `RECOMMENDED`, `MAY`).

1. **Zero-Daemon Execution Invariant**:
   - The CLI binary `lh-navic` `MUST` execute as a clean, self-contained, stateless, zero-daemon process.
   - It `MUST NOT` spawn background daemons, fork unmonitored worker processes, or establish persistent Unix domain socket listeners unless explicitly invoked in daemon mode.
   - All state transitions `MUST` be deterministic, ephemeral, and strictly bounded by the execution lifecycle.

2. **Zero External Runtime Dependency Mandate**:
   - The software `MUST NOT` load, link, or depend upon third-party runtime package registries (e.g. PyPI, npm, Packagist, crates.io, Maven) at runtime.
   - For `C++`, all cryptographic, mathematical, serialization, and networking primitives `MUST` be implemented exclusively via language standard libraries or clean-room internal zero-dependency modules.

3. **Deterministic Air-Gapped Operation**:
   - The system `MUST` function with 100% feature parity in air-gapped, isolated, and offline enclaves.
   - The software `MUST NOT` initiate unsolicited network calls, telemetry telemetry pings, phone-home beacons, or external CDN queries.
   - All verification baselines, heuristics, and schemas `MUST` be packaged locally within the distribution.

4. **Constant-Time Cryptographic Comparison**:
   - All cryptographic comparison routines (hashes, HMAC tags, signatures, secret tokens, and commitments) `MUST` execute in constant time (`subtle.ConstantTimeCompare`, `CRYPTO_memcmp`, or equivalent).
   - The implementation `MUST NOT` short-circuit string comparisons when evaluating sensitive keys, thereby mitigating timing side-channel attacks.

5. **Memory Safety & Explicit Zeroization**:
   - All transient buffers containing sensitive key material, decrypted plaintexts, seeds, and cryptographic nonces `MUST` be explicitly overwritten with zeroes immediately following completion of the cryptographic operation.
   - For C/C++ implementations, bounds checks and safe buffer wrappers `MUST` prevent stack/heap overflows. For Rust implementations, `#![forbid(unsafe_code)]` `MUST` be enforced.

6. **Standardized Telemetry Interoperability**:
   - All security assessment outputs `MUST` support machine-readable ingestion via OASIS SARIF v2.1.0 JSON and Canonical Indented JSON schemas, alongside ANSI console tables.

---

## Section 3: Core Mathematical & Algorithmic Specification

### 3.1 Domain Engine Mathematical Foundation: Dual-Band Ionospheric Delay Cancellation Function

The underlying computational core of `lh-navic-decoder` is formally governed by the following mathematical and algorithmic formulations:

$$I = \frac{f_{\text{L5}}^2}{f_{\text{S}}^2 - f_{\text{L5}}^2} \left(P_{\text{L5}} - P_{\text{S}}\right)$$

### 3.2 Standardized Logihertz Security Posture Score Formulation

To achieve absolute cross-tool normalization across all 60 repositories in the Logihertz Systems Ecosystem, the security posture score $S_{\text{posture}}$ is evaluated on a standardized continuous integer scale $[0, 100]$:

$$S_{\text{posture}} = \max\left(0, \; 100 - \left(25 \cdot c_{\text{CRITICAL}} + 10 \cdot c_{\text{HIGH}} + 4 \cdot c_{\text{MEDIUM}} + 1 \cdot c_{\text{LOW}}\right)\right)$$

Where:
- $c_{\text{CRITICAL}}$ represents the total count of triggered Critical-severity normative rules (deduction: $-25$ per finding).
- $c_{\text{HIGH}}$ represents the total count of triggered High-severity normative rules (deduction: $-10$ per finding).
- $c_{\text{MEDIUM}}$ represents the total count of triggered Medium-severity normative rules (deduction: $-4$ per finding).
- $c_{\text{LOW}}$ represents the total count of triggered Low-severity normative rules (deduction: $-1$ per finding).

The final compliance verdict is strictly bifurcated:
$$\text{Verdict} = \begin{cases} \text{PASS} & \text{if } S_{\text{posture}} = 100 \land \sum c_i = 0 \\ \text{FAIL} & \text{otherwise} \end{cases}$$

---

## Section 4: Normative 10-Rule Detection Matrix

The detection engine evaluates configurations, manifests, firmware, protocols, and streams against the following 10 deterministic security rules:

| Rule ID | Severity | Weight | CWE ID | Trigger Predicate / Condition | Remediation Guideline |
| :--- | :--- | :---: | :---: | :--- | :--- |
| `LH-NAVIC-001` | `CRITICAL` | `25` | `CWE-345` | dual_band_iono_elimination == false | Enable dual-band L5+S ionospheric delay compensation in tracking loops. |
| `LH-NAVIC-002` | `CRITICAL` | `25` | `CWE-353` | crc24q_verification == false | Enforce strict CRC-24Q subframe parity and 16-bit synchronization validation. |
| `LH-NAVIC-003` | `CRITICAL` | `25` | `CWE-290` | alert_flag_check == false or max_ura_index > 7 | Enforce immediate satellite exclusion on Alert flag or URA index $> 7$. |
| `LH-NAVIC-004` | `HIGH` | `10` | `CWE-682` | sqm_monitoring == false or max_sqm_asymmetry > 0.20 | Activate SQM peak asymmetry metrics with threshold $\le 0.20$. |
| `LH-NAVIC-005` | `HIGH` | `10` | `CWE-345` | nma_authentication == false | Enable NavIC ECDSA/TESLA Navigation Message Authentication. |
| `LH-NAVIC-006` | `HIGH` | `10` | `CWE-682` | towc_monotonicity_check == false | Enforce strict TOWC delta increments and week-rollover boundary checks. |
| `LH-NAVIC-007` | `HIGH` | `10` | `CWE-20` | gso_bounding_box_check == false | Verify satellite sub-points remain within Indian regional GSO/GEO bounds. |
| `LH-NAVIC-008` | `MEDIUM` | `4` | `CWE-682` | dll_bandwidth_hz > 2.0 or pll_bandwidth_hz > 18.0 | Restrict DLL tracking bandwidth $\le 2.0\text{ Hz}$ and PLL $\le 18.0\text{ Hz}$. |
| `LH-NAVIC-009` | `MEDIUM` | `4` | `CWE-306` | Non-loopback 0.0.0.0 binding without TLS/mTLS authentication | Bind telemetry export sockets to 127.0.0.1 or mandate mTLS. |
| `LH-NAVIC-010` | `LOW` | `1` | `CWE-778` | max_autonav_duration_hours > 168 or anomaly_logging == false | Limit AutoNav mode to $\le 168\text{ h}$ and enforce audit logging. |

---

## Section 5: CLI Contract, Exit Codes & OASIS SARIF v2.1.0 Specification

### 5.1 Command-Line Interface Syntax

The primary entrypoint `lh-navic` conforms to standard POSIX argument conventions:

```text
USAGE:
  lh-navic <command> [flags] [arguments]

CORE COMMANDS:
  scan <target> [--format table|json|sarif] [--output <file>]
      Execute deterministic security evaluation against target manifest or telemetry stream.
  verify <target>
      Validate operational parameters, cryptographic checksums, and structural invariants.
  --version, version
      Output release version, architecture, and legal attribution.
  --help, help
      Display comprehensive operational usage and command reference.
```

### 5.2 Deterministic Exit Code Protocol

| Exit Code | Classification | Operational Semantics |
| :---: | :--- | :--- |
| `0` | **SUCCESS / PASS** | Target passed all checks; zero Critical/High violations detected; $S_{\text{posture}} = 100$. |
| `1` | **FINDINGS / FAIL** | Target triggered one or more security rule violations; policy threshold breached. |
| `2` | **ERROR / USAGE** | Invalid CLI invocation, syntax error, missing input file, or fatal I/O failure. |

### 5.3 OASIS SARIF v2.1.0 Schema Compliance

Exported SARIF documents validate against the official OASIS schema (`sarif-schema-2.1.0.json`). Each result object maps:
- `ruleId`: Standardized identifier (`LH-NAVIC-001` .. `LH-NAVIC-010`).
- `level`: SARIF severity levels (`error` for Critical/High, `warning` for Medium, `note` for Low).
- `message.text`: Comprehensive explanation, CWE mapping, and actionable remediation steps.
- `locations[].physicalLocation.artifactLocation.uri`: Absolute or normalized relative path of target artifact.

---

## 6. Standalone Future Development Roadmap (`v0.2.0` — `v0.5.0`)

`lh-navic-decoder` is developed as an independent, standalone open-source project. The following roadmap defines the planned architectural, algorithmic, and protocol enhancements for upcoming point releases of `lh-navic-decoder`:

| Target Release | Milestone Focus | Planned Technical Deliverables for `lh-navic-decoder` |
|---|---|---|
| **`v0.2.0`** | Parser & Protocol Expansion | Add an FFT-based parallel code-phase acquisition engine (circular cross-correlation via Radix-2 FFT/IFFT) for rapid cold-start PRN search across $\pm 10\text{ kHz}$ Doppler bins. |
| **`v0.3.0`** | Deep Algorithmic & Stateful Analysis | Implement a complete 4+ satellite Weighted Least-Squares (WLS) ECEF-to-WGS84 $(\text{Lat}, \text{Lon}, \text{Alt})$ navigation solver with GDOP/PDOP calculation. |
| **`v0.4.0`** | Remediation, Synthesis & Format Interop | Add NavIC Subframe 3 Ionospheric Grid Point (IGP) delay interpolation and UTC / Indian Standard Time (IST) offset decoding. |
| **`v0.5.0`** | High-Throughput Performance & Formal Verification | Support modernized NavIC L1 SPS ($1575.42\text{ MHz}$) BOC/MBOC signal decoding and RINEX v3.05 observation/navigation file export. |

### 6.1 `v0.2.0` — Parser & Protocol Expansion
- **Format & Protocol Ingestion**: Add an FFT-based parallel code-phase acquisition engine (circular cross-correlation via Radix-2 FFT/IFFT) for rapid cold-start PRN search across $\pm 10\text{ kHz}$ Doppler bins.
- **Zero-Dependency Architecture**: Pure standard library implementation with deterministic parsing guarantees and strict error handling.
- **Verification & Telemetry**: Regression test suites verifying schema edge-cases, malformed payload rejections, and SARIF v2.1.0 output fidelity.

### 6.2 `v0.3.0` — Deep Algorithmic & Stateful Analysis
- **Algorithmic Core**: Implement a complete 4+ satellite Weighted Least-Squares (WLS) ECEF-to-WGS84 $(\text{Lat}, \text{Lon}, \text{Alt})$ navigation solver with GDOP/PDOP calculation.
- **State Machine & Analysis Engine**: High-fidelity state tracking, cycle detection, and invariant checking with minimal computational overhead.
- **Differential & Regression Verification**: Comprehensive test harnesses covering stateful transitions, edge conditions, and mathematical boundary constraints.

### 6.3 `v0.4.0` — Remediation, Synthesis & Format Interoperability
- **Synthesis & Interoperability**: Add NavIC Subframe 3 Ionospheric Grid Point (IGP) delay interpolation and UTC / Indian Standard Time (IST) offset decoding.
- **Automated Remediation Workflows**: Idempotent generation of verified configuration patches, policy rules, and standardized cross-format exports.
- **Contract & Artifact Validation**: Cryptographic baseline verification and end-to-end integration testing for synthesized deliverables.

### 6.4 `v0.5.0` — High-Throughput Performance & Formal Verification
- **High-Throughput Optimization**: Support modernized NavIC L1 SPS ($1575.42\text{ MHz}$) BOC/MBOC signal decoding and RINEX v3.05 observation/navigation file export.
- **Hardware & Kernel Acceleration**: Vectorized SIMD/eBPF/sub-millisecond streaming execution with bounded memory allocation.
- **Formal Verification & Assurance**: Mathematical proof of invariant preservation, false-positive calibration, and high-assurance certification baselines.

---

## Section 7: Verification, Quality Assurance & Legal Attribution

### 7.1 Automated Verification Protocol

Prior to distribution, all builds must execute the standardized Logihertz test suite:
- Unit and mathematical property tests verifying all equations and edge cases.
- Static analysis with zero warnings under standard strict linters.
- Subprocess CLI verification covering exit codes `0`, `1`, and `2`.

### 7.2 Copyright & Governance Notice

```text
Copyright 2025-2026 Logihertz Systems (OPC) Private Limited.
Creator, Founder, Director & Sole Developer: Sameer Tawade <ops@logihertz.com> / <security@logihertz.com>
Official Corporate Portal: https://www.logihertz.com
```

Licensed under the Apache License, Version 2.0 (the "License"); you may not use this software except in compliance with the License. You may obtain a copy of the License at `http://www.apache.org/licenses/LICENSE-2.0`.
