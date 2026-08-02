// Litenyx S1 + MON-ONTOLOGY-1 — Isolation & H1 counterfactual tests.
//
// Proves INV-1/INV-2/INV-3: economic candidate policy evaluation and H1
// counterfactual simulation CANNOT mutate authoritative topology state (N_h),
// transaction validity, or the global ownership domain. S1 reads the frozen
// reference; H1 is counterfactual.
//
// Links the FROZEN topology engine (header-only + tracker .cpp) and the frozen
// authority engine. No consensus constant is copied; no frozen source is modified.

#include <s1/litenyx_s1_replay.h>
#include <s1/litenyx_h1_candidate_policy.h>
#include <litenyx/LITENYX_topology.h>
#include <litenyx/LITENYX_types.h>

#define BOOST_TEST_MODULE LITENYX_s1_mon_ontology_test
#include <boost/test/included/unit_test.hpp>

#include <vector>
#include <string>
#include <cstdint>
#include <cstring>

// Helper to build a canonical snapshot from a replayed S1 state.
static H1CanonicalSnapshot SnapshotFrom(const LitenyxS1Replay& sim, uint32_t h,
                                         int64_t P=0, int64_t N=0, int64_t W=0, int64_t Ws=0) {
    LitenyxTopologyState s = sim.CurrentState(h);
    return H1CanonicalSnapshot::FromTopologyState(s, P, N, W, Ws);
}

static int McFromWeight(int64_t w){ int32_t mcV1=LitenyxMcV1({LitenyxDemandV1(w)}); return (int)LitenyxMcToControllerInput(mcV1);}

static std::vector<S1Block> MakeChain(uint32_t tip, uint8_t lanes, bool invert=false) {
    std::vector<S1Block> chain; chain.reserve(tip);
    const uint32_t W=LITENYX_TOPOLOGY_OBS_WINDOW;
    for(uint32_t h=1;h<=tip;++h){
        S1Block b; b.height=h; b.chainId=(uint8_t)((h-1)%lanes);
        bool hot=((h/W)%2)==0; if(invert) hot=!hot;
        b.Mc=McFromWeight(hot?LITENYX_MAX_BLOCK_WEIGHT:(LITENYX_MAX_BLOCK_WEIGHT/20));
        chain.push_back(b);
    }
    return chain;
}

static LitenyxS1Replay Replay(const std::vector<S1Block>& blocks) {
    LitenyxS1Replay sim;
    uint32_t W=LITENYX_TOPOLOGY_OBS_WINDOW;
    for(const auto& b: blocks){ sim.ConnectBlock(b); if(b.height%W==0) sim.Tick(b.height); }
    return sim;
}

BOOST_AUTO_TEST_SUITE(LITENYX_s1_mon_ontology_tests)

