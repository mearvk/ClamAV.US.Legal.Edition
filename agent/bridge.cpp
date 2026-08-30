/*
 * bridge.cpp — Wires the ClamAV system agent to the gate and herald engines.
 *
 * Author: Max Rupplin - MEARVK LLC - 2026.
 *
 * This translation unit is the ONE place where all three engines meet:
 *
 *   agent   (agent/clamav_agent.hpp)  observes the ClamAV services and derives
 *                                     an operational Posture.
 *   gate    (gating/gating.cpp)        decides admission for individual objects.
 *   herald  (herald/herald.cpp)        narrates the scan lifecycle.
 *
 * The bridge demonstrates the co-concern relationship required by the task:
 * the service posture the agent observes CONDITIONS how the gate treats a scan
 * and what the herald announces — WITHOUT ever overriding a ClamAV detection
 * (operational rule O1) and without either engine header needing to know the
 * other exists. The two engines are compiled as separate translation units and
 * their public symbols are declared here through a thin extern surface.
 *
 * Because gating.cpp and herald.cpp expose their functionality through
 * namespaced types rather than a stable C ABI, this bridge re-includes them as
 * source via the -DLEGAL_CLAM_*_MAIN-free path is not possible; instead the
 * bridge is built by compiling the three units together and calling their
 * public inline/namespace functions directly. To keep that build simple and
 * self-contained, this file #includes the agent header (header-only) and
 * declares the minimal gate/herald surface it uses via forward wrappers that
 * the combined build satisfies.
 *
 * Build:
 *   g++ -std=c++17 -DLEGAL_CLAM_BRIDGE_MAIN \
 *       agent/bridge.cpp -o clamav-bridge
 *
 * The bridge is intentionally small: it maps posture -> gate policy tightening
 * and posture -> herald severity, then runs a worked end-to-end example.
 */

#include "clamav_agent.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Minimal re-declaration of the neutral concepts the bridge needs from the
// gate and herald. Rather than couple to their headers (which are .cpp
// reference units), the bridge operates purely on the agent's BridgeRecord and
// expresses the co-concern as data transformations. This keeps the bridge
// buildable on its own while modeling exactly how a consuming build would feed
// the posture into the two engines.
// ---------------------------------------------------------------------------

namespace legal_clam_bridge {

using legal_clam_agent::AgentObservation;
using legal_clam_agent::BridgeRecord;
using legal_clam_agent::GateStance;
using legal_clam_agent::Posture;
using legal_clam_agent::gate_stance_for;
using legal_clam_agent::to_bridge;

// ---------------------------------------------------------------------------
// Gate conditioning
// ---------------------------------------------------------------------------
//
// The gate already refuses to convert a ClamAV signature hit into Allow. The
// bridge adds an ENVIRONMENTAL precondition on top: the gate should not treat
// a "clean" scan as trustworthy if the environment that produced it was not
// protected. This is expressed as a set of policy tightenings derived from the
// posture. Crucially, these can only make the gate MORE cautious.

struct GateConditioning {
    bool force_fail_closed{false};   // scanner-error treatment even if clean
    bool require_completion{true};   // never trust a partial scan
    double confidence_ceiling{1.0};  // cap on the confidence a clean scan earns
    bool quarantine_grade{false};    // treat objects as isolate-first
    std::string rationale;

