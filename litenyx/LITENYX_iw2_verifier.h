// IW2 Verifier: ValidParentWorkCarrier_G
//
// Implements the four-predicate decomposition:
//   ValidParentWorkCarrier_G(P_G, B_Y, T_Y) =
//     DomainBound_G(P_G)
//     ∧ CommitmentValid_G(P_G, B_Y)
//     ∧ WorkValid_G(P_G, T_Y)
//     ∧ StructuralValid_G(P_G)
//
// Frozen carrier (ICF-1B CLOSED, ICF-1D RATIFIED):
//   H_G = v_8 ∥ D_{P,32LE} ∥ D_{N,32LE} ∥ R_{256} ∥ n_{64LE}
//   |H_G| = 49 bytes
//
// Commitment proof language (ICF-1C CLOSED):
//   L_Y = H(TAG_L ∥ H(B_Y))
//   N(a,b) = H(TAG_N ∥ a ∥ b)
//   π_Y = (i, s_0, ..., s_{d_max-1})
//   Verify: z_0 = L_Y, z_{j+1} = N(z_j or s_j, ...), accept iff z_{d_max} = R

#pragma once

#include <litenyx/LITENYX_galactic_carrier.h>
#include <crypto/sha256.h>
#include <crypto/scrypt.h>

#include <cstdint>
#include <cstring>
#include <array>
#include <vector>
#include <functional>
#include <algorithm>

