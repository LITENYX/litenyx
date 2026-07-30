# BUILD-EVIDENCE-CANON-1: Canonical Build/Test Evidence

Date: 2026-07-27
Git HEAD: aae45b40d1cf292e23cb5b96432fbf001f62c14c
Branch: phase7-draining-authority
Toolchain: g++ 16.1.0 MSYS2 UCRT64 / Boost 1.81.0 linked-mode

## Build Flags (all 11 targets)
  g++ -std=c++20 -O0 -DKERRNYX_STANDALONE_TEST -DBOOST_TEST_DYN_LINK -mconsole
  -I. -I./cpp_reference -I./external/dogecoin/src -I./external/dogecoin/src/config
  <source> <extra-objects> -o <name>.exe -lboost_unit_test_framework-mt -lpthread

---

## bitcoin-config.h Stub

  Path:      external/dogecoin/src/config/bitcoin-config.h
  SHA-256:   B52938BB9916EE6B1E78BE9DD1F33563E00BEF889705D4E8E51FC9D97953E536
  Git:       UNTRACKED (build artifact)
  Required:  Dogecoin autotools configure normally generates this header.
             MSYS2 UCRT64 lacks <byteswap.h>; fallback inline byte-swap
             functions require HAVE_BYTESWAP_H undefined and
             HAVE_DECL_BSWAP_16/32/64 set to 0.
  Canonical: NO — must not silently become canonical source.

---

## Executable Inventory (11 binaries, 206 tests, 206/206 PASS)

| #  | Executable                          | Tests | Binary Size | SHA-256 (binary)                              | SHA-256 (source)                              |
|----|--------------------------------------|-------|-------------|-----------------------------------------------|-----------------------------------------------|
| 1  | test_security_floor_golden           | 12    | 316,106     | EE872D62DDE115DC9A8CD7730EAD349ADFE33C08806070E13147EB7743373D70 | 8C204404AFBADFA5A08811B6D6BC676F7F5E4B495E19D73609973E7F6F338EC3 |
| 2  | test_litenyx_topology                | 5     | 508,884     | A695346131087EABB0A7DB0674A60595CEC842229C4EB9917F15C8A00898B8D6 | 0CC617B8138E277A8DA475145343966BABE6B5C760A3B13842ACFE9C8F11787A |
| 3  | test_litenyx_topology_authority      | 22    | 759,645     | CD6D9BAEECD6CA9AAEC98FDBBA5AE1A3BA6C17CDC49D130F2EB42DDB0550654E | 875BD85CD5949800E5AEB3FD531D3377584484772CA8AC1B2424EC301604310C |
| 4  | test_litenyx_chainid_lifecycle       | 12    | 627,629     | A17AB1A27F11A12F61E559F5F135E17CE3DCA0D9CFF7FCA6F83DDBE40152E20F | E3FA464A93050CC7E0194FD9F650612F5BA2ED9A14A9D51F37A9BDFE3CA8756D |
| 5  | test_litenyx_v3_carrier              | 5     | 392,064     | 460EA2D12E55660C07B080BED7C591906FEE6C9484BE26EA0979BDD8138C5899 | 424325201E476193BDD1426BB3DD018C50D1048C349CD691159BB63B01FEC836 |
| 6  | test_litenyx_execution_authority     | 17    | 390,524     | ECD49A622B41F422C22DA7453A3EF5FCEDFBC6C589D58AB400B8E41EB489A0DA | BF92745562E3FDB7C035AF653A7547510D3083E6F294CC666795E2FED41EEC9A |
| 7  | test_litenyx_draining_authority      | 18    | 431,661     | A3778A36B41D1DCF07C19E0A8B05DE33A6E5D803D5B78C19A0BDD705C8EBEFB5 | 0C3E30E349D8790525D740A62D748924216537924D19D90DD38D745C7188FE6A |
| 8  | test_litenyx_shared_delta            | 6     | 694,723     | 7618866CF4A26B2F1EE33EDF4A5BBA3683E1707C70439DF09659B389FB6AA565 | D36E34060B1B2881D4F014484456DD242AF5DC7A1719D770379FA39F0BDDAC93 |
| 9  | test_icf1d_carrier                   | 24    | 711,041     | 5FE9F4CC1E7F92E60C3F3DB7040914C9AE06875EDC60FF8149974FF7FD4BD6D4 | 15A16130F5579F8C72770DDA8270FEB4714D4C095A87B1AA4AFAB3353AB78F23 |
| 10 | test_iw2_verifier                    | 36    | 674,395     | FA41319004BE14821AEA2BD67924F5D9D81CD78DCDC08FF6CFE03298348EB6DD | 301DF3433D0EB70DE4478664E651667851BFABBE5D3ABD6A6CA4DDE49D94120B |
| 11 | test_work_adapter_eng1               | 49    | 5,084,536   | 1E6256B299FE05BE61F996595295D28B0C674D8A0DFAF266FE1B8EA71F925AA1 | 5DA38EEC6DD0E0C843684CA67833EB455D1B729AC544BCA7964462894C7DE90F |

TOTAL: 11 binaries | 206 test registrations | 206/206 PASS | 0 FAIL

---

## Reproducibility Chain

  source state (git HEAD aae45b4)
    + compiler/configuration (g++ 16.1.0, build flags above)
    + build artifact (bitcoin-config.h stub)
    = 11 binaries (hashes above)
      + 206 discovered test registrations
        = 206 successful executions

---

## Status

  harness_test.exe:            NOT REPOSRCE — rejected by multi-binary evidence
  130-test aggregate narrative: REJECTED / SUPERSEDED
  206-test engineering suite:  VERIFIED
  S1 tests (69 registrations): Separate jurisdiction, NOT included in this 206
  H_complete:                  NOT ESTABLISHED
  MSF execution:               NOT AUTHORIZED
