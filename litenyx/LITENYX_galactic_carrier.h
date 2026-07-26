// ICF-1D Evidence: Galactic Work Carrier 49-byte header encoder/decoder
//
// Frozen candidate (d9e48c3):
//   H_G = v_8 ∥ D_{P,32LE} ∥ D_{N,32LE} ∥ R_{256} ∥ n_{64LE}
//   |H_G| = 49 bytes
//
// This file implements the canonical encoder/decoder and M1-M6 evidence tests.
// The carrier format is a consensus-level frozen candidate, not yet finalized ABI.
// Finalization depends on ICF-1D evidence → ICF-1B closure.

#pragma once

#include <cstdint>
#include <cstring>
#include <array>
#include <vector>
#include <algorithm>
#include <string>

namespace galactic {

// Frozen constants (ICF-1A / IW2A)
// Protocol domain: Trunc_32(H("Liten-Galactic-Work-Domain"))
// Network domain: Trunc_32(H("Liten-<NetworkLabel>"))
// These are specification constants, not cryptographic identities.

// Format version
constexpr uint8_t  GW_VERSION_V1       = 0x01;

// Domain identifiers (specification constants, 4 bytes each)
// Placeholder values — real values derived from H(canonical labels)
constexpr uint32_t GW_PROTOCOL_DOMAIN  = 0x4C475744; // "LGWD" placeholder
constexpr uint32_t GW_NETWORK_MAINNET  = 0x4C4D4E4E; // "LMNN" placeholder
constexpr uint32_t GW_NETWORK_TESTNET  = 0x4C544E4E; // "LTNN" placeholder

// Header layout constants (ICF-1B)
constexpr size_t GW_HEADER_SIZE        = 49;
constexpr size_t GW_OFFSET_VERSION     = 0;
constexpr size_t GW_OFFSET_PROTOCOL    = 1;
constexpr size_t GW_OFFSET_NETWORK     = 5;
constexpr size_t GW_OFFSET_ROOT        = 9;
constexpr size_t GW_OFFSET_NONCE       = 41;
constexpr size_t GW_SIZE_VERSION       = 1;
constexpr size_t GW_SIZE_PROTOCOL      = 4;
constexpr size_t GW_SIZE_NETWORK       = 4;
constexpr size_t GW_SIZE_ROOT          = 32;
constexpr size_t GW_SIZE_NONCE         = 8;

// Commitment tree constants (ICF-1C)
constexpr size_t GW_TAG_L_SIZE         = 4;
constexpr size_t GW_TAG_N_SIZE         = 4;
constexpr size_t GW_EMPTY_LEAF_SIZE    = 32;

// Domain-separated tags (LeafDomain ≠ NodeDomain)
// These are specification constants.
static const uint8_t GW_TAG_LEAF[GW_TAG_L_SIZE] = {0x4C, 0x69, 0x74, 0x4C}; // "LitL"
static const uint8_t GW_TAG_NODE[GW_TAG_N_SIZE] = {0x4C, 0x69, 0x74, 0x4E}; // "LitN"

// Default empty leaf: H(TAG_L ∥ EMPTY)
// Computed once at startup or hardcoded after first computation.

// Galactic Work Carrier header (49 bytes, frozen candidate)
#pragma pack(push, 1)
struct GalacticWorkHeader {
    uint8_t  version;        // [0]     1 byte  — format version
    uint32_t protocolDomain; // [1]     4 bytes — protocol domain (LE)
    uint32_t networkDomain;  // [5]     4 bytes — network domain (LE)
    uint8_t  commitmentRoot[32]; // [9]  32 bytes — commitment root
    uint64_t nonce;          // [41]    8 bytes — work search field (LE)
};
#pragma pack(pop)

static_assert(sizeof(GalacticWorkHeader) == GW_HEADER_SIZE,
    "GalacticWorkHeader must be exactly 49 bytes");
static_assert(offsetof(GalacticWorkHeader, version) == GW_OFFSET_VERSION,
    "version at offset 0");
static_assert(offsetof(GalacticWorkHeader, protocolDomain) == GW_OFFSET_PROTOCOL,
    "protocolDomain at offset 1");
static_assert(offsetof(GalacticWorkHeader, networkDomain) == GW_OFFSET_NETWORK,
    "networkDomain at offset 5");
static_assert(offsetof(GalacticWorkHeader, commitmentRoot) == GW_OFFSET_ROOT,
    "commitmentRoot at offset 9");
static_assert(offsetof(GalacticWorkHeader, nonce) == GW_OFFSET_NONCE,
    "nonce at offset 41");

// Encode: semantic tuple → 49-byte canonical serialization
inline std::array<uint8_t, GW_HEADER_SIZE> EncodeGW(
    uint8_t version,
    uint32_t protocolDomain,
    uint32_t networkDomain,
    const uint8_t root[32],
    uint64_t nonce)
{
    std::array<uint8_t, GW_HEADER_SIZE> out{};
    out[GW_OFFSET_VERSION] = version;
    // Little-endian encoding for all integer fields
    out[GW_OFFSET_PROTOCOL + 0] = (protocolDomain >>  0) & 0xFF;
    out[GW_OFFSET_PROTOCOL + 1] = (protocolDomain >>  8) & 0xFF;
    out[GW_OFFSET_PROTOCOL + 2] = (protocolDomain >> 16) & 0xFF;
    out[GW_OFFSET_PROTOCOL + 3] = (protocolDomain >> 24) & 0xFF;
    out[GW_OFFSET_NETWORK + 0] = (networkDomain >>  0) & 0xFF;
    out[GW_OFFSET_NETWORK + 1] = (networkDomain >>  8) & 0xFF;
    out[GW_OFFSET_NETWORK + 2] = (networkDomain >> 16) & 0xFF;
    out[GW_OFFSET_NETWORK + 3] = (networkDomain >> 24) & 0xFF;
    std::memcpy(&out[GW_OFFSET_ROOT], root, 32);
    out[GW_OFFSET_NONCE + 0] = (nonce >>  0) & 0xFF;
    out[GW_OFFSET_NONCE + 1] = (nonce >>  8) & 0xFF;
    out[GW_OFFSET_NONCE + 2] = (nonce >> 16) & 0xFF;
    out[GW_OFFSET_NONCE + 3] = (nonce >> 24) & 0xFF;
    out[GW_OFFSET_NONCE + 4] = (nonce >> 32) & 0xFF;
    out[GW_OFFSET_NONCE + 5] = (nonce >> 40) & 0xFF;
    out[GW_OFFSET_NONCE + 6] = (nonce >> 48) & 0xFF;
    out[GW_OFFSET_NONCE + 7] = (nonce >> 56) & 0xFF;
    return out;
}

// Decode: 49-byte canonical serialization → semantic tuple
inline bool DecodeGW(
    const uint8_t data[GW_HEADER_SIZE],
    uint8_t& version,
    uint32_t& protocolDomain,
    uint32_t& networkDomain,
    uint8_t root[32],
    uint64_t& nonce)
{
    version = data[GW_OFFSET_VERSION];
    protocolDomain = 0;
    protocolDomain |= (uint32_t)data[GW_OFFSET_PROTOCOL + 0] <<  0;
    protocolDomain |= (uint32_t)data[GW_OFFSET_PROTOCOL + 1] <<  8;
    protocolDomain |= (uint32_t)data[GW_OFFSET_PROTOCOL + 2] << 16;
    protocolDomain |= (uint32_t)data[GW_OFFSET_PROTOCOL + 3] << 24;
    networkDomain = 0;
    networkDomain |= (uint32_t)data[GW_OFFSET_NETWORK + 0] <<  0;
    networkDomain |= (uint32_t)data[GW_OFFSET_NETWORK + 1] <<  8;
    networkDomain |= (uint32_t)data[GW_OFFSET_NETWORK + 2] << 16;
    networkDomain |= (uint32_t)data[GW_OFFSET_NETWORK + 3] << 24;
    std::memcpy(root, &data[GW_OFFSET_ROOT], 32);
    nonce = 0;
    nonce |= (uint64_t)data[GW_OFFSET_NONCE + 0] <<  0;
    nonce |= (uint64_t)data[GW_OFFSET_NONCE + 1] <<  8;
    nonce |= (uint64_t)data[GW_OFFSET_NONCE + 2] << 16;
    nonce |= (uint64_t)data[GW_OFFSET_NONCE + 3] << 24;
    nonce |= (uint64_t)data[GW_OFFSET_NONCE + 4] << 32;
    nonce |= (uint64_t)data[GW_OFFSET_NONCE + 5] << 40;
    nonce |= (uint64_t)data[GW_OFFSET_NONCE + 6] << 48;
    nonce |= (uint64_t)data[GW_OFFSET_NONCE + 7] << 56;
    return true; // Fixed-width decode always succeeds on 49 bytes (E3: FullConsumption)
}

// Canonical encoding identity (E1/E2/E7):
// Encode(Decode(data)) == data for any valid 49-byte input
inline bool RoundTripTest(const uint8_t data[GW_HEADER_SIZE]) {
    uint8_t ver, root[32];
    uint32_t dp, dn;
    uint64_t n;
    DecodeGW(data, ver, dp, dn, root, n);
    auto reencoded = EncodeGW(ver, dp, dn, root, n);
    return std::memcmp(data, reencoded.data(), GW_HEADER_SIZE) == 0;
}

// Hex encoding utility
inline std::string HexOf(const uint8_t* p, size_t n) {
    static const char* d = "0123456789abcdef";
    std::string s; s.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) { s += d[p[i] >> 4]; s += d[p[i] & 0xF]; }
    return s;
}

} // namespace galactic