namespace galactic {

// ============================================================================
// SHA-256 helper: single-pass H(data)
// ============================================================================
inline void SHA256_SinglePass(const uint8_t* data, size_t len, uint8_t out[32]) {
    CSHA256 ctx;
    ctx.Write(data, len);
    ctx.Finalize(out);
}

// ============================================================================
// Predicate result structure
// ============================================================================
struct PredicateResult {
    bool pass;
    const char* predicate;
    const char* reason;
};

// ============================================================================
// BB2: DomainBound_G
//
// DomainBound_G(P_G) ⟺
//     ProtocolDomain(P_G) = D_P
//     ∧ NetworkDomain(P_G) = D_N
//
// D_P and D_N are specification constants, not cryptographic identities.
// They are covered by the physical work hash (Scrypt input ⊃ D_P ∥ D_N).
// ============================================================================

inline PredicateResult DomainBound_G(
    const GalacticWorkHeader& hdr,
    uint32_t expected_protocol,
    uint32_t expected_network)
{
    if (hdr.protocolDomain != expected_protocol)
        return {false, "DomainBound_G", "ProtocolDomain mismatch"};
    if (hdr.networkDomain != expected_network)
        return {false, "DomainBound_G", "NetworkDomain mismatch"};
    return {true, "DomainBound_G", "OK"};
}

// ============================================================================
// BB3: CommitmentValid_G
//
// CommitmentValid_G(P_G, B_Y) ⟺
//     ∃ π_Y : VerifyInclusion(H(B_Y), π_Y, R) = TRUE
//
// ICF-1C proof language:
//   Leaf:  L_Y = H(TAG_L ∥ H(B_Y))
//   Node:  N(a,b) = H(TAG_N ∥ a ∥ b)  — ordered
//   Proof: π_Y = (i, s_0, ..., s_{d_max-1})
//   Verify: z_0 = L_Y, z_{j+1} = bit_j(i)=0 ? N(z_j, s_j) : N(s_j, z_j)
//   Accept iff z_{d_max} = R
//
// d_max = log2(Capacity_v). For v=1, Capacity is specified by the commitment
// algorithm C_v. Here we parameterize d_max for generality.
// ============================================================================

// Inclusion proof for a single leaf
struct InclusionProof {
    uint64_t leaf_index;      // i — position in the tree
    std::vector<std::array<uint8_t, 32>> sibling_hashes; // s_0, ..., s_{d_max-1}
};

// Compute leaf hash: L_Y = H(TAG_L ∥ H(B_Y))
inline void ComputeLeafHash(const uint8_t block_hash[32], uint8_t leaf_out[32]) {
    // H(B_Y) first, then TAG_L ∥ H(B_Y)
    uint8_t inner[32];
    // B_Y is already a hash — use it directly
    // L_Y = H(TAG_L ∥ B_Y)
    CSHA256 ctx;
    ctx.Write(galactic::GW_TAG_LEAF, galactic::GW_TAG_L_SIZE);
    ctx.Write(block_hash, 32);
    ctx.Finalize(leaf_out);
}

// Compute node hash: N(a,b) = H(TAG_N ∥ a ∥ b) — ordered
inline void ComputeNodeHash(const uint8_t left[32], const uint8_t right[32], uint8_t node_out[32]) {
    CSHA256 ctx;
    ctx.Write(galactic::GW_TAG_NODE, galactic::GW_TAG_N_SIZE);
    ctx.Write(left, 32);
    ctx.Write(right, 32);
    ctx.Finalize(node_out);
}

// Verify inclusion proof against commitment root R
inline PredicateResult CommitmentValid_G(
    const uint8_t commitment_root[32],
    const uint8_t block_hash[32],
    const InclusionProof& proof,
    size_t d_max)
{
    // Validate proof structure
    if (proof.sibling_hashes.size() != d_max)
        return {false, "CommitmentValid_G", "Proof length != d_max"};

    // z_0 = L_Y
    uint8_t z[32];
    ComputeLeafHash(block_hash, z);

    // z_{j+1} = bit_j(i)=0 ? N(z_j, s_j) : N(s_j, z_j)
    for (size_t j = 0; j < d_max; ++j) {
        bool bit_j = (proof.leaf_index >> j) & 1;
        uint8_t next[32];
        if (!bit_j) {
            ComputeNodeHash(z, proof.sibling_hashes[j].data(), next);
        } else {
            ComputeNodeHash(proof.sibling_hashes[j].data(), z, next);
        }
        std::memcpy(z, next, 32);
    }

    // Accept iff z_{d_max} = R
    if (std::memcmp(z, commitment_root, 32) != 0)
        return {false, "CommitmentValid_G", "Root mismatch after proof verification"};

    return {true, "CommitmentValid_G", "OK"};
}

// ============================================================================
// BB4: WorkValid_G
//
// WorkValid_G(Header_G, T_Y) ⟺
//     ∃ w_G : w_G = Scrypt(Header_G) ∧ w_G ≤ T_Y
//
// Uses the canonical 49-byte carrier. The 80-byte ASIC adapter is NOT
// a consensus serialization — it is a mining interface concern only.
// Work is validated against child target T_Y only.
// ============================================================================

// Compute Scrypt work hash from canonical 49-byte carrier
inline void ComputeWorkHash(const uint8_t header_g[49], uint8_t work_out[32]) {
    // scrypt_1024_1_1_256 takes 80-byte input. The mining adapter embeds
    // the 49-byte carrier into bytes [0..48] of an 80-byte buffer.
    // Bytes [49..79] are adapter-provided (not consensus-relevant).
    uint8_t input80[80] = {};
    std::memcpy(input80, header_g, 49);
    // Bytes [49..79] = 0 for testing; adapter fills these in production.
    scrypt_1024_1_1_256((const char*)input80, (char*)work_out);
}

// Compare work hash against target (256-bit big-endian comparison)
// w ≤ T  ⟺  w is lexicographically ≤ T in big-endian byte order
inline bool WorkHashLeTarget(const uint8_t work[32], const uint8_t target[32]) {
    for (int i = 0; i < 32; ++i) {
        if (work[i] < target[i]) return true;
        if (work[i] > target[i]) return false;
    }
    return true; // equal
}

inline PredicateResult WorkValid_G(
    const uint8_t header_g[49],
    const uint8_t target_y[32])
{
    uint8_t work[32];
    ComputeWorkHash(header_g, work);

    if (!WorkHashLeTarget(work, target_y))
        return {false, "WorkValid_G", "Scrypt(header) > target"};

    return {true, "WorkValid_G", "OK"};
}

// ============================================================================
// BB5: StructuralValid_G
//
// StructuralValid_G(P_G) =
//     Canonical(P_G) ∧ Bounded(P_G) ∧ WellFormed(P_G)
//
// Canonical:  exactly one interpretation for 49 bytes
// Bounded:    version = 1, field sizes match, d_max consistent
// WellFormed: all static_asserts pass, no gaps, no overlaps
// ============================================================================

inline PredicateResult StructuralValid_G(const GalacticWorkHeader& hdr) {
    // Canonical: version must be exactly 1
    if (hdr.version != GW_VERSION_V1)
        return {false, "StructuralValid_G", "version != 1"};

    // Bounded: protocol domain must be non-zero (a domain constant exists)
    if (hdr.protocolDomain == 0)
        return {false, "StructuralValid_G", "protocolDomain == 0"};

    // Bounded: network domain must be non-zero
    if (hdr.networkDomain == 0)
        return {false, "StructuralValid_G", "networkDomain == 0"};

    // WellFormed: commitment root is opaque (any 32-byte value is valid)
    // No additional structural constraints on root content.
    // The root's validity is checked by CommitmentValid_G.

    return {true, "StructuralValid_G", "OK"};
}

// ============================================================================
// BB6: Recompose ValidParentWorkCarrier_G
//
// ValidParentWorkCarrier_G(P_G, B_Y, T_Y) =
//     DomainBound_G(P_G)
//     ∧ CommitmentValid_G(P_G, B_Y)
//     ∧ WorkValid_G(P_G, T_Y)
//     ∧ StructuralValid_G(P_G)
//
// All four predicates must pass. No short-circuit ordering.
// The conjunction is evaluated predicate-by-predicate; all results
// are collected before the final verdict.
// ============================================================================

struct VerificationResult {
    PredicateResult domain;
    PredicateResult commitment;
    PredicateResult work;
    PredicateResult structural;
    bool valid;

    // Map to canonical failure identifier
    const char* first_failure() const {
        if (!domain.pass) return "DomainBound_G";
        if (!structural.pass) return "StructuralValid_G";
        if (!commitment.pass) return "CommitmentValid_G";
        if (!work.pass) return "WorkValid_G";
        return nullptr; // all pass
    }
};

inline VerificationResult ValidParentWorkCarrier_G(
    const GalacticWorkHeader& hdr,
    uint32_t expected_protocol,
    uint32_t expected_network,
    const uint8_t block_hash[32],
    const InclusionProof& proof,
    size_t d_max,
    const uint8_t target_y[32])
{
    VerificationResult r;
    r.domain = DomainBound_G(hdr, expected_protocol, expected_network);
    r.structural = StructuralValid_G(hdr);
    r.commitment = CommitmentValid_G(hdr.commitmentRoot, block_hash, proof, d_max);
    r.work = WorkValid_G(reinterpret_cast<const uint8_t*>(&hdr), target_y);
    r.valid = r.domain.pass && r.structural.pass && r.commitment.pass && r.work.pass;
    return r;
}

} // namespace galactic