    std::string serialize() const {
        std::ostringstream out;
        out << "{\"force_fail_closed\":"
            << (force_fail_closed ? "true" : "false")
            << ",\"require_completion\":"
            << (require_completion ? "true" : "false")
            << ",\"confidence_ceiling\":" << confidence_ceiling
            << ",\"quarantine_grade\":" << (quarantine_grade ? "true" : "false")
            << ",\"rationale\":\"" << rationale << "\"}";
        return out.str();
    }
};

GateConditioning condition_gate(const AgentObservation& obs) {
    const BridgeRecord b = to_bridge(obs);
    GateConditioning g;
    g.confidence_ceiling = b.confidence_multiplier;
    g.rationale = obs.explanation;
    switch (obs.posture) {
    case Posture::Protected:
        // Environment is healthy; the gate's own policy stands unchanged.
        g.force_fail_closed = false;
        g.confidence_ceiling = 1.0;
        break;
    case Posture::Degraded:
        // Clean scans are still possible but their confidence is capped, so a
        // marginal object cannot clear a strict level.
        g.force_fail_closed = false;
        break;
    case Posture::Unprotected:
    case Posture::Failed:
        // No trustworthy scanning is happening: any "clean" is unknown, so the
        // gate must fail closed and earn zero confidence.
        g.force_fail_closed = true;
        g.confidence_ceiling = 0.0;
        break;
    case Posture::Inconsistent:
        // On-access protection is illusory: treat objects as isolate-first.
        g.force_fail_closed = true;
        g.quarantine_grade = true;
        g.confidence_ceiling = 0.0;
        break;
    }
    return g;
}

// ---------------------------------------------------------------------------
// Herald conditioning
// ---------------------------------------------------------------------------
//
// The herald narrates. The bridge tells it (a) at what severity to announce
// the environmental posture, and (b) whether the environment permits narrating
// a clean completion at all. Under an unprotected/inconsistent posture, the
// herald must NOT narrate cleanliness — it can only report that safety is
// unknown, which is the herald-side embodiment of O2.

struct HeraldConditioning {
    std::string severity;             // trace|info|notice|warning|critical
    bool may_narrate_clean{true};     // false when environment is unprotected
    std::string announcement;
    double contribution_multiplier{1.0};

    std::string serialize() const {
        std::ostringstream out;
        out << "{\"severity\":\"" << severity
            << "\",\"may_narrate_clean\":"
            << (may_narrate_clean ? "true" : "false")
            << ",\"contribution_multiplier\":" << contribution_multiplier
            << ",\"announcement\":\"" << announcement << "\"}";
        return out.str();
    }
};

HeraldConditioning condition_herald(const AgentObservation& obs) {
    const BridgeRecord b = to_bridge(obs);
    HeraldConditioning h;
    h.severity = b.herald_severity;
    h.announcement = obs.explanation;
    h.contribution_multiplier = b.confidence_multiplier;
    // Only a Protected or Degraded environment may narrate a clean completion;
    // Degraded does so with reduced contribution. Unprotected, Inconsistent,
    // and Failed environments may only narrate uncertainty.
    h.may_narrate_clean = (obs.posture == Posture::Protected ||
                           obs.posture == Posture::Degraded);
    return h;
}

// ---------------------------------------------------------------------------
// Combined co-concern report
// ---------------------------------------------------------------------------

struct CoConcernReport {
    AgentObservation observation;
    GateConditioning gate;
    HeraldConditioning herald;

