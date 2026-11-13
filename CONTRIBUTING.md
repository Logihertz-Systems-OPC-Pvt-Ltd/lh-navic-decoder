# Contributing to `lh-navic-decoder`

Thank you for contributing to **`lh-navic-decoder`**, the zero-dependency C++17 India NavIC (IRNSS) Software Baseband Receiver, PRN Correlator, CRC-24Q Telemetry Decoder, and Sovereign PNT Security Posture Auditor.

---

## Architectural & Governance Principles

1. **Zero External Third-Party Library Dependencies**:
   - The entire codebase must compile using strictly the **C++17 Standard Library (`std`)**.
   - Do NOT introduce external libraries (e.g., Boost, nlohmann-json, OpenSSL, or FFTW).
   - All PRN Gold code generation, correlation arithmetic, CRC-24Q polynomial division, JSON parsing, and SARIF export must remain self-contained.

2. **Strict Compiler Quality & Warnings**:
   - Code must compile with zero warnings under both `g++` and `clang++`:
     ```bash
     -std=c++17 -O3 -Wall -Wextra -Werror -pedantic
     ```

3. **Adherence to ISRO NavIC Signal-in-Space (SIS) ICD**:
   - Implementations of the SPS ranging code generators, 10-stage LFSR polynomials ($G_1(X) = 1 + X^3 + X^{10}$ and $G_2(X) = 1 + X^2 + X^3 + X^6 + X^8 + X^9 + X^{10}$), 292-bit subframe format, Qualcomm CRC-24Q (`0x1864CFB`), and dual-band L5 ($1176.45\text{ MHz}$) / S-band ($2492.028\text{ MHz}$) ionospheric correction must strictly conform to official ISRO specifications.

4. **Deterministic Testing**:
   - Every feature or bug fix must be accompanied by deterministic unit and integration test assertions in `tests/test_runner.cpp`.
   - The test suite must pass 100% cleanly with zero memory leaks and exit code `0`.

---

## Development Workflow

### 1. Build the Binary & Run Test Suite
```bash
# Clean previous builds
make clean

# Compile CLI binary (bin/lh-navic) and test runner (tests/test_runner)
make all

# Execute the test suite
make test
```

### 2. Verify with Clang++ (if installed)
```bash
make clean
make CXX=clang++ all test
```

### 3. Verify Local CLI Commands
```bash
./bin/lh-navic --version
./bin/lh-navic --help
./bin/lh-navic scan fixtures/hardened_navic_policy.json --format table
./bin/lh-navic gen-prn --prn 1
./bin/lh-navic decode-subframe fixtures/sample_navic_subframes.json
./bin/lh-navic calc-iono --rho-l5 22500018.40 --rho-s 22500004.10
```

---

## Pull Request Guidelines

1. Ensure all new code adheres to the 10-rule posture framework (`LH-NAVIC-001` through `LH-NAVIC-010`).
2. Verify that `make test` executes with zero failures and that code conforms to `pedantic` C++17 standards.
3. Keep commits focused, well-documented, and signed off.