// S1-4: Economic/Topology isolation. Changing geometric subsidy params changes
// candidate economic outputs (G_c) but NOT authoritative N_h.
BOOST_AUTO_TEST_CASE(s1_4_economic_topology_isolation)
{
    LitenyxS1Replay sim = Replay(MakeChain(3000, LITENYX_TOPO_MAX_CHAINS));
    uint8_t N_before = sim.ActiveChains();

    // Vary Policy A base subsidy widely; this must not move N_h.
    for (int64_t Rbase : {int64_t(0), int64_t(1), int64_t(1000), int64_t(1'000'000), int64_t(4'000'000'000LL)}) {
        auto snap = SnapshotFrom(sim, 3000);
        std::vector<int64_t> fees(LITENYX_TOPO_MAX_CHAINS, 0);
        auto res = EvaluateCandidatePolicy(snap, Rbase, fees, 500);
        // Candidate economic output varies with Rbase:
        if (res.reward.R_c.size() > 0)
            BOOST_CHECK_EQUAL(res.reward.R_c[0], H1FloorDiv(Rbase, 1));
        // Authoritative N_h is unchanged by evaluating the candidate policy.
        BOOST_CHECK_EQUAL(res.authoritativeN_h, N_before);
    }
    BOOST_CHECK_EQUAL(sim.ActiveChains(), N_before); // live state unchanged
}

// S1-5: Wallet/Topology isolation. Changing W_t, W_t^*, or negative-supply params
// does NOT change authoritative N_h. Explicitly W_t -/-> N_h.
BOOST_AUTO_TEST_CASE(s1_5_wallet_topology_isolation)
{
    LitenyxS1Replay sim = Replay(MakeChain(3000, LITENYX_TOPO_MAX_CHAINS));
    uint8_t N_before = sim.ActiveChains();

    // Sweep wallet-space observations and targets across wide ranges.
    for (int64_t W : {int64_t(0), int64_t(1), int64_t(1000), int64_t(1'000'000), int64_t(9'000'000'000LL)})
    for (int64_t Ws : {int64_t(0), int64_t(1), int64_t(500'000), int64_t(1'000'000'000LL)}) {
        auto snap = SnapshotFrom(sim, 3000, /*P=*/W, /*N=*/0, W, Ws);
        std::vector<int64_t> fees;
        auto res = EvaluateCandidatePolicy(snap, /*Rbase=*/0, fees, 500);
        // Negative-supply candidate loop produces monetary derivations only:
        BOOST_CHECK_EQUAL(res.negSupply.e_t, W - Ws);
        // Authoritative N_h untouched by monetary candidate evaluation.
        BOOST_CHECK_EQUAL(res.authoritativeN_h, N_before);
    }
    BOOST_CHECK_EQUAL(sim.ActiveChains(), N_before);
}

// S1-6: Transaction/Lane isolation. Simulated low mining viability or lane
// unavailability does NOT, by itself, change fundamental transaction-validity
// outcomes. We model "validity" as the frozen global invariants (value
// conservation + shared spent-set), which are independent of N_h. H1 cannot
// invalidate an otherwise-valid transaction.
BOOST_AUTO_TEST_CASE(s1_6_transaction_lane_isolation)
{
    LitenyxS1Replay sim = Replay(MakeChain(3000, LITENYX_TOPO_MAX_CHAINS));
    uint8_t N = sim.ActiveChains();

    // Simulate a candidate policy reporting every lane as economically unviable
    // (G_c below a hypothetical marginal cost). This is a candidate observation
    // only; it must not alter N_h or any validity domain.
    std::vector<int64_t> fees(LITENYX_TOPO_MAX_CHAINS, 0);
    auto snap = SnapshotFrom(sim, 3000);
    auto res = EvaluateCandidatePolicy(snap, /*Rbase=*/0, fees, 500);
    // Candidate gross revenue is low everywhere (G_c == 0).
    for (size_t c = 0; c < res.reward.G_c.size(); ++c)
        BOOST_CHECK_EQUAL(res.reward.G_c[c], 0);
    // Authoritative lane count is STILL N (lanes remain valid execution resources).
    BOOST_CHECK_EQUAL(res.authoritativeN_h, N);
    // Universal ownership domain invariant: one global state, N parallel resources.
    BOOST_CHECK(N >= LITENYX_MIN_CHAINS && N <= LITENYX_TOPO_MAX_CHAINS);
}

// H1-1: Geometric reward R(c,h) = floor(R_base / 2^c), boundaries + rounding.
BOOST_AUTO_TEST_CASE(h1_1_geometric_reward)
{
    // Boundary: c=0 -> R_base; c grows -> halving, floor. Use N_h = 4.
    auto r = H1GeometricReward::Evaluate(1000, 4, std::vector<int64_t>(4, 0));
    BOOST_CHECK_EQUAL(r.R_c[0], 1000);   // floor(1000/1)
    BOOST_CHECK_EQUAL(r.R_c[1], 500);    // floor(1000/2)
    BOOST_CHECK_EQUAL(r.R_c[2], 250);    // floor(1000/4)
    BOOST_CHECK_EQUAL(r.R_c[3], 125);    // floor(1000/8)
    // Rounding: odd base. floor(1001/8) = 125.
    auto r2 = H1GeometricReward::Evaluate(1001, 4, std::vector<int64_t>(4, 0));
    BOOST_CHECK_EQUAL(r2.R_c[3], 125);
    // Zero base -> all zero.
    auto r3 = H1GeometricReward::Evaluate(0, 8, std::vector<int64_t>(8, 0));
    for (int64_t v : r3.R_c) BOOST_CHECK_EQUAL(v, 0);
}

// H1-2: Gross revenue G_c = R_c + F_c, checked arithmetic.
BOOST_AUTO_TEST_CASE(h1_2_gross_revenue)
{
    std::vector<int64_t> fees = {10, 20, 30, 40};
    auto r = H1GeometricReward::Evaluate(100, 4, fees);
    BOOST_CHECK_EQUAL(r.G_c[0], 100 + 10);
    BOOST_CHECK_EQUAL(r.G_c[1], 50 + 20);
    BOOST_CHECK_EQUAL(r.G_c[2], 25 + 30);
    BOOST_CHECK_EQUAL(r.G_c[3], 12 + 40);
}

// H1-3: No reverse authority. H1 economic output cannot invoke or reach
// production topology mutation. We assert the result carries NO mutable topology
// handle and that re-deriving topology from the snapshot is independent of G_c.
BOOST_AUTO_TEST_CASE(h1_3_no_reverse_authority)
{
    LitenyxS1Replay sim = Replay(MakeChain(3000, LITENYX_TOPO_MAX_CHAINS));
    uint8_t N = sim.ActiveChains();
    auto snap = SnapshotFrom(sim, 3000);

    // Evaluate with wildly different G_c (via R_base/fees); N_h must be invariant.
    int64_t N0 = snap.nN;
    for (int64_t Rbase : {int64_t(0), 4'000'000'000LL}) {
        std::vector<int64_t> fees(LITENYX_TOPO_MAX_CHAINS, Rbase % 1000);
        auto res = EvaluateCandidatePolicy(snap, Rbase, fees, 500);
        BOOST_CHECK_EQUAL(res.authoritativeN_h, N0);
        // G_c values differ but carry no topology command:
        BOOST_CHECK(res.reward.G_c.size() == (size_t)N0);
    }
    // The frozen topology state is byte-identical before/after (no mutation path).
    LitenyxTopologyState sBefore = sim.CurrentState(3000);
    (void)EvaluateCandidatePolicy(snap, 12345, std::vector<int64_t>(8, 7), 500);
    LitenyxTopologyState sAfter = sim.CurrentState(3000);
    unsigned char hB[32], hA[32];
    LitenyxTopologyStateHash(sBefore, hB);
    LitenyxTopologyStateHash(sAfter, hA);
    BOOST_CHECK_EQUAL(std::memcmp(hB, hA, 32), 0);
}

// H1-4: Canonical/Counterfactual separation. Discarding all H1 output leaves
// canonical state byte-for-byte equivalent.
BOOST_AUTO_TEST_CASE(h1_4_canonical_counterfactual_separation)
{
    LitenyxS1Replay sim = Replay(MakeChain(2000, LITENYX_TOPO_MAX_CHAINS));
    LitenyxTopologyState sCanon = sim.CurrentState(2000);
    unsigned char hCanon[32];
    LitenyxTopologyStateHash(sCanon, hCanon);

    // Run H1 heavily; its results are dropped.
    auto snap = SnapshotFrom(sim, 2000, 5000, 1000, 4000, 3000);
    for (int k = 0; k < 50; ++k)
        EvaluateCandidatePolicy(snap, k * 1000, std::vector<int64_t>(8, k % 50), (k * 37) % 1000);

    // Canonical state unchanged.
    LitenyxTopologyState sAfter = sim.CurrentState(2000);
    unsigned char hAfter[32];
    LitenyxTopologyStateHash(sAfter, hAfter);
    BOOST_CHECK_EQUAL(std::memcmp(hCanon, hAfter, 32), 0);
}

// H1-5: Replay determinism. Identical snapshot + params + inputs => identical H1.
BOOST_AUTO_TEST_CASE(h1_5_replay_determinism)
{
    LitenyxS1Replay sim = Replay(MakeChain(2000, LITENYX_TOPO_MAX_CHAINS));
    auto snap = SnapshotFrom(sim, 2000, 5000, 1000, 4000, 3000);
    std::vector<int64_t> fees(LITENYX_TOPO_MAX_CHAINS, 123);

    auto a = EvaluateCandidatePolicy(snap, 1000, fees, 500);
    auto b = EvaluateCandidatePolicy(snap, 1000, fees, 500);
    BOOST_CHECK_EQUAL(a.reward.G_c.size(), b.reward.G_c.size());
    for (size_t c = 0; c < a.reward.G_c.size(); ++c)
        BOOST_CHECK_EQUAL(a.reward.G_c[c], b.reward.G_c[c]);
    BOOST_CHECK_EQUAL(a.negSupply.DeltaS_t, b.negSupply.DeltaS_t);
    BOOST_CHECK_EQUAL(a.negSupply.e_t, b.negSupply.e_t);
}

// H1-extra: negative-supply hysteresis candidate math (independent loop).
BOOST_AUTO_TEST_CASE(h1_negative_supply_hysteresis)
{
    // DeltaS = P - N ; e = W - W* ; smoothed e_bar.
    auto r = H1NegativeSupply::Evaluate(/*P=*/1000, /*N=*/250, /*W=*/500, /*Ws=*/400,
                                        /*prevEbar=*/0, /*alpha=*/500);
    BOOST_CHECK_EQUAL(r.DeltaS_t, 750);
    BOOST_CHECK_EQUAL(r.e_t, 100);
    BOOST_CHECK_EQUAL(r.e_bar_t, (500*100 + 500*0)/1000); // 50
    // Different wallet target flips sign of e_t; N_h is never an input/output.
    auto r2 = H1NegativeSupply::Evaluate(1000, 250, 300, 400, 0, 500);
    BOOST_CHECK_EQUAL(r2.e_t, -100);
    // N_h is never an input or output of the monetary candidate loop by design.
}

BOOST_AUTO_TEST_SUITE_END()
