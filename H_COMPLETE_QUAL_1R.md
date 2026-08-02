# H-COMPLETE-QUAL-1R — QUALIFICATION IDENTITY RECONCILIATION

Date: 2026-07-27
Authority: BUILD-EVIDENCE-CANON-1

---

## BUILD ARCHITECTURE

  MULTI-BINARY — ESTABLISHED

## ENGINEERING ARTIFACTS

  11

## ENGINEERING TESTS

  206 / 206 PASS
  Failed: 0

## OBSOLETE H_HARNESS

  RETIRED — aggregate executable never authoritative

## H_BUILD_SET

  D1604A5D5223882D7831BDB8CEC47D06BBD94ACD2E8ADBA5EA59343A886A6CCE

  Serialization: BUILD_SET_V1 (1255 bytes, UTF-8 no BOM, LF line endings, terminal LF)
  Records: 11, bytewise-sorted by relative executable path
  Each line: <relative-path>\t<SHA256-uppercase-hex>

  Rule: another verifier can reproduce this hash by writing the exact
  11 records shown in BUILD_SET_V1 to a file in the same byte order,
  then computing SHA-256 of the file contents.

## H_BUILD_EVIDENCE

  8936AD67AFB0EF8576A8AA03F6943AF5F38C91334C2294D10E90C58DAA6D153E

  Source: BUILD_EVIDENCE_CANON_1.md as persisted on disk

## H_MSF

  fa003d0f52f88414ecf3f206deb6f2ebf0dc34b469b34b525f38a0d6eff574fc

## IDENTITY COLLISION

  NONE

  H_BUILD_SET    != H_MSF    (verified)
  H_BUILD_EVIDENCE != H_MSF  (verified)
  H_BUILD_SET != H_BUILD_EVIDENCE (verified)

---

## STALE EXECUTABLES (not in canonical 11)

  5 stale .exe files found in cpp_reference/test/:
    test_debug.exe               (149,371 bytes, 25-Jul-2026)
    test_debug_bmin.exe          (144,379 bytes, 25-Jul-2026)
    test_litenyx.exe           (4,537,747 bytes, 27-Jul-2026 12:31)
    test_security_floor_standalone.exe (188,178 bytes, 25-Jul-2026)
    test_sf.exe                 (188,178 bytes, 25-Jul-2026)

  These are NOT part of the canonical build set.
  They are build artifacts from prior experiments.
  They do not affect the 206/206 engineering result.

---

