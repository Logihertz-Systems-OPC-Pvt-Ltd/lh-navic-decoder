# Security Policy

## Supported Versions

Logihertz Systems applies continuous security hardening to the India NavIC (IRNSS) baseband receiver and telemetry decoding ecosystem.

| Version | Supported          | Security Maintenance Level |
|---------|--------------------|----------------------------|
| 0.1.x   | :white_check_mark: | Active Core Development    |
| < 0.1.0 | :x:                | Deprecated / Prototype     |

---

## Threat Model & Security Posture

`lh-navic-decoder` (`bin/lh-navic`) operates within critical national sovereign positioning, navigation, and timing (PNT) environments, aerospace guidance systems, autonomous marine/ground robotics, and defense infrastructure.

The core security model encompasses:
1. **NavIC Gold Code PRN Tracking Integrity**: Guarding Against PRN cross-correlation false locks, off-peak sidelobe capture ($|R_{k,m}(\tau)| > 65$), and evil-waveform correlation asymmetry ($\mathcal{A}_{\text{SQM}} > 0.20$).
2. **Subframe Telemetry Authenticity**: Ensuring strict 16-bit preamble synchronization (`0xEB90` or Costas-inverted `0x146F`), full 24-bit Qualcomm CRC-24Q (`0x1864CFB`) mathematical verification, and immediate rejection of subframes asserting the satellite `Alert` flag.
3. **Dual-Band L5/S Ionospheric Anomaly Trapping**: Detecting selective single-band spoofing by enforcing physical upper bounds on calculated L5 ionospheric delay ($|\Delta\rho_{\text{iono}, L5}| \le 65.0\text{ m}$).
4. **Time-of-Week Count (TOWC) Rollback Defense**: Enforcing strict monotonicity across consecutive 12-second subframe epochs ($0 \dots 50399$) to reject time-replay and ephemeris rollback attacks.
5. **Zero Memory Corruption Vulnerabilities**: Strict `#![forbid(unsafe_code)]` equivalent in C++17: bounds-checked bit manipulation, zero raw pointer arithmetic escapes, no heap buffer overflows, and zero third-party dependencies.

---

## Reporting a Vulnerability

If you identify a security defect, baseband correlator bypass, CRC-24Q collision, or posture auditing discrepancy, report it immediately to our security engineering team:

- **Primary Contact**: Sameer Tawade
- **Email**: `security@logihertz.com`
- **Secondary Contact**: `ops@logihertz.com`

### Disclosure Protocol
1. Please provide a detailed description of the vulnerability, including:
   - Affected CLI subcommand or C++ module (`prn_correlator`, `telemetry_decoder`, `navic_auditor`)
   - Proof of Concept (PoC) subframe bitstream or receiver configuration manifest
   - Reproduction steps and expected vs. actual behavior
2. Logihertz Systems will acknowledge receipt within **24 hours** and provide an initial technical triage within **72 hours**.
3. Coordinated public disclosure will be scheduled once patches are verified across supported compilers (`g++` and `clang++`).
