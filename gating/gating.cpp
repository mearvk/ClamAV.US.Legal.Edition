/*
 * gating.cpp — System-centric admission gate for ClamAV.US.Legal.Edition.
 *
 * Author: Max Rupplin - MEARVK LLC - 2026.
 *
 * ---------------------------------------------------------------------------
 * PURPOSE
 * ---------------------------------------------------------------------------
 * This translation unit is the authoritative C++ reference embodiment of the
 * procedural admission gate. The eight numbered levels under gating/{1,3,5,7}
 * and herald/{2,4,6,8} are the staged, C90 embodiment of the discipline that
 * this file specifies once, completely, and reviewably.
 *
 * The gate follows the scanner discipline demanded of the ClamAV reference:
 *
 *   - explicit terminal states,
 *   - bounded work (every loop and recursion is budgeted),
 *   - a hard separation of technical evidence from policy, and
 *   - fail-closed handling of scanner errors and uncertainty.
 *
 * ---------------------------------------------------------------------------
 * THE DEMOGRAPHIC FIREWALL (non-negotiable invariant)
 * ---------------------------------------------------------------------------
 * No human identity, reputation, demographic attribute, affiliation, belief,
 * or membership is a malware indicator, and none may enter the gate decision
 * path. This is not a stylistic preference; it is a structural guarantee
 * enforced in code by `system_only()` and audited by `firewall_report()`.
 * Any evidence tagged with an operator/demographic origin is stripped before
 * evaluation, and the stripping is itself recorded so that a reviewer can see
 * that the firewall acted.
 *
 * ---------------------------------------------------------------------------
 * THE SIX SAFETY RULES (from PROCEDURAL_GATING.md)
 * ---------------------------------------------------------------------------
 *   1. Never treat absence of a gate finding as proof of safety.
 *   2. Never replace a ClamAV detection with a gate result.
 *   3. Preserve uncertainty when content cannot be completely observed.
 *   4. Preserve provenance where available.
 *   5. Require explicit closure at level 8.
 *   6. Keep authorship, provenance, detection, and legal responsibility
 *      distinct.
 *
 * ---------------------------------------------------------------------------
 * THE CAUSAL VOCABULARY (from PROCEDURAL_CAUSATION.hss)
 * ---------------------------------------------------------------------------
 *   ROOT -> METHOD -> CAUSE -> ATTENTION -> ATTENUATION -> CLOSURE
 *
 * The gate models an object as a continuing procedural state rather than a
 * one-shot verdict. It records the chain of causes that produced the present
 * observation, raises attention where warranted, attenuates factors under
 * uncertainty, and requires an explicit closure that never implies that
 * unobserved content was proven safe.
 *
 * This file is a single self-contained translation unit. It has no external
 * dependencies beyond the C++17 standard library so that it can be reviewed
 * in isolation before any deeper coupling to libclamav.
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace legal_clam_gate {

// ===========================================================================
// SECTION 1 — Enumerations and their canonical names
// ===========================================================================
//
// Every enumeration used in a decision is paired with a stable string name.
// The names are part of the audit contract: downstream consumers, tests, and
// human reviewers all key off them, so they must never drift silently.

// The terminal disposition the gate can express. The gate is advisory: even
// `Allow` never overrides a ClamAV detection, and `Error` is fail-closed.
enum class Verdict {
    Allow,       // technical evidence satisfies the admission policy
    Observe,     // insufficient certainty; continue to watch, do not admit
    Quarantine,  // isolate; a boundary or signature condition was met
    Reject,      // refuse admission outright (path escape, policy rejection)
    Error        // scanner/procedural failure; safety is unknown -> fail closed
};

// Why a verdict was reached. Reasons are technical, never demographic.
enum class Reason {
    None,
    MissingIdentity,
    IdentityMismatch,
    Signature,
    Heuristic,
    ResourceLimit,
    ScannerError,
    PathBoundary,
    Incomplete,
    Policy,
    ProvenanceGap,
    CausalBreak,
    AttenuationFloor,
    ClosureMissing
};

// The technical channel a piece of evidence came from. Mirrors the five
// observation channels in DESCRIPTOR.md plus the internal scanner channel.
enum class EvidenceKind {
    Identity,     // object id, hash, canonical path
    Provenance,   // origin, signer, version, timestamp, change history
    Signature,    // ClamAV signature / hash / logical match
    Heuristic,    // heuristic or bytecode indication (not proof)
    Resource,     // bytes, files, recursion, archive depth, elapsed time
    Permission,   // access boundaries encountered during inspection
    Transform,    // decompression / decoding transforms
    Scanner,      // scanner execution state itself
    Dependency,   // retro-dependency: prior library/component required
    Update        // update-channel event: signature or package change
};

// Ordered confidence. Ordering matters: comparisons like `>= High` are used
// to require independent corroboration.
enum class Confidence { None, Low, Medium, High, Confirmed };

// The causal vocabulary stages from PROCEDURAL_CAUSATION.hss.
enum class CausalStage { Root, Method, Cause, Attention, Attenuation, Closure };

// The five procedural-consequence outcomes of the continuity test
// (DESCRIPTOR.md section 8E).
enum class Consequence { Created, Continued, Modified, Suspended, Terminated };

// The eight sequitur levels. Odd levels are gating; even are herald. Each
// level inherits the concerns of the prior and adds one more gate. The ladder
// only ever grows more cautious.
enum class Level {
    L1_Baseline = 1,
    L2_SecondLook = 2,
    L3_Tracing = 3,
    L4_Qualification = 4,
    L5_Causal = 5,
    L6_Attenuation = 6,
    L7_Closure = 7,
    L8_OperatorHerald = 8
};

static const char* verdict_name(Verdict v) {
    switch (v) {
    case Verdict::Allow:      return "allow";
    case Verdict::Observe:    return "observe";
    case Verdict::Quarantine: return "quarantine";
    case Verdict::Reject:     return "reject";
    case Verdict::Error:      return "error";
    }
    return "unknown";
}

static const char* reason_name(Reason r) {
    switch (r) {
    case Reason::None:             return "none";
    case Reason::MissingIdentity:  return "missing_identity";
    case Reason::IdentityMismatch: return "identity_mismatch";
    case Reason::Signature:        return "signature";
    case Reason::Heuristic:        return "heuristic";
    case Reason::ResourceLimit:    return "resource_limit";
    case Reason::ScannerError:     return "scanner_error";
    case Reason::PathBoundary:     return "path_boundary";
    case Reason::Incomplete:       return "incomplete";
    case Reason::Policy:           return "policy";
    case Reason::ProvenanceGap:    return "provenance_gap";
    case Reason::CausalBreak:      return "causal_break";
    case Reason::AttenuationFloor: return "attenuation_floor";
    case Reason::ClosureMissing:   return "closure_missing";
    }
    return "unknown";
}

static const char* kind_name(EvidenceKind k) {
    switch (k) {
    case EvidenceKind::Identity:   return "identity";
    case EvidenceKind::Provenance: return "provenance";
    case EvidenceKind::Signature:  return "signature";
    case EvidenceKind::Heuristic:  return "heuristic";
    case EvidenceKind::Resource:   return "resource";
    case EvidenceKind::Permission: return "permission";
    case EvidenceKind::Transform:  return "transform";
    case EvidenceKind::Scanner:    return "scanner";
    case EvidenceKind::Dependency: return "dependency";
    case EvidenceKind::Update:     return "update";
    }
    return "unknown";
}

static const char* confidence_name(Confidence c) {
    switch (c) {
    case Confidence::None:      return "none";
    case Confidence::Low:       return "low";
    case Confidence::Medium:    return "medium";
    case Confidence::High:      return "high";
    case Confidence::Confirmed: return "confirmed";
    }
    return "unknown";
}

static const char* stage_name(CausalStage s) {
    switch (s) {
    case CausalStage::Root:        return "root";
    case CausalStage::Method:      return "method";
    case CausalStage::Cause:       return "cause";
    case CausalStage::Attention:   return "attention";
    case CausalStage::Attenuation: return "attenuation";
    case CausalStage::Closure:     return "closure";
    }
    return "unknown";
}

static const char* consequence_name(Consequence c) {
    switch (c) {
    case Consequence::Created:    return "created";
    case Consequence::Continued:  return "continued";
    case Consequence::Modified:   return "modified";
    case Consequence::Suspended:  return "suspended";
    case Consequence::Terminated: return "terminated";
    }
    return "unknown";
}

static const char* level_name(Level l) {
    switch (l) {
    case Level::L1_Baseline:       return "baseline";
    case Level::L2_SecondLook:     return "second_look";
    case Level::L3_Tracing:        return "tracing";
    case Level::L4_Qualification:  return "qualification";
    case Level::L5_Causal:         return "causal";
    case Level::L6_Attenuation:    return "attenuation";
    case Level::L7_Closure:        return "closure";
    case Level::L8_OperatorHerald: return "operator_herald";
    }
    return "unknown";
}

// ===========================================================================
// SECTION 2 — Text utilities (deterministic, dependency-free)
// ===========================================================================
//
// The gate emits JSON for audit. JSON escaping and small string helpers live
// here. They are deliberately conservative: they never throw and never assume
// a locale.

static std::string escape_json(std::string_view value) {
    std::ostringstream out;
    for (const unsigned char c : value) {
        switch (c) {
        case '\"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\n': out << "\\n";  break;
        case '\r': out << "\\r";  break;
        case '\t': out << "\\t";  break;
        case '\b': out << "\\b";  break;
        case '\f': out << "\\f";  break;
        default:
            if (c < 0x20) {
                // Control characters become \u00XX so audit records stay
                // strictly valid JSON regardless of source bytes.
                static const char* hex = "0123456789abcdef";
                out << "\\u00" << hex[(c >> 4) & 0xF] << hex[c & 0xF];
            } else {
                out << static_cast<char>(c);
            }
        }
    }
    return out.str();
}

static bool is_blank(std::string_view s) {
    for (const char c : s)
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') return false;
    return true;
}

static std::string trim(std::string_view s) {
    size_t begin = 0, end = s.size();
    while (begin < end && (s[begin] == ' ' || s[begin] == '\t')) ++begin;
    while (end > begin && (s[end - 1] == ' ' || s[end - 1] == '\t')) --end;
    return std::string(s.substr(begin, end - begin));
}

// A stable, non-cryptographic digest used only to give evidence a short,
// order-independent identity in audit output. It is NOT a security hash and
// must never be relied upon to prove content integrity — that remains the job
// of the ClamAV signed databases.
static uint64_t stable_digest(std::string_view s) {
    uint64_t h = 1469598103934665603ULL; // FNV-1a offset basis
    for (const unsigned char c : s) {
        h ^= c;
        h *= 1099511628211ULL;
    }
    return h;
}

// ===========================================================================
// SECTION 3 — Path safety
// ===========================================================================
//
// Path handling is a classic source of scanner escapes. The gate accepts only
// canonical, relative, boundary-respecting paths. Absolute paths, empty
// components, "." and ".." are all rejected. This mirrors gate1's `valid_path`
// but is stricter and reusable.

static bool safe_relative_path(std::string_view path) {
    if (path.empty()) return false;
    if (path.front() == '/') return false;      // no absolute paths
    if (path.find('\0') != std::string_view::npos) return false; // no NUL
    size_t start = 0;
    size_t components = 0;
    while (start < path.size()) {
        size_t end = path.find('/', start);
        if (end == std::string_view::npos) end = path.size();
        const auto component = path.substr(start, end - start);
        if (component.empty()) return false;     // no // or trailing /
        if (component == ".") return false;       // no current-dir hops
        if (component == "..") return false;      // no parent-dir escapes
        ++components;
        if (components > 4096) return false;      // bounded work
        start = end + 1;
    }
    return components > 0;
}

// Count path depth for boundary observation.
static uint32_t path_depth(std::string_view path) {
    if (path.empty()) return 0;
    uint32_t depth = 1;
    for (const char c : path)
        if (c == '/') ++depth;
    return depth;
}

// ===========================================================================
// SECTION 4 — Evidence
// ===========================================================================
//
// Evidence is the atomic unit of the gate. Each item names its channel, a
// confidence, a technical source, and a free-form detail. Evidence may also
// carry an `origin` tag; the demographic firewall keys on this tag.

struct Evidence {
    EvidenceKind kind{EvidenceKind::Scanner};
    Confidence confidence{Confidence::None};
    std::string source;   // technical source, e.g. "clamav", "libmspack"
    std::string detail;   // human-readable technical detail
    std::string origin;   // "system" (default) or "operator" (firewalled)
    uint64_t observed_ms{0};

    Evidence() = default;
    Evidence(EvidenceKind k, Confidence c, std::string src, std::string det)
        : kind(k), confidence(c), source(std::move(src)),
          detail(std::move(det)), origin("system") {}

    // Is this evidence sourced from an operator/demographic channel? Such
    // evidence is inadmissible to the decision path.
    bool is_operator_origin() const {
        if (origin == "operator") return true;
        if (kind == EvidenceKind::Permission && source == "operator")
            return true;
        return false;
    }

    std::string id() const {
        std::ostringstream key;
        key << kind_name(kind) << ':' << source << ':' << detail;
        std::ostringstream out;
        out << std::hex << stable_digest(key.str());
        return out.str();
    }
};

static Evidence make_evidence(EvidenceKind k, Confidence c,
                              std::string source, std::string detail) {
    return Evidence(k, c, std::move(source), std::move(detail));
}

// ===========================================================================
// SECTION 5 — Causal chain (ROOT -> ... -> CLOSURE)
// ===========================================================================
//
// The causal chain records how the present observation came to be. A link may
// be followed by another link when the observed state depends on a prior
// transformation, dependency, acquisition, or update (PROCEDURAL_CAUSATION).

struct CausalLink {
    CausalStage stage{CausalStage::Root};
    std::string actor;    // technical actor: "updater", "extractor", "engine"
    std::string detail;
    double weight{1.0};   // contribution weight in [0,1] after attenuation

    CausalLink() = default;
    CausalLink(CausalStage s, std::string a, std::string d, double w = 1.0)
        : stage(s), actor(std::move(a)), detail(std::move(d)), weight(w) {}
};

class CausalChain {
public:
    // A chain must begin at ROOT and end at CLOSURE. Between them, the stages
    // must not regress: ATTENTION cannot precede CAUSE, and so forth.
    void add(const CausalLink& link) { links_.push_back(link); }

    void add(CausalStage stage, std::string actor, std::string detail,
             double weight = 1.0) {
        links_.emplace_back(stage, std::move(actor), std::move(detail), weight);
    }

    const std::vector<CausalLink>& links() const { return links_; }

    bool empty() const { return links_.empty(); }
    size_t size() const { return links_.size(); }

    // The chain is well-formed if it starts at ROOT, ends at CLOSURE, and the
    // stage index is monotonically non-decreasing across links. A well-formed
    // chain proves the procedure reached a closure rather than trailing off.
    bool well_formed() const {
        if (links_.size() < 2) return false;
        if (links_.front().stage != CausalStage::Root) return false;
        if (links_.back().stage != CausalStage::Closure) return false;
        int last = static_cast<int>(links_.front().stage);
        for (size_t i = 1; i < links_.size(); ++i) {
            const int cur = static_cast<int>(links_[i].stage);
            if (cur < last) return false; // no regression
            last = cur;
        }
        return true;
    }

    bool has_stage(CausalStage s) const {
        for (const auto& l : links_)
            if (l.stage == s) return true;
        return false;
    }

    // Effective contribution: the product of link weights, floored at zero.
    // Attenuation links push this down; a broken chain drives it toward zero.
    double effective_contribution() const {
        if (links_.empty()) return 0.0;
        double acc = 1.0;
        for (const auto& l : links_) {
            double w = l.weight;
            if (w < 0.0) w = 0.0;
            if (w > 1.0) w = 1.0;
            acc *= w;
        }
        return acc;
    }

    std::string serialize() const {
        std::ostringstream out;
        out << '[';
        bool first = true;
        for (const auto& l : links_) {
            if (!first) out << ',';
            first = false;
            out << "{\"stage\":\"" << stage_name(l.stage)
                << "\",\"actor\":\"" << escape_json(l.actor)
                << "\",\"detail\":\"" << escape_json(l.detail)
                << "\",\"weight\":" << l.weight << '}';
        }
        out << ']';
        return out.str();
    }

private:
    std::vector<CausalLink> links_;
};

// ===========================================================================
// SECTION 6 — Retro-dependency record (DESCRIPTOR.md section 4)
// ===========================================================================
//
// A retro-dependency points backward from a present state to an earlier
// condition required to explain that state. The gate records these so a
// reviewer can distinguish a change in the artifact from a change in the
// scanner's knowledge or environment.

struct RetroDependency {
    std::string present_observation;   // "executable loads library"
    std::string prior_condition;       // "libfoo 2.1 supplied interface"
    EvidenceKind channel{EvidenceKind::Dependency};
    bool prior_condition_known{false}; // false => provenance gap
    Confidence linkage{Confidence::Low};

    std::string serialize() const {
        std::ostringstream out;
        out << "{\"present\":\"" << escape_json(present_observation)
            << "\",\"prior\":\"" << escape_json(prior_condition)
            << "\",\"channel\":\"" << kind_name(channel)
            << "\",\"known\":" << (prior_condition_known ? "true" : "false")
            << ",\"linkage\":\"" << confidence_name(linkage) << "\"}";
        return out.str();
    }
};

// ===========================================================================
// SECTION 7 — Observation
// ===========================================================================
//
// An Observation is the full technical record presented to the gate. It is
// system-centric: everything here is a measurable property of the object or
// the scan, never a property of a person.

struct Observation {
    // Identity
    std::string object_id;
    std::string canonical_path;
    std::string content_digest;   // e.g. a ClamAV-reported hash (optional)

    // Resource accounting
    uint64_t size_bytes{0};
    uint64_t scanned_bytes{0};
    uint64_t scanned_files{0};
    uint32_t recursion_depth{0};
    uint32_t archive_depth{0};
    uint64_t elapsed_ms{0};

    // Scan state flags
    bool scan_completed{false};
    bool signature_hit{false};
    bool heuristic_hit{false};
    bool scanner_error{false};
    bool permission_denied{false};
    bool transform_failed{false};
    bool path_rejected{false};

    // Continuity (DESCRIPTOR.md section 8)
    bool prior_state_recorded{false};
    bool intervening_event_recorded{false};
    bool resulting_state_recorded{false};

    // Structured technical detail
    std::vector<Evidence> evidence;
    std::vector<RetroDependency> retro_dependencies;
    CausalChain causal;

    Observation() = default;

    void add_evidence(Evidence e) { evidence.push_back(std::move(e)); }
    void add_retro(RetroDependency r) {
        retro_dependencies.push_back(std::move(r));
    }
};

// ===========================================================================
// SECTION 8 — Policy
// ===========================================================================
//
// Policy is where operator-tunable knobs live. Note carefully: none of these
// knobs can admit an object that ClamAV flagged. They can only make the gate
// MORE cautious, never less. The escalation ladder derives policies for each
// of the eight levels, each stricter than the last.

struct Policy {
    // Resource budgets. Exceeding any of these quarantines the object because
    // an unbounded scan is an unreliable scan.
    uint64_t max_bytes{1024ULL * 1024ULL * 1024ULL}; // 1 GiB
    uint64_t max_files{100000};
    uint32_t max_recursion{32};
    uint32_t max_archive_depth{16};
    uint64_t max_elapsed_ms{300000}; // 5 minutes
    uint32_t max_path_depth{64};

    // Behavioral switches. All default to the cautious interpretation.
    bool fail_closed_on_scanner_error{true};
    bool require_completion{true};
    bool quarantine_signature_hits{true};
    bool reject_path_escape{true};
    bool reject_permission_denied{false};
    bool quarantine_transform_failure{true};

    // Corroboration: require this many independent high-confidence evidence
    // kinds before an Allow is possible.
    uint32_t minimum_high_confidence_kinds{1};

    // Provenance and causality requirements grow with the level.
    bool require_provenance{false};
    bool require_well_formed_causal_chain{false};
    bool require_explicit_closure{false};
    bool require_continuity_record{false};

    // The attenuation floor: if the causal chain's effective contribution
    // falls below this, the object cannot be admitted regardless of other
    // evidence. Uncertainty is not cleanliness.
    double attenuation_floor{0.0};

    Level level{Level::L1_Baseline};
};

// Derive the policy for a given level. Each higher level inherits the prior
// and tightens one or more knobs. The progression is intentionally more
// cautious, never more permissive.
static Policy policy_for_level(Level level) {
    Policy p;
    p.level = level;
    const int n = static_cast<int>(level);

    // Level 1 baseline: structural provenance only, advisory, not fail-closed.
    if (n >= 1) {
        p.require_provenance = false;
        p.fail_closed_on_scanner_error = false;
        p.require_completion = false;
        p.minimum_high_confidence_kinds = 0;
    }
    // Level 2 second-look: require completion and one corroborating kind.
    if (n >= 2) {
        p.require_completion = true;
        p.minimum_high_confidence_kinds = 1;
    }
    // Level 3 tracing: require provenance and become fail-closed.
    if (n >= 3) {
        p.require_provenance = true;
        p.fail_closed_on_scanner_error = true;
    }
    // Level 4 qualification: require two independent high-confidence kinds.
    if (n >= 4) {
        p.minimum_high_confidence_kinds = 2;
        p.require_continuity_record = true;
    }
    // Level 5 causal: require a well-formed causal chain.
    if (n >= 5) {
        p.require_well_formed_causal_chain = true;
    }
    // Level 6 attenuation: install a non-trivial attenuation floor.
    if (n >= 6) {
        p.attenuation_floor = 0.25;
        p.reject_permission_denied = true;
    }
    // Level 7 closure: require explicit closure and raise the floor.
    if (n >= 7) {
        p.require_explicit_closure = true;
        p.attenuation_floor = 0.40;
        p.minimum_high_confidence_kinds = 3;
    }
    // Level 8 operator herald: strictest budgets and highest floor.
    if (n >= 8) {
        p.attenuation_floor = 0.55;
        p.max_recursion = 24;
        p.max_archive_depth = 12;
    }
    return p;
}

// ===========================================================================
// SECTION 9 — Decision
// ===========================================================================

struct Decision {
    Verdict verdict{Verdict::Error};
    Reason reason{Reason::Policy};
    Confidence confidence{Confidence::None};
    Consequence consequence{Consequence::Suspended};
    Level level{Level::L1_Baseline};
    std::string object_id;
    std::string explanation;
    std::vector<Evidence> supporting;
    double effective_contribution{0.0};

    // A decision is admitting only when it is an explicit Allow. Everything
    // else — Observe, Quarantine, Reject, Error — withholds admission.
    bool admits() const { return verdict == Verdict::Allow; }

    // Fail-closed helper: is this a state where we must assume the worst?
    bool is_fail_closed() const {
        return verdict == Verdict::Error || verdict == Verdict::Quarantine ||
               verdict == Verdict::Reject;
    }
};

// ===========================================================================
// SECTION 10 — The demographic firewall
// ===========================================================================
//
// Before any evaluation, the observation passes through system_only(). Any
// evidence carrying an operator/demographic origin is removed and counted.
// The count is surfaced in the audit output so a reviewer can confirm the
// firewall acted. This is safety rule 6 (keep authorship/detection distinct)
// realized in code.

struct FirewallReport {
    size_t admitted{0};
    size_t stripped{0};
    std::vector<std::string> stripped_sources;
};

static Observation system_only(Observation o, FirewallReport* report = nullptr) {
    std::vector<Evidence> technical;
    technical.reserve(o.evidence.size());
    for (auto& e : o.evidence) {
        if (e.is_operator_origin()) {
            if (report) {
                ++report->stripped;
                report->stripped_sources.push_back(e.source);
            }
        } else {
            if (report) ++report->admitted;
            technical.push_back(std::move(e));
        }
    }
    o.evidence = std::move(technical);
    return o;
}

// ===========================================================================
// SECTION 11 — Evidence accounting
// ===========================================================================

// Count distinct high-confidence evidence kinds. Requiring independent kinds
// (not merely independent items) resists a single noisy source dominating.
static size_t high_confidence_kinds(const Observation& o) {
    std::array<bool, 16> seen{};
    size_t count = 0;
    for (const auto& e : o.evidence) {
        if (e.confidence < Confidence::High) continue;
        const auto index = static_cast<size_t>(e.kind);
        if (index < seen.size() && !seen[index]) {
            seen[index] = true;
            ++count;
        }
    }
    return count;
}

// Is any provenance evidence present at all? Safety rule 4.
static bool has_provenance(const Observation& o) {
    for (const auto& e : o.evidence)
        if (e.kind == EvidenceKind::Provenance) return true;
    return false;
}

// Count retro-dependencies whose prior condition is unknown. Each unknown
// prior is a provenance gap that attenuates confidence.
static size_t provenance_gaps(const Observation& o) {
    size_t gaps = 0;
    for (const auto& r : o.retro_dependencies)
        if (!r.prior_condition_known) ++gaps;
    return gaps;
}

static bool resource_budget_ok(const Observation& o, const Policy& p) {
    if (o.size_bytes > p.max_bytes) return false;
    if (o.scanned_bytes > p.max_bytes) return false;
    if (o.scanned_files > p.max_files) return false;
    if (o.recursion_depth > p.max_recursion) return false;
    if (o.archive_depth > p.max_archive_depth) return false;
    if (o.elapsed_ms > p.max_elapsed_ms) return false;
    if (path_depth(o.canonical_path) > p.max_path_depth) return false;
    return true;
}

// ===========================================================================
// SECTION 12 — Decision construction
// ===========================================================================

static Decision make_decision(Verdict v, Reason r, Confidence c,
                              Consequence q, const Policy& p,
                              const Observation& o, std::string text) {
    Decision d;
    d.verdict = v;
    d.reason = r;
    d.confidence = c;
    d.consequence = q;
    d.level = p.level;
    d.object_id = o.object_id;
    d.explanation = std::move(text);
    d.supporting = o.evidence;
    d.effective_contribution = o.causal.empty()
                                   ? 0.0
                                   : o.causal.effective_contribution();
    return d;
}

// ===========================================================================
// SECTION 13 — The five-stage continuity test (DESCRIPTOR.md section 8)
// ===========================================================================
//
//   A. Identify           — object id / path / hash / provenance present
//   B. Prior state        — dependencies/config/signatures before the event
//   C. Intervening event  — an install/update/scan/exec/removal event
//   D. Resulting state     — what changed and what remained continuous
//   E. Consequence         — created / continued / modified / suspended /
//                            terminated
//
// The test is descriptive; it does not by itself grant or deny admission. It
// classifies the procedural consequence, which the evaluator then uses.

struct ContinuityResult {
    bool identified{false};
    bool prior_recorded{false};
    bool event_recorded{false};
    bool result_recorded{false};
    Consequence consequence{Consequence::Suspended};
    std::string note;
};

static ContinuityResult continuity_test(const Observation& o) {
    ContinuityResult r;
    r.identified = !o.object_id.empty();
    r.prior_recorded = o.prior_state_recorded;
    r.event_recorded = o.intervening_event_recorded;
    r.result_recorded = o.resulting_state_recorded;

    if (!r.identified) {
        r.consequence = Consequence::Suspended;
        r.note = "object could not be identified";
        return r;
    }
    if (!r.prior_recorded && !r.event_recorded && !r.result_recorded) {
        r.consequence = Consequence::Created;
        r.note = "first observation; no prior procedural state recorded";
        return r;
    }
    if (r.prior_recorded && r.event_recorded && r.result_recorded) {
        // We saw a before, an event, and an after: determine what the event
        // did to the object's continuing condition.
        if (o.transform_failed || o.scanner_error) {
            r.consequence = Consequence::Suspended;
            r.note = "event left the object in an unresolved state";
        } else if (o.signature_hit) {
            r.consequence = Consequence::Terminated;
            r.note = "detection terminates the safe procedural condition";
        } else {
            r.consequence = Consequence::Modified;
            r.note = "event modified the object; continuity preserved";
        }
        return r;
    }
    // Partial continuity record.
    r.consequence = Consequence::Continued;
    r.note = "partial procedural record; object continues under observation";
    return r;
}

// ===========================================================================
// SECTION 14 — The evaluator
// ===========================================================================
//
// This is the heart of the gate. The order of checks is deliberate and
// documented: the most decisive, fail-closed conditions are tested first so
// that no later, more permissive branch can ever be reached once a hard stop
// applies. Read top to bottom, the function only ever becomes more cautious.

Decision evaluate(const Observation& raw, const Policy& p,
                  FirewallReport* firewall = nullptr) {
    // Step 0: the demographic firewall runs before anything else.
    const Observation o = system_only(raw, firewall);

    // Step 1: identity is mandatory. Without an object identity there is
    // nothing to reason about, and we fail closed (Error), not open. A blank
    // (whitespace-only) identity is treated as absent.
    if (o.object_id.empty() || is_blank(o.object_id))
        return make_decision(Verdict::Error, Reason::MissingIdentity,
                             Confidence::Low, Consequence::Suspended, p, o,
                             "technical object identity is required");

    // Step 2: path boundary. A path escape is an outright rejection: the
    // object tried to reference something outside the sanctioned tree.
    if (p.reject_path_escape &&
        (o.path_rejected || !safe_relative_path(o.canonical_path)))
        return make_decision(Verdict::Reject, Reason::PathBoundary,
                             Confidence::High, Consequence::Terminated, p, o,
                             "canonical path crossed the configured boundary");

    // Step 3: resource budget. An unbounded scan is unreliable; quarantine.
    if (!resource_budget_ok(o, p))
        return make_decision(Verdict::Quarantine, Reason::ResourceLimit,
                             Confidence::High, Consequence::Suspended, p, o,
                             "scan resource budget was exceeded");

    // Step 4: permission boundary, if policy elevates it to a stop.
    if (o.permission_denied && p.reject_permission_denied)
        return make_decision(Verdict::Quarantine, Reason::Policy,
                             Confidence::High, Consequence::Suspended, p, o,
                             "permission boundary prevented inspection");

    // Step 5: transform failure. If a required decode/decompress failed, the
    // content underneath was never actually inspected.
    if (o.transform_failed && p.quarantine_transform_failure)
        return make_decision(Verdict::Quarantine, Reason::Policy,
                             Confidence::High, Consequence::Suspended, p, o,
                             "required transformation failed; content unseen");

    // Step 6: scanner error. Fail closed by default: a scanner that could not
    // run tells us nothing, and "nothing" is not "clean" (safety rule 1).
    if (o.scanner_error)
        return p.fail_closed_on_scanner_error
            ? make_decision(Verdict::Error, Reason::ScannerError,
                            Confidence::Low, Consequence::Suspended, p, o,
                            "scanner execution failed; safety is unknown")
            : make_decision(Verdict::Observe, Reason::ScannerError,
                            Confidence::Low, Consequence::Continued, p, o,
                            "scanner execution failed; result is uncertain");

    // Step 7: completion. An incomplete scan has not reached a terminal
    // observation; we withhold admission and keep observing (safety rule 3).
    if (p.require_completion && !o.scan_completed)
        return make_decision(Verdict::Observe, Reason::Incomplete,
                             Confidence::Medium, Consequence::Continued, p, o,
                             "scan did not reach a terminal observation");

    // Step 8: SIGNATURE. This is the ClamAV verdict. It is authoritative and
    // terminal. The gate NEVER converts a signature hit to Allow. It either
    // quarantines (default) or rejects, per policy (safety rule 2).
    if (o.signature_hit)
        return p.quarantine_signature_hits
            ? make_decision(Verdict::Quarantine, Reason::Signature,
                            Confidence::Confirmed, Consequence::Terminated,
                            p, o, "signature evidence requires isolation")
            : make_decision(Verdict::Reject, Reason::Signature,
                            Confidence::Confirmed, Consequence::Terminated,
                            p, o, "signature evidence rejected by policy");

    // Step 9: heuristic. A heuristic indication is never treated as proof; it
    // moves us to Observe rather than Quarantine or Allow.
    if (o.heuristic_hit)
        return make_decision(Verdict::Observe, Reason::Heuristic,
                             Confidence::Medium, Consequence::Continued, p, o,
                             "heuristic evidence is not treated as proof");

    // Step 10: provenance requirement (safety rule 4). Higher levels demand
    // that provenance actually be present, not merely assumed. Additionally,
    // if any recorded retro-dependency has an unknown prior condition, that is
    // a provenance gap in the causal history and withholds admission.
    if (p.require_provenance && !has_provenance(o))
        return make_decision(Verdict::Observe, Reason::ProvenanceGap,
                             Confidence::Medium, Consequence::Continued, p, o,
                             "required provenance evidence is absent");
    if (p.require_provenance && provenance_gaps(o) > 0)
        return make_decision(Verdict::Observe, Reason::ProvenanceGap,
                             Confidence::Medium, Consequence::Continued, p, o,
                             "a retro-dependency has an unknown prior state");

    // Step 11: continuity record requirement.
    if (p.require_continuity_record) {
        const ContinuityResult cr = continuity_test(o);
        if (!cr.identified || !cr.prior_recorded || !cr.result_recorded)
            return make_decision(Verdict::Observe, Reason::Incomplete,
                                 Confidence::Medium, cr.consequence, p, o,
                                 "continuity record is incomplete: " + cr.note);
    }

    // Step 12: causal chain well-formedness (safety rule 5 in spirit).
    if (p.require_well_formed_causal_chain && !o.causal.well_formed())
        return make_decision(Verdict::Observe, Reason::CausalBreak,
                             Confidence::Medium, Consequence::Continued, p, o,
                             "causal chain is not well-formed to closure");

    // Step 13: explicit closure requirement (safety rule 5, literally).
    if (p.require_explicit_closure && !o.causal.has_stage(CausalStage::Closure))
        return make_decision(Verdict::Observe, Reason::ClosureMissing,
                             Confidence::Medium, Consequence::Continued, p, o,
                             "explicit procedural closure is required");

    // Step 14: attenuation floor. Even with everything else in order, if the
    // causal chain's effective contribution is below the floor, uncertainty
    // dominates and we cannot admit (safety rule 3).
    if (p.attenuation_floor > 0.0 && !o.causal.empty() &&
        o.causal.effective_contribution() < p.attenuation_floor)
        return make_decision(Verdict::Observe, Reason::AttenuationFloor,
                             Confidence::Medium, Consequence::Continued, p, o,
                             "attenuated contribution is below the floor");

    // Step 15: corroboration. Require the configured number of independent
    // high-confidence evidence kinds before admitting.
    if (p.minimum_high_confidence_kinds > 0 &&
        high_confidence_kinds(o) < p.minimum_high_confidence_kinds)
        return make_decision(Verdict::Observe, Reason::Incomplete,
                             Confidence::Medium, Consequence::Continued, p, o,
                             "insufficient independent technical evidence");

    // Step 16: the only admitting branch. Reached only after every stop above
    // has been cleared. The consequence is "Continued": the object continues
    // under the same observed condition, now with admission.
    return make_decision(Verdict::Allow, Reason::None, Confidence::High,
                         Consequence::Continued, p, o,
                         "technical evidence satisfies the admission policy");
}

// Convenience wrapper preserving the historical name.
Decision evaluate_system_only(const Observation& o, const Policy& p) {
    FirewallReport report;
    return evaluate(o, p, &report);
}

// ===========================================================================
// SECTION 15 — Level escalation
// ===========================================================================
//
// The eight-level ladder is run as a sequence. The gate evaluates the object
// against every level from 1 up to a ceiling and reports the FIRST level that
// withholds admission. Because each level is stricter, the ladder can only
// stop earlier as scrutiny increases — it can never "recover" an Allow that a
// stricter level would deny.

struct LadderStep {
    Level level{Level::L1_Baseline};
    Decision decision;
};

struct LadderResult {
    std::vector<LadderStep> steps;
    Level reached{Level::L1_Baseline};
    bool admitted_through{false}; // admitted at every level up to the ceiling
    Decision final_decision;
};

LadderResult evaluate_ladder(const Observation& o, Level ceiling) {
    LadderResult result;
    const int top = static_cast<int>(ceiling);
    bool all_admitted = true;
    for (int n = 1; n <= top; ++n) {
        const Level level = static_cast<Level>(n);
        const Policy p = policy_for_level(level);
        FirewallReport report;
        Decision d = evaluate(o, p, &report);
        result.steps.push_back(LadderStep{level, d});
        result.reached = level;
        result.final_decision = d;
        if (!d.admits()) {
            all_admitted = false;
            break; // stop at the first withholding level
        }
    }
    result.admitted_through = all_admitted;
    return result;
}

// ===========================================================================
// SECTION 16 — Serialization
// ===========================================================================

static std::string serialize_evidence(const Evidence& e) {
    std::ostringstream out;
    out << "{\"id\":\"" << e.id()
        << "\",\"kind\":\"" << kind_name(e.kind)
        << "\",\"confidence\":\"" << confidence_name(e.confidence)
        << "\",\"source\":\"" << escape_json(e.source)
        << "\",\"detail\":\"" << escape_json(e.detail)
        << "\",\"origin\":\"" << escape_json(e.origin) << "\"}";
    return out.str();
}

std::string serialize(const Decision& d) {
    std::ostringstream out;
    out << "{\"object_id\":\"" << escape_json(d.object_id)
        << "\",\"level\":\"" << level_name(d.level)
        << "\",\"verdict\":\"" << verdict_name(d.verdict)
        << "\",\"reason\":\"" << reason_name(d.reason)
        << "\",\"confidence\":\"" << confidence_name(d.confidence)
        << "\",\"consequence\":\"" << consequence_name(d.consequence)
        << "\",\"effective_contribution\":" << d.effective_contribution
        << ",\"explanation\":\"" << escape_json(d.explanation)
        << "\",\"supporting\":[";
    bool first = true;
    for (const auto& e : d.supporting) {
        if (!first) out << ',';
        first = false;
        out << serialize_evidence(e);
    }
    out << "]}";
    return out.str();
}

std::string serialize_ladder(const LadderResult& r) {
    std::ostringstream out;
    out << "{\"reached\":\"" << level_name(r.reached)
        << "\",\"admitted_through\":" << (r.admitted_through ? "true" : "false")
        << ",\"steps\":[";
    bool first = true;
    for (const auto& s : r.steps) {
        if (!first) out << ',';
        first = false;
        out << "{\"level\":\"" << level_name(s.level)
            << "\",\"decision\":" << serialize(s.decision) << '}';
    }
    out << "]}";
    return out.str();
}

// ===========================================================================
// SECTION 17 — Audit ledger
// ===========================================================================
//
// Every decision the gate publishes is recorded in a monotonic, append-only
// ledger. Sequence numbers never repeat and never regress. The ledger is the
// artifact a reviewer inspects to reconstruct exactly what the gate decided
// and why, in order.

struct AuditRecord {
    uint64_t sequence{0};
    uint64_t timestamp_ms{0};
    Verdict verdict{Verdict::Error};
    Reason reason{Reason::Policy};
    Level level{Level::L1_Baseline};
    std::string object_id;
    std::string explanation;
    size_t firewall_stripped{0};
};

class AuditLedger {
public:
    explicit AuditLedger(size_t capacity = 65536) : capacity_(capacity) {}

    // Record a decision and return it unchanged so callers can chain.
    Decision record(const Decision& d, uint64_t now_ms = 0,
                    size_t firewall_stripped = 0) {
        AuditRecord r;
        r.sequence = ++sequence_;
        r.timestamp_ms = now_ms;
        r.verdict = d.verdict;
        r.reason = d.reason;
        r.level = d.level;
        r.object_id = d.object_id;
        r.explanation = d.explanation;
        r.firewall_stripped = firewall_stripped;
        if (records_.size() >= capacity_) records_.erase(records_.begin());
        records_.push_back(std::move(r));
        return d;
    }

    const std::vector<AuditRecord>& records() const { return records_; }
    uint64_t sequence() const { return sequence_; }
    bool empty() const { return records_.empty(); }

    // Verify the ledger's internal invariant: sequences strictly increase.
    bool consistent() const {
        for (size_t i = 1; i < records_.size(); ++i)
            if (records_[i].sequence <= records_[i - 1].sequence) return false;
        return true;
    }

    // Count how many records reached each terminal verdict — useful summary
    // for an operator herald at level 8.
    std::map<std::string, size_t> verdict_histogram() const {
        std::map<std::string, size_t> hist;
        for (const auto& r : records_)
            ++hist[verdict_name(r.verdict)];
        return hist;
    }

    std::string serialize() const {
        std::ostringstream out;
        out << "{\"sequence\":" << sequence_ << ",\"records\":[";
        bool first = true;
        for (const auto& r : records_) {
            if (!first) out << ',';
            first = false;
            out << "{\"sequence\":" << r.sequence
                << ",\"timestamp_ms\":" << r.timestamp_ms
                << ",\"level\":\"" << level_name(r.level)
                << "\",\"verdict\":\"" << verdict_name(r.verdict)
                << "\",\"reason\":\"" << reason_name(r.reason)
                << "\",\"object_id\":\"" << escape_json(r.object_id)
                << "\",\"firewall_stripped\":" << r.firewall_stripped
                << ",\"explanation\":\"" << escape_json(r.explanation)
                << "\"}";
        }
        out << "]}";
        return out.str();
    }

private:
    uint64_t sequence_{0};
    size_t capacity_;
    std::vector<AuditRecord> records_;
};

// ===========================================================================
// SECTION 18 — High-level gate facade
// ===========================================================================
//
// The Gate ties the pieces together: it runs the firewall, evaluates against
// a level (or the full ladder), records to the ledger, and exposes the JSON
// audit surface. This is the object a consuming build would embed.

class Gate {
public:
    explicit Gate(Level default_level = Level::L1_Baseline)
        : default_level_(default_level) {}

    // Evaluate one observation at the configured default level.
    Decision admit(const Observation& o, uint64_t now_ms = 0) {
        return admit_at(o, default_level_, now_ms);
    }

    // Evaluate at a specific level.
    Decision admit_at(const Observation& o, Level level, uint64_t now_ms = 0) {
        const Policy p = policy_for_level(level);
        FirewallReport report;
        Decision d = evaluate(o, p, &report);
        return ledger_.record(d, now_ms, report.stripped);
    }

    // Evaluate through the full ladder up to a ceiling; record the final step.
    LadderResult admit_ladder(const Observation& o, Level ceiling,
                              uint64_t now_ms = 0) {
        LadderResult r = evaluate_ladder(o, ceiling);
        FirewallReport report;
        (void)system_only(o, &report); // recompute stripped count for audit
        ledger_.record(r.final_decision, now_ms, report.stripped);
        return r;
    }

    const AuditLedger& ledger() const { return ledger_; }
    Level default_level() const { return default_level_; }
    void set_default_level(Level l) { default_level_ = l; }

    // A convenience predicate that honors safety rule 1: absence of a finding
    // is never safety. Only an explicit Allow is treated as admitting.
    static bool admits(const Decision& d) { return d.admits(); }

private:
    Level default_level_;
    AuditLedger ledger_;
};

// ===========================================================================
// SECTION 19 — Builders (ergonomic construction of observations)
// ===========================================================================
//
// Constructing an Observation by hand is verbose. These fluent builders keep
// call sites readable while ensuring the demographic firewall's contract is
// visible: an operator-origin evidence item must be explicitly tagged, so it
// is impossible to add one by accident.

class ObservationBuilder {
public:
    explicit ObservationBuilder(std::string object_id) {
        o_.object_id = std::move(object_id);
    }

    ObservationBuilder& path(std::string p) {
        o_.canonical_path = std::move(p);
        return *this;
    }
    ObservationBuilder& digest(std::string d) {
        o_.content_digest = std::move(d);
        return *this;
    }
    ObservationBuilder& size(uint64_t bytes) {
        o_.size_bytes = bytes;
        return *this;
    }
    ObservationBuilder& scanned(uint64_t bytes, uint64_t files) {
        o_.scanned_bytes = bytes;
        o_.scanned_files = files;
        return *this;
    }
    ObservationBuilder& depths(uint32_t recursion, uint32_t archive) {
        o_.recursion_depth = recursion;
        o_.archive_depth = archive;
        return *this;
    }
    ObservationBuilder& elapsed(uint64_t ms) {
        o_.elapsed_ms = ms;
        return *this;
    }
    ObservationBuilder& completed(bool c = true) {
        o_.scan_completed = c;
        return *this;
    }
    ObservationBuilder& signature(bool hit = true) {
        o_.signature_hit = hit;
        return *this;
    }
    ObservationBuilder& heuristic(bool hit = true) {
        o_.heuristic_hit = hit;
        return *this;
    }
    ObservationBuilder& scanner_error(bool err = true) {
        o_.scanner_error = err;
        return *this;
    }
    ObservationBuilder& transform_failed(bool f = true) {
        o_.transform_failed = f;
        return *this;
    }
    ObservationBuilder& permission_denied(bool d = true) {
        o_.permission_denied = d;
        return *this;
    }
    ObservationBuilder& continuity(bool prior, bool event, bool result) {
        o_.prior_state_recorded = prior;
        o_.intervening_event_recorded = event;
        o_.resulting_state_recorded = result;
        return *this;
    }
    ObservationBuilder& evidence(EvidenceKind kind, Confidence conf,
                                 std::string source, std::string detail) {
        o_.add_evidence(make_evidence(kind, conf, std::move(source),
                                      std::move(detail)));
        return *this;
    }
    // The ONLY way to introduce operator-origin evidence — explicit, visible,
    // and destined to be firewalled out before evaluation.
    ObservationBuilder& operator_assertion(std::string source,
                                           std::string detail) {
        Evidence e = make_evidence(EvidenceKind::Permission, Confidence::Low,
                                   std::move(source), std::move(detail));
        e.origin = "operator";
        o_.add_evidence(std::move(e));
        return *this;
    }
    ObservationBuilder& retro(std::string present, std::string prior,
                              bool known, Confidence linkage) {
        RetroDependency r;
        r.present_observation = std::move(present);
        r.prior_condition = std::move(prior);
        r.prior_condition_known = known;
        r.linkage = linkage;
        o_.add_retro(std::move(r));
        return *this;
    }
    ObservationBuilder& causal(CausalStage stage, std::string actor,
                               std::string detail, double weight = 1.0) {
        o_.causal.add(stage, std::move(actor), std::move(detail), weight);
        return *this;
    }

    Observation build() const { return o_; }

private:
    Observation o_;
};

// ===========================================================================
// SECTION 20 — Explanations and diagnostics
// ===========================================================================
//
// The gate's job is not only to decide but to explain. These helpers render a
// decision into prose a reviewer can read without parsing JSON, and expose the
// mapping between reasons and the safety rules they enforce.

static const char* safety_rule_for_reason(Reason r) {
    switch (r) {
    case Reason::Signature:
        return "rule 2: never replace a ClamAV detection with a gate result";
    case Reason::ScannerError:
        return "rule 1: absence of a finding is not proof of safety";
    case Reason::Incomplete:
    case Reason::AttenuationFloor:
        return "rule 3: preserve uncertainty when observation is incomplete";
    case Reason::ProvenanceGap:
        return "rule 4: preserve provenance where available";
    case Reason::ClosureMissing:
    case Reason::CausalBreak:
        return "rule 5: require explicit closure at the closure levels";
    default:
        return "rule 6: keep detection, provenance, and responsibility distinct";
    }
}

std::string explain(const Decision& d) {
    std::ostringstream out;
    out << "object=" << (d.object_id.empty() ? "<none>" : d.object_id)
        << " level=" << level_name(d.level)
        << " verdict=" << verdict_name(d.verdict)
        << " reason=" << reason_name(d.reason)
        << " confidence=" << confidence_name(d.confidence)
        << " consequence=" << consequence_name(d.consequence)
        << "\n  " << d.explanation
        << "\n  enforcing: " << safety_rule_for_reason(d.reason);
    return out.str();
}

// A self-check that a decision never contradicts the six safety rules. Used
// by tests and available to a consuming build as a runtime assertion.
bool decision_respects_rules(const Decision& d, const Observation& o) {
    // Rule 2: a signature hit must never yield Allow.
    if (o.signature_hit && d.verdict == Verdict::Allow) return false;
    // Rule 1/3: a scanner error or incompleteness must never yield Allow.
    if ((o.scanner_error || (!o.scan_completed)) && d.verdict == Verdict::Allow)
        return false;
    // An Allow must carry at least High confidence.
    if (d.verdict == Verdict::Allow && d.confidence < Confidence::High)
        return false;
    return true;
}

// ===========================================================================
// SECTION 21 — Configuration parsing (minimal, dependency-free)
// ===========================================================================
//
// The level config.json files are flat objects of primitives. Rather than
// pull in a JSON library, the gate reads the handful of keys it cares about
// with a tiny scanner. This keeps the translation unit self-contained and
// reviewable. Unknown keys are ignored; malformed input fails closed to the
// baseline policy.

class MiniConfig {
public:
    // Parse a flat JSON-ish object. Only string, number, and boolean scalar
    // values are recognized. This is intentionally not a general JSON parser.
    static std::map<std::string, std::string> parse(std::string_view text) {
        std::map<std::string, std::string> kv;
        size_t i = 0;
        const size_t n = text.size();
        auto skip_ws = [&]() {
            while (i < n && (text[i] == ' ' || text[i] == '\t' ||
                             text[i] == '\n' || text[i] == '\r'))
                ++i;
        };
        auto read_string = [&](std::string& out) -> bool {
            if (i >= n || text[i] != '\"') return false;
            ++i;
            std::ostringstream s;
            while (i < n && text[i] != '\"') {
                if (text[i] == '\\' && i + 1 < n) {
                    ++i;
                    s << text[i];
                } else {
                    s << text[i];
                }
                ++i;
            }
            if (i >= n) return false;
            ++i; // closing quote
            out = s.str();
            return true;
        };
        auto read_scalar = [&](std::string& out) {
            std::ostringstream s;
            while (i < n && text[i] != ',' && text[i] != '}' &&
                   text[i] != ' ' && text[i] != '\n' && text[i] != '\r' &&
                   text[i] != '\t') {
                s << text[i];
                ++i;
            }
            out = s.str();
        };
        skip_ws();
        if (i < n && text[i] == '{') ++i;
        while (i < n) {
            skip_ws();
            if (i < n && text[i] == '}') break;
            std::string key;
            if (!read_string(key)) break;
            skip_ws();
            if (i < n && text[i] == ':') ++i;
            skip_ws();
            std::string value;
            if (i < n && text[i] == '\"') {
                if (!read_string(value)) break;
            } else if (i < n && text[i] == '[') {
                // Skip arrays wholesale; the gate does not read them here.
                int depth = 0;
                do {
                    if (text[i] == '[') ++depth;
                    else if (text[i] == ']') --depth;
                    ++i;
                } while (i < n && depth > 0);
                value = "[array]";
            } else {
                read_scalar(value);
            }
            kv[trim(key)] = trim(value);
            skip_ws();
            if (i < n && text[i] == ',') ++i;
        }
        return kv;
    }

    // Build a policy from a parsed config, starting from the level baseline
    // and only ever tightening. Unknown or malformed keys leave the baseline
    // intact — the gate never loosens itself from a config file.
    static Policy to_policy(const std::map<std::string, std::string>& kv) {
        Level level = Level::L1_Baseline;
        auto it = kv.find("level");
        if (it != kv.end()) {
            const int n = std::atoi(it->second.c_str());
            if (n >= 1 && n <= 8) level = static_cast<Level>(n);
        }
        Policy p = policy_for_level(level);

        auto truthy = [](const std::string& v) {
            return v == "true" || v == "1" || v == "yes";
        };
        // Config may only make the gate MORE cautious. We therefore OR the
        // cautious flags on; we never turn a cautious flag off from config.
        if (auto k = kv.find("fail_closed"); k != kv.end() && truthy(k->second))
            p.fail_closed_on_scanner_error = true;
        if (auto k = kv.find("require_provenance");
            k != kv.end() && truthy(k->second))
            p.require_provenance = true;
        if (auto k = kv.find("uncertainty_gate");
            k != kv.end() && truthy(k->second) && p.attenuation_floor < 0.25)
            p.attenuation_floor = 0.25;
        if (auto k = kv.find("explicit_closure");
            k != kv.end() && truthy(k->second))
            p.require_explicit_closure = true;
        if (auto k = kv.find("operator_closure");
            k != kv.end() && truthy(k->second))
            p.require_explicit_closure = true;
        return p;
    }
};

// ===========================================================================
// SECTION 22 — Invariant self-tests
// ===========================================================================
//
// A consuming build can call run_self_tests() at startup to assert that the
// gate honors its invariants on a battery of synthetic observations. These
// are pure and deterministic. They return the number of failures; zero means
// every invariant held.

namespace selftest {

static int expect(bool condition, int& failures) {
    if (!condition) ++failures;
    return failures;
}

// Rule 2: a ClamAV signature hit can never become Allow, at any level.
static void test_signature_never_allows(int& failures) {
    for (int n = 1; n <= 8; ++n) {
        Observation o = ObservationBuilder("obj-sig")
                            .path("bin/sample")
                            .completed(true)
                            .signature(true)
                            .build();
        const Policy p = policy_for_level(static_cast<Level>(n));
        const Decision d = evaluate(o, p);
        expect(d.verdict != Verdict::Allow, failures);
        expect(decision_respects_rules(d, o), failures);
    }
}

// Rule 1: a scanner error fails closed at levels that demand it.
static void test_scanner_error_fails_closed(int& failures) {
    Observation o = ObservationBuilder("obj-err")
                        .path("bin/sample")
                        .scanner_error(true)
                        .build();
    const Policy p = policy_for_level(Level::L3_Tracing);
    const Decision d = evaluate(o, p);
    expect(d.verdict == Verdict::Error, failures);
}

// The firewall strips operator-origin evidence and never lets it influence
// the verdict.
static void test_firewall_strips_operator(int& failures) {
    Observation o = ObservationBuilder("obj-fw")
                        .path("bin/sample")
                        .completed(true)
                        .evidence(EvidenceKind::Provenance, Confidence::High,
                                  "updater", "signed cvd 27000")
                        .evidence(EvidenceKind::Signature, Confidence::High,
                                  "clamav", "clean")
                        .operator_assertion("operator", "trust me")
                        .build();
    FirewallReport report;
    const Policy p = policy_for_level(Level::L2_SecondLook);
    const Decision d = evaluate(o, p, &report);
    expect(report.stripped == 1, failures);
    // The operator assertion must not appear in the supporting evidence.
    for (const auto& e : d.supporting)
        expect(!e.is_operator_origin(), failures);
}

// A path escape is always a Reject.
static void test_path_escape_rejects(int& failures) {
    Observation o = ObservationBuilder("obj-esc")
                        .path("../../etc/passwd")
                        .completed(true)
                        .build();
    const Policy p = policy_for_level(Level::L1_Baseline);
    const Decision d = evaluate(o, p);
    expect(d.verdict == Verdict::Reject, failures);
    expect(d.reason == Reason::PathBoundary, failures);
}

// The ladder can only stop earlier as scrutiny rises: if level k denies, no
// level > k is consulted, and the final verdict is not Allow.
static void test_ladder_monotone(int& failures) {
    // An object with no provenance passes level 1-2 but is held at level 3.
    Observation o = ObservationBuilder("obj-ladder")
                        .path("bin/sample")
                        .completed(true)
                        .evidence(EvidenceKind::Signature, Confidence::High,
                                  "clamav", "clean")
                        .build();
    const LadderResult r = evaluate_ladder(o, Level::L8_OperatorHerald);
    // It should not be admitted through all eight levels without provenance
    // and a well-formed causal chain.
    expect(!r.admitted_through, failures);
}

// A fully-qualified object clears the baseline levels.
static void test_qualified_object_allows_low_levels(int& failures) {
    Observation o = ObservationBuilder("obj-ok")
                        .path("bin/sample")
                        .completed(true)
                        .evidence(EvidenceKind::Signature, Confidence::High,
                                  "clamav", "clean")
                        .build();
    const Policy p1 = policy_for_level(Level::L1_Baseline);
    const Decision d1 = evaluate(o, p1);
    expect(d1.verdict == Verdict::Allow, failures);
}

// The audit ledger keeps a strictly increasing sequence.
static void test_ledger_monotone(int& failures) {
    Gate gate(Level::L1_Baseline);
    Observation o = ObservationBuilder("obj-a")
                        .path("bin/a")
                        .completed(true)
                        .evidence(EvidenceKind::Signature, Confidence::High,
                                  "clamav", "clean")
                        .build();
    gate.admit(o, 100);
    gate.admit(o, 200);
    gate.admit(o, 300);
    expect(gate.ledger().consistent(), failures);
    expect(gate.ledger().sequence() == 3, failures);
}

// The config parser reads a level and never loosens the gate.
static void test_config_only_tightens(int& failures) {
    const auto kv = MiniConfig::parse(
        "{\"level\":8,\"fail_closed\":true,\"operator_closure\":true}");
    const Policy p = MiniConfig::to_policy(kv);
    expect(p.level == Level::L8_OperatorHerald, failures);
    expect(p.fail_closed_on_scanner_error, failures);
    expect(p.require_explicit_closure, failures);
}

// The continuity test classifies a first observation as Created and a full
// before/event/after with a detection as Terminated.
static void test_continuity_classification(int& failures) {
    Observation first = ObservationBuilder("obj-first").path("bin/x").build();
    const ContinuityResult a = continuity_test(first);
    expect(a.consequence == Consequence::Created, failures);

    Observation full = ObservationBuilder("obj-full")
                           .path("bin/x")
                           .continuity(true, true, true)
                           .signature(true)
                           .build();
    const ContinuityResult b = continuity_test(full);
    expect(b.consequence == Consequence::Terminated, failures);
}

// A transform failure quarantines because the content underneath was unseen.
static void test_transform_failure_quarantines(int& failures) {
    Observation o = ObservationBuilder("obj-tf")
                        .path("bin/x")
                        .transform_failed(true)
                        .build();
    const Policy p = policy_for_level(Level::L1_Baseline);
    const Decision d = evaluate(o, p);
    expect(d.verdict == Verdict::Quarantine, failures);
}

// A resource-budget overrun quarantines.
static void test_resource_overrun_quarantines(int& failures) {
    Observation o = ObservationBuilder("obj-big")
                        .path("bin/x")
                        .completed(true)
                        .depths(9999, 0) // far beyond max_recursion
                        .build();
    const Policy p = policy_for_level(Level::L1_Baseline);
    const Decision d = evaluate(o, p);
    expect(d.verdict == Verdict::Quarantine, failures);
    expect(d.reason == Reason::ResourceLimit, failures);
}

// Level 5 withholds admission when the causal chain is not well-formed.
static void test_level5_requires_causal_chain(int& failures) {
    Observation o = ObservationBuilder("obj-nochain")
                        .path("bin/x")
                        .completed(true)
                        .continuity(true, true, true)
                        .evidence(EvidenceKind::Signature, Confidence::High,
                                  "clamav", "clean")
                        .evidence(EvidenceKind::Provenance, Confidence::High,
                                  "updater", "signed")
                        .build();
    const Policy p = policy_for_level(Level::L5_Causal);
    const Decision d = evaluate(o, p);
    // No causal chain -> not well-formed -> Observe (CausalBreak).
    expect(d.verdict == Verdict::Observe, failures);
    expect(d.reason == Reason::CausalBreak, failures);
}

} // namespace selftest

int run_self_tests() {
    int failures = 0;
    selftest::test_signature_never_allows(failures);
    selftest::test_scanner_error_fails_closed(failures);
    selftest::test_firewall_strips_operator(failures);
    selftest::test_path_escape_rejects(failures);
    selftest::test_ladder_monotone(failures);
    selftest::test_qualified_object_allows_low_levels(failures);
    selftest::test_ledger_monotone(failures);
    selftest::test_config_only_tightens(failures);
    selftest::test_continuity_classification(failures);
    selftest::test_transform_failure_quarantines(failures);
    selftest::test_resource_overrun_quarantines(failures);
    selftest::test_level5_requires_causal_chain(failures);
    return failures;
}

} // namespace legal_clam_gate

// ===========================================================================
// SECTION 23 — Optional standalone entry point
// ===========================================================================
//
// When compiled with -DLEGAL_CLAM_GATE_MAIN this file becomes a tiny
// self-test driver, mirroring the numbered levels' test.c convention. In an
// ordinary build the symbol is absent and the file is a pure library.

#ifdef LEGAL_CLAM_GATE_MAIN
#include <iostream>
int main() {
    const int failures = legal_clam_gate::run_self_tests();
    std::cout << "legal_clam_gate self-tests: "
              << (failures == 0 ? "all passed" : "FAILURES")
              << " (" << failures << ")\n";

    // Demonstrate the audit surface on a small ladder run.
    using namespace legal_clam_gate;
    Observation o = ObservationBuilder("demo-object")
                        .path("bin/demo")
                        .size(4096)
                        .scanned(4096, 1)
                        .depths(1, 0)
                        .elapsed(12)
                        .completed(true)
                        .continuity(true, true, true)
                        .evidence(EvidenceKind::Signature, Confidence::High,
                                  "clamav", "clean")
                        .evidence(EvidenceKind::Provenance, Confidence::High,
                                  "updater", "signed cvd 27000")
                        .evidence(EvidenceKind::Dependency, Confidence::High,
                                  "loader", "libc 2.39 resolved")
                        .causal(CausalStage::Root, "acquisition", "download")
                        .causal(CausalStage::Method, "installer", "unpack")
                        .causal(CausalStage::Cause, "updater", "db refresh")
                        .causal(CausalStage::Attention, "engine", "full scan")
                        .causal(CausalStage::Attenuation, "engine",
                                "no gaps", 0.9)
                        .causal(CausalStage::Closure, "engine", "verdict")
                        .build();
    const LadderResult r = evaluate_ladder(o, Level::L8_OperatorHerald);
    std::cout << serialize_ladder(r) << "\n";

    // Also show a single-level explanation for a signature-hit object so the
    // fail-closed behavior is visible in the demo output.
    Observation dirty = ObservationBuilder("dirty-object")
                            .path("bin/dirty")
                            .completed(true)
                            .signature(true)
                            .build();
    const Decision dd = evaluate(dirty, policy_for_level(Level::L1_Baseline));
    std::cout << explain(dd) << "\n";
    return failures == 0 ? 0 : 1;
}
#endif

/*
 * ---------------------------------------------------------------------------
 * DESIGN SUMMARY (why this file is shaped the way it is)
 * ---------------------------------------------------------------------------
 * The gate DECIDES; the herald (herald.cpp) NARRATES. They are separate
 * translation units so that their authorities never blur, which is safety
 * rule 6 realized at the level of software architecture.
 *
 * Every branch in evaluate() (Section 14) exists to enforce one of the six
 * safety rules, and the order of branches is itself a safety property: the
 * function is written so that it can only ever become MORE cautious as control
 * flows downward. The single admitting branch (Step 16) is reachable only
 * after every fail-closed and withholding condition above it has been cleared.
 *
 *   - A ClamAV signature hit (Step 8) can never become Allow (rule 2).
 *   - A scanner error (Step 6) fails closed where policy demands it (rule 1).
 *   - Incompleteness, attenuation floors, and insufficient corroboration
 *     (Steps 7, 14, 15) preserve uncertainty rather than resolving it (rule 3).
 *   - Provenance requirements and retro-dependency gaps (Step 10) preserve and
 *     demand provenance where available (rule 4).
 *   - The causal-chain and explicit-closure requirements (Steps 12, 13)
 *     enforce closure at the closure levels (rule 5).
 *   - The demographic firewall (Section 10, Step 0) strips operator/
 *     demographic evidence before any reasoning occurs (rule 6).
 *
 * The eight-level escalation ladder (Section 15) can only ever stop earlier as
 * scrutiny rises; policy_for_level() (Section 8) is constructed so that each
 * higher level inherits and tightens the prior. No configuration file may
 * loosen the gate: MiniConfig::to_policy() (Section 21) only ORs cautious
 * flags on.
 *
 * The audit ledger (Section 17) records every published decision with a
 * strictly increasing sequence, so a reviewer can reconstruct exactly what the
 * gate decided and why, in order. That reviewability — not any single verdict —
 * is the point of the whole layer.
 *
 * Author: Max Rupplin - MEARVK LLC - 2026.
 */