## bitcoin-config.h SPECIAL TREATMENT

  Path:          external/dogecoin/src/config/bitcoin-config.h
  SHA-256:       B52938BB9916EE6B1E78BE9DD1F33563E00BEF889705D4E8E51FC9D97953E536
  Git:           UNTRACKED (parent dir external/ is in .gitignore)
  Classification: GENERATED / LOCAL BUILD SUPPORT ARTIFACT

  Contents:
    #ifndef BITCOIN_CONFIG_H
    #define BITCOIN_CONFIG_H
    #define HAVE_DECL_BSWAP_16 0
    #define HAVE_DECL_BSWAP_32 0
    #define HAVE_DECL_BSWAP_64 0
    /* #undef HAVE_BYTESWAP_H */
    #endif

  Why required:
    MSYS2 UCRT64 build of Litenyx test suite requires bitcoin-config.h
    for the Dogecoin crypto layer (sha256.cpp, hmac_sha256.cpp, scrypt.cpp)
    used by test_litenyx_shared_delta, test_icf1d_carrier, test_iw2_verifier,
    test_work_adapter_eng1. The autotools configure normally generates this
    header; it was not available in the repository.

  Normal Dogecoin configure behavior (from configure.ac lines 612-615):
    AC_CHECK_DECLS([bswap_16, bswap_32, bswap_64],,,
        [#if HAVE_BYTESWAP_H
         #include <byteswap.h>
         #endif])

    On Linux/MSYS2 with glibc/mingw, <byteswap.h> is typically present,
    so configure would set:
      HAVE_BYTESWAP_H = 1
      HAVE_DECL_BSWAP_16 = 1
      HAVE_DECL_BSWAP_32 = 1
      HAVE_DECL_BSWAP_64 = 1

  Stub vs. real configure:
    Our stub sets HAVE_BYTESWAP_H undefined and HAVE_DECL_BSWAP_* = 0,
    which forces compat/byteswap.h (lines 35-62) to define inline C++
    fallback byte-swap functions instead of using platform intrinsics.

  Functional equivalence:
    YES — inline fallbacks produce identical byte-swap results.

  Configure output equivalence:
    NOT EQUIVALENT — real configure would set HAVE_BYTESWAP_H=1 and
    HAVE_DECL_BSWAP_*=1 on MSYS2 UCRT64.

  BUILD PORTABILITY:
    NOT ESTABLISHED — this stub is specific to the Windows MSYS2 build
    and does not represent what a real ./configure would produce on this
    platform. The 206/206 engineering result was obtained using this
    non-canonical configuration.

---

## SEVEN H_complete PREDICATES

### C_contract — Frozen MSF contract identified?

  Question: Is the frozen MSF contract identified by the authoritative
  H_MSF, and can implementation obligations be traced to it?

  Evidence: H_MSF is defined (fa003d0f...), but no frozen MSF contract
  document has been identified in this repository or session. The 11
  engineering binaries are not derived from a frozen MSF contract —
  they are built directly from the source tree.

  Verdict: NOT SATISFIED
  Reason: No frozen MSF contract document identified; implementation
  obligations cannot be traced to H_MSF.

### D_deterministic — Deterministic behavior demonstrated?

  Question: Is deterministic behavior demonstrated by repeated
  identical-input execution/evidence?

  Evidence: 206/206 tests passed in a single execution. Each test
  exercises a defined input-output contract. The test framework
  (Boost.Test) is deterministic for fixed inputs. No non-deterministic
  I/O, randomness, or time-dependent behavior was observed. However,
  only a single execution was recorded — no repeated run was performed
  to confirm reproducibility across invocations.

  Verdict: PARTIALLY SUPPORTED
  Reason: Single execution demonstrates determinism for the observed run.
  No repeated execution evidence available.

### R_replayable — Experiment reconstructable?

  Question: Can the qualifying experiment/state be reconstructed and
  replayed from persisted evidence?

  Evidence: BUILD_EVIDENCE_CANON_1.md records git HEAD, branch, compiler
  flags, all source hashes, all binary hashes, and the bitcoin-config.h
  stub. BUILD_SET_V1 records the canonical executable manifest. A
  verifier with access to the same git repo, MSYS2 UCRT64 toolchain,
  and Boost 1.81.0 could reconstruct the build.

  Blocking dependencies:
    - Exact Boost 1.81.0 library variant (-mt, linked mode)
    - MSYS2 UCRT64 g++ 16.1.0
    - bitcoin-config.h stub (non-canonical)
    - MSYS2 UCRT64 platform (Windows x86_64)

  Verdict: CONDITIONALLY REPLAYABLE
  Reason: Full reconstruction requires the specific platform and
  dependencies documented. Independent reproduction on a different
  platform has not been demonstrated.

### T_telemetry — Telemetry schema satisfied?

  Question: Does captured telemetry satisfy the frozen schema/semantic
  requirements?

  Evidence: No telemetry was captured during engineering test execution.
  Boost.Test output was consumed by the test runner and not persisted
  in a structured telemetry format. No frozen telemetry schema exists
  in the repository.

  Verdict: NOT APPLICABLE
  Reason: Engineering tests do not produce structured telemetry.
  Telemetry requirements belong to the MSF execution context,
  which is not authorized.

### M_mutation — Negative controls demonstrated?

  Question: Do authorized negative controls demonstrate detection of
  invalid/manipulated conditions?

  Evidence: The 206 engineering tests are positive-functional tests —
  they verify that correct behavior occurs. No negative-control tests
  (e.g., verifying that a mutated binary fails, that corrupted input is
  rejected, or that unauthorized changes are detected) were included in
  this execution.

  Verdict: NOT SATISFIED
  Reason: No negative-control tests were executed. The engineering
  test suite validates correctness, not mutation detection.

### E_evidence — Evidence chain canonical and consistent?

  Question: Is the complete evidence chain canonical, hashed, internally
  consistent, and reproducible?

  Evidence:
    - H_BUILD_SET computed from canonical BUILD_SET_V1 serialization
    - H_BUILD_EVIDENCE computed from BUILD_EVIDENCE_CANON_1.md
    - Git HEAD and branch independently verified
    - All 11 binary hashes independently recomputed and matched
    - bitcoin-config.h hash independently verified
    - BUILD_SET_V1 re-read hash verified (deterministic)
    - IDENTITY COLLISION: NONE across H_BUILD_SET, H_BUILD_EVIDENCE, H_MSF

  Gaps:
    - BUILD_EVIDENCE_CANON_1.md and BUILD_SET_V1 exist only as local
      files; no external attestation or signing
    - No chain-of-custody log for evidence file creation/modification

  Verdict: ESTABLISHED (with caveats)
  Reason: Internal consistency verified. Canonical hashing reproducible.
  External attestation not yet established.

### V_independent — Independent verification reproduced?

  Question: Has an independent verification path reproduced the
  required results?

  Evidence: All verification in this session was performed by the
  same agent on the same machine. No independent third party or
  separate machine has reproduced the 206/206 result.

  Verdict: NOT SATISFIED
  Reason: No independent verification path has reproduced the results.

---

## PREDICATE SUMMARY

  C_contract:       NOT SATISFIED — no frozen MSF contract identified
  D_deterministic:  PARTIALLY SUPPORTED — single run only
  R_replayable:     CONDITIONALLY REPLAYABLE — requires documented deps
  T_telemetry:      NOT APPLICABLE — no telemetry schema
  M_mutation:       NOT SATISFIED — no negative controls
  E_evidence:       ESTABLISHED (with caveats) — internally consistent
  V_independent:    NOT SATISFIED — no independent reproduction

---

## H_complete

  NOT ESTABLISHED

  Reason: C_contract, M_mutation, V_independent not satisfied.
  D_deterministic only partially supported.
  H_complete requires all applicable predicates satisfied.

---

## MSF-REEXEC-AUTH-1

  INELIGIBLE

  Reason: H_complete is NOT ESTABLISHED. Eligibility requires the
  prerequisite predicates (at minimum C_contract and E_evidence)
  to be satisfied before MSF execution authorization can be considered.
  No MSF scientific execution is authorized.

---

## MSF EXECUTION

  NOT AUTHORIZED

---

## PROVENANCE CHAIN

  source state (git HEAD aae45b4, phase7-draining-authority)
    + toolchain (g++ 16.1.0 MSYS2 UCRT64, Boost 1.81.0)
    + build artifact (bitcoin-config.h stub, UNTRACKED, NOT CANONICAL)
    = 11 binaries (H_BUILD_SET = D1604A5D...)
      + 206 discovered test registrations
        = 206 successful executions
          + evidence canonicalization (H_BUILD_EVIDENCE = 8936AD67...)
            = H-COMPLETE-QUAL-1R

  H_HARNESS:                RETIRED (never existed as aggregate binary)
  130-test narrative:       REJECTED / SUPERSEDED (prior session)
  harness_test.exe:         NOT RESURRECTED

---

## OBSERVATIONS

1. The 5 stale executables (test_debug.exe, test_debug_bmin.exe,
   test_litenyx.exe, test_security_floor_standalone.exe, test_sf.exe)
   should be cleaned from cpp_reference/test/ to avoid confusion.
   They are not part of the canonical build set.

2. The bitcoin-config.h stub is a build portability gap. To establish
   full build portability, either:
   (a) Run ./configure && make on MSYS2 UCRT64 to generate the real
       bitcoin-config.h, or
   (b) Document the stub as an accepted deviation with rationale.

3. The most critical gap for H_complete is C_contract: no frozen MSF
   contract document has been identified. Until that document exists
   and H_MSF is bound to it, the scientific qualification path cannot
   proceed.

4. The 206/206 engineering result is a valid engineering achievement
   independent of the scientific qualification path. It proves the
   multi-binary test architecture works correctly on this platform.