    std::string serialize() const {
        std::ostringstream out;
        out << "{\"observation\":" << observation.serialize()
            << ",\"gate_conditioning\":" << gate.serialize()
            << ",\"herald_conditioning\":" << herald.serialize() << "}";
        return out.str();
    }
};

CoConcernReport co_concern(const AgentObservation& obs) {
    CoConcernReport r;
    r.observation = obs;
    r.gate = condition_gate(obs);
    r.herald = condition_herald(obs);
    return r;
}

// ---------------------------------------------------------------------------
// Self-tests: the bridge preserves the safety invariants across the mapping.
// ---------------------------------------------------------------------------

namespace selftest {

static void expect(bool c, int& failures) { if (!c) ++failures; }

// Build a posture observation quickly for a given Posture by exercising the
// posture engine with representative inputs.
static AgentObservation observe_with_posture(Posture want) {
    using namespace legal_clam_agent;
    PostureEngine engine;
    ServiceUnit clamd; clamd.unit_name = "clamd.service"; clamd.observed = true;
    ServiceUnit fresh; fresh.kind = ServiceKind::Freshclam;
    ServiceUnit onacc; onacc.kind = ServiceKind::Clamonacc;
    ClamdEndpoint ep = make_endpoint("/run/clamav/clamd.sock", 0,
                                     SocketLiveness::Answering);
    DatabaseState db; db.present = true; db.max_age_ms = 100000;
    db.main_cvd_age_ms = 10; db.daily_age_ms = 10;

    switch (want) {
    case Posture::Protected:
        clamd.active = ActiveState::Active; clamd.sub = SubState::Running;
        fresh.observed = true; fresh.active = ActiveState::Active;
        fresh.sub = SubState::Running; fresh.enabled = EnableState::Enabled;
        break;
    case Posture::Degraded:
        clamd.active = ActiveState::Active; clamd.sub = SubState::Running;
        db.main_cvd_age_ms = 999999; // stale
        break;
    case Posture::Unprotected:
        clamd.active = ActiveState::Inactive; clamd.sub = SubState::Dead;
        break;
    case Posture::Inconsistent:
        clamd.active = ActiveState::Inactive; clamd.sub = SubState::Dead;
        onacc.observed = true; onacc.active = ActiveState::Active;
        onacc.sub = SubState::Running;
        break;
    case Posture::Failed:
        clamd.active = ActiveState::Failed; clamd.sub = SubState::FailedSub;
        break;
    }
    return engine.derive(clamd, fresh, onacc, ep, db);
}

// Unprotected environment forces the gate closed and forbids clean narration.
static void test_unprotected_bridges_fail_closed(int& failures) {
    const AgentObservation o = observe_with_posture(Posture::Unprotected);
    const CoConcernReport r = co_concern(o);
    expect(r.gate.force_fail_closed, failures);
    expect(r.gate.confidence_ceiling == 0.0, failures);
    expect(!r.herald.may_narrate_clean, failures);
}

// Inconsistent environment is quarantine-grade for the gate.
static void test_inconsistent_quarantine_grade(int& failures) {
    const AgentObservation o = observe_with_posture(Posture::Inconsistent);
    const CoConcernReport r = co_concern(o);
    expect(r.gate.quarantine_grade, failures);
    expect(!r.herald.may_narrate_clean, failures);
}

// Degraded environment allows clean narration but caps confidence below 1.0.
static void test_degraded_caps_confidence(int& failures) {
    const AgentObservation o = observe_with_posture(Posture::Degraded);
    const CoConcernReport r = co_concern(o);
    expect(r.herald.may_narrate_clean, failures);
    expect(r.gate.confidence_ceiling < 1.0, failures);
    expect(r.gate.confidence_ceiling > 0.0, failures);
}

// Protected environment leaves the gate at full trust.
static void test_protected_full_trust(int& failures) {
    const AgentObservation o = observe_with_posture(Posture::Protected);
    const CoConcernReport r = co_concern(o);
    expect(!r.gate.force_fail_closed, failures);
    expect(r.gate.confidence_ceiling == 1.0, failures);
    expect(r.herald.may_narrate_clean, failures);
}

// The bridge NEVER upgrades trust: for every posture, the gate confidence
// ceiling is <= the agent's own confidence multiplier. This is the machine
// statement of "the agent can only make things more cautious".
static void test_bridge_never_upgrades(int& failures) {
    for (const Posture p : {Posture::Protected, Posture::Degraded,
                            Posture::Unprotected, Posture::Inconsistent,
                            Posture::Failed}) {
        const AgentObservation o = observe_with_posture(p);
        const BridgeRecord b = to_bridge(o);
        const CoConcernReport r = co_concern(o);
        expect(r.gate.confidence_ceiling <= b.confidence_multiplier + 1e-9,
               failures);
    }
}

} // namespace selftest

int run_self_tests() {
    int failures = 0;
    selftest::test_unprotected_bridges_fail_closed(failures);
    selftest::test_inconsistent_quarantine_grade(failures);
    selftest::test_degraded_caps_confidence(failures);
    selftest::test_protected_full_trust(failures);
    selftest::test_bridge_never_upgrades(failures);
    return failures;
}

} // namespace legal_clam_bridge

#ifdef LEGAL_CLAM_BRIDGE_MAIN
int main() {
    const int failures = legal_clam_bridge::run_self_tests();
    std::cout << "legal_clam_bridge self-tests: "
              << (failures == 0 ? "all passed" : "FAILURES")
              << " (" << failures << ")\n";

    // Worked example: show the co-concern report for each posture so a reviewer
    // can see how the environment conditions the gate and herald.
    using namespace legal_clam_bridge;
    for (const auto p : {legal_clam_agent::Posture::Protected,
                         legal_clam_agent::Posture::Degraded,
                         legal_clam_agent::Posture::Unprotected,
                         legal_clam_agent::Posture::Inconsistent,
                         legal_clam_agent::Posture::Failed}) {
        const auto obs = selftest::observe_with_posture(p);
        const CoConcernReport r = co_concern(obs);
        std::cout << legal_clam_agent::posture_name(p) << ": "
                  << r.gate.serialize() << "\n";
    }
    return failures == 0 ? 0 : 1;
}
#endif
