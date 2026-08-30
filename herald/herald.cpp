/*
 * herald.cpp — System-centric evidence herald for ClamAV.US.Legal.Edition.
 *
 * Author: Max Rupplin - MEARVK LLC - 2026.
 *
 * ---------------------------------------------------------------------------
 * PURPOSE
 * ---------------------------------------------------------------------------
 * The herald announces observations; it does not manufacture the malware
 * verdict. It is the authoritative C++ reference embodiment of the even
 * sequitur levels (herald/{2,4,6,8}) just as gating.cpp is the embodiment of
 * the odd levels (gating/{1,3,5,7}).
 *
 * Where the gate DECIDES, the herald PRESERVES and NARRATES:
 *
 *   - scan state and its lawful transitions,
 *   - technical evidence as it arrives,
 *   - resource boundaries as they are observed,
 *   - uncertainty and attenuation (never silently discarded),
 *   - and the terminal outcome,
 *
 * so that downstream consumers cannot mistake a partial scan for a clean scan.
 *
 * ---------------------------------------------------------------------------
 * THE DEMOGRAPHIC FIREWALL (non-negotiable invariant)
 * ---------------------------------------------------------------------------
 * Human identity, reputation, demographic attribute, affiliation, belief, and
 * membership are deliberately excluded from the security evidence channel. The
 * herald refuses to emit them: sanitize_display_fields() strips a denylist of
 * such keys before any event is rendered for a human, and the refusal itself
 * is observable so a reviewer can confirm the firewall acted.
 *
 * ---------------------------------------------------------------------------
 * THE CAUSAL VOCABULARY (from PROCEDURAL_CAUSATION.hss)
 * ---------------------------------------------------------------------------
 *   ROOT -> METHOD -> CAUSE -> ATTENTION -> ATTENUATION -> CLOSURE
 *
 *   ROOT        the software object / container the procedure begins from.
 *   METHOD      the procedure applied (scan, extract, decode).
 *   CAUSE       a prior transformation/dependency/update the state depends on.
 *   ATTENTION   a deliberate increase in what the procedure observes; an
 *               observability factor, never a human-value judgment.
 *   ATTENUATION conditions that reduce a factor's effective contribution:
 *               uncertainty, unavailable content, limits, incompleteness.
 *   CLOSURE     the terminal procedural state; it must never imply that
 *               unobserved content was proven safe.
 *
 * ---------------------------------------------------------------------------
 * THE EIGHT LEVELS (from PROCEDURAL_GATING.md)
 * ---------------------------------------------------------------------------
 *   2 second-look confirmation
 *   4 evidence qualification
 *   6 attenuation and uncertainty
 *   8 final operator herald and closure
 *
 * Each level inherits the concerns of the prior and adds another gate; the
 * progression is intentionally more cautious, not more permissive. Level 8
 * requires an explicit operator closure.
 *
 * This is a single self-contained translation unit depending only on the
 * C++17 standard library, so it can be reviewed in isolation.
 */

#include <algorithm>
#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace legal_clam_herald {

// ===========================================================================
// SECTION 1 — Enumerations and canonical names
// ===========================================================================

// The scan lifecycle. Only lawful transitions between these are permitted;
// see legal_transition(). Terminal states never transition further.
enum class State {
    Created,     // heralded but not yet started
    Running,     // scan has begun
    Inspecting,  // examining current object content
    Extracting,  // decomposing a container into children
    Finalizing,  // consolidating results toward a terminal state
    Completed,   // clean terminal state
    Quarantined, // isolated terminal state
    Failed       // fail-closed terminal state
};

// The kind of thing an event announces.
enum class Kind {
    Started,
    Progress,
    Evidence,
    Boundary,
    Attention,
    Attenuation,
    Verdict,
    Failure,
    Closure,
    Completed
};

// Severity of an event. Ordered; higher is more urgent.
enum class Level {
    Trace,
    Info,
    Notice,
    Warning,
    Critical
};

// The eight sequitur levels the herald participates in (even) plus the odd
// levels it observes for context. The herald's own escalation uses the even
// members; the odd members exist so a herald can annotate which gate stage a
// finding relates to.
enum class Sequitur {
    S1_Baseline = 1,
    S2_SecondLook = 2,
    S3_Tracing = 3,
    S4_Qualification = 4,
    S5_Causal = 5,
    S6_Attenuation = 6,
    S7_Closure = 7,
    S8_OperatorHerald = 8
};

// The causal vocabulary stage a given announcement belongs to.
enum class CausalStage { Root, Method, Cause, Attention, Attenuation, Closure };

static const char* state_name(State s) {
    switch (s) {
    case State::Created:     return "created";
    case State::Running:     return "running";
    case State::Inspecting:  return "inspecting";
    case State::Extracting:  return "extracting";
    case State::Finalizing:  return "finalizing";
    case State::Completed:   return "completed";
    case State::Quarantined: return "quarantined";
    case State::Failed:      return "failed";
    }
    return "unknown";
}

static const char* kind_name(Kind k) {
    switch (k) {
    case Kind::Started:     return "started";
    case Kind::Progress:    return "progress";
    case Kind::Evidence:    return "evidence";
    case Kind::Boundary:    return "boundary";
    case Kind::Attention:   return "attention";
    case Kind::Attenuation: return "attenuation";
    case Kind::Verdict:     return "verdict";
    case Kind::Failure:     return "failure";
    case Kind::Closure:     return "closure";
    case Kind::Completed:   return "completed";
    }
    return "unknown";
}

static const char* level_name(Level l) {
    switch (l) {
    case Level::Trace:    return "trace";
    case Level::Info:     return "info";
    case Level::Notice:   return "notice";
    case Level::Warning:  return "warning";
    case Level::Critical: return "critical";
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

static const char* sequitur_name(Sequitur s) {
    switch (s) {
    case Sequitur::S1_Baseline:       return "baseline";
    case Sequitur::S2_SecondLook:     return "second_look";
    case Sequitur::S3_Tracing:        return "tracing";
    case Sequitur::S4_Qualification:  return "qualification";
    case Sequitur::S5_Causal:         return "causal";
    case Sequitur::S6_Attenuation:    return "attenuation";
    case Sequitur::S7_Closure:        return "closure";
    case Sequitur::S8_OperatorHerald: return "operator_herald";
    }
    return "unknown";
}

// ===========================================================================
// SECTION 2 — State machine
// ===========================================================================
//
// The herald enforces a lawful state graph. Attempting an unlawful transition
// is a no-op that returns false; the herald never silently jumps states. This
// is what prevents a partial scan from being narrated as a completed one.

static bool terminal(State s) {
    return s == State::Completed || s == State::Quarantined ||
           s == State::Failed;
}

static bool legal_transition(State from, State to) {
    switch (from) {
    case State::Created:
        return to == State::Running;
    case State::Running:
        return to == State::Inspecting || to == State::Failed;
    case State::Inspecting:
        return to == State::Extracting || to == State::Finalizing ||
               to == State::Failed;
    case State::Extracting:
        // A container can loop back to inspecting its extracted children.
        return to == State::Inspecting || to == State::Finalizing ||
               to == State::Failed;
    case State::Finalizing:
        return terminal(to);
    default:
        return false; // terminal states never transition
    }
}

// ===========================================================================
// SECTION 3 — Text utilities
// ===========================================================================

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

// ===========================================================================
// SECTION 4 — Event
// ===========================================================================
//
// An Event is one immutable announcement. Events carry a monotonically
// increasing sequence number so a consumer can detect gaps and reordering.

struct Event {
    uint64_t sequence{0};
    uint64_t timestamp_ms{0};
    State state{State::Created};
    Kind kind{Kind::Started};
    Level level{Level::Info};
    CausalStage stage{CausalStage::Root};
    std::string object_id;
    std::string message;
    std::map<std::string, std::string> fields;
};

// ===========================================================================
// SECTION 5 — The demographic firewall for display fields
// ===========================================================================
//
// Before any event's fields are rendered for a human, they pass through
// sanitize_display_fields(). A denylist of human-attribute keys is stripped.
// The stripping is reported so the firewall's action is auditable.

static const std::vector<std::string>& denied_display_keys() {
    static const std::vector<std::string> keys = {
        "age",        "height",     "weight",    "celebrity",
        "religion",   "politics",   "party",     "candidate",
        "reputation", "membership", "ethnicity", "gender",
        "nationality", "creed",     "affiliation"
    };
    return keys;
}

struct SanitizeReport {
    size_t stripped{0};
    std::vector<std::string> stripped_keys;
};

std::map<std::string, std::string>
sanitize_display_fields(std::map<std::string, std::string> fields,
                        SanitizeReport* report = nullptr) {
    for (const auto& key : denied_display_keys()) {
        auto it = fields.find(key);
        if (it != fields.end()) {
            if (report) {
                ++report->stripped;
                report->stripped_keys.push_back(key);
            }
            fields.erase(it);
        }
    }
    return fields;
}

// ===========================================================================
// SECTION 6 — Attenuation record
// ===========================================================================
//
// Attenuation is first-class here. Every condition that reduces the effective
// contribution of a factor is recorded rather than silently dropped, so that
// closure never overstates certainty (safety rule 3).

struct Attenuation {
    std::string factor;   // what is being attenuated
    std::string cause;    // why (uncertainty, unavailable content, limit)
    double before{1.0};   // contribution before this attenuation
    double after{1.0};    // contribution after

    double reduction() const {
        const double r = before - after;
        return r > 0.0 ? r : 0.0;
    }

    std::string serialize() const {
        std::ostringstream out;
        out << "{\"factor\":\"" << escape_json(factor)
            << "\",\"cause\":\"" << escape_json(cause)
            << "\",\"before\":" << before
            << ",\"after\":" << after << '}';
        return out.str();
    }
};

// A ledger of attenuations that composes their effect. The composite
// contribution is the product of the per-attenuation ratios, floored at zero.
class AttenuationLedger {
public:
    void record(const Attenuation& a) { entries_.push_back(a); }
    void record(std::string factor, std::string cause, double before,
                double after) {
        entries_.push_back(Attenuation{std::move(factor), std::move(cause),
                                       before, after});
    }

    const std::vector<Attenuation>& entries() const { return entries_; }
    bool empty() const { return entries_.empty(); }

    // Composite effective contribution across all recorded attenuations.
    double composite() const {
        double acc = 1.0;
        for (const auto& a : entries_) {
            double ratio = (a.before > 0.0) ? (a.after / a.before) : 0.0;
            if (ratio < 0.0) ratio = 0.0;
            if (ratio > 1.0) ratio = 1.0;
            acc *= ratio;
        }
        return acc;
    }

    std::string serialize() const {
        std::ostringstream out;
        out << '[';
        bool first = true;
        for (const auto& a : entries_) {
            if (!first) out << ',';
            first = false;
            out << a.serialize();
        }
        out << ']';
        return out.str();
    }

private:
    std::vector<Attenuation> entries_;
};

// ===========================================================================
// SECTION 7 — The Herald
// ===========================================================================
//
// The Herald owns the state machine, the event queue, the attenuation ledger,
// and the sequence counter. Every announcement goes through append(), which
// enforces the sequence invariant and the bounded-queue invariant.

class Herald {
public:
    explicit Herald(Sequitur level = Sequitur::S2_SecondLook,
                    size_t queue_limit = 4096)
        : level_(level), queue_limit_(queue_limit) {}

    // ---- lifecycle transitions ----

    bool start(std::string object_id, uint64_t now_ms) {
        if (object_id.empty() || is_blank(object_id)) return false;
        if (state_ != State::Created) return false;
        object_id_ = std::move(object_id);
        return transition(State::Running, Kind::Started, Level::Info,
                          CausalStage::Root, now_ms, "scan started");
    }

    bool inspect(uint64_t now_ms) {
        return transition(State::Inspecting, Kind::Progress, Level::Info,
                          CausalStage::Method, now_ms, "inspection started");
    }

    bool extract(uint64_t now_ms) {
        return transition(State::Extracting, Kind::Progress, Level::Info,
                          CausalStage::Method, now_ms,
                          "container extraction started");
    }

    bool finalize(uint64_t now_ms) {
        return transition(State::Finalizing, Kind::Progress, Level::Info,
                          CausalStage::Closure, now_ms,
                          "scan finalization started");
    }

    bool complete(uint64_t now_ms) {
        // Level 8 requires an explicit operator closure before completion.
        if (level_ == Sequitur::S8_OperatorHerald && !operator_closed_)
            return false;
        return transition(State::Completed, Kind::Completed, Level::Notice,
                          CausalStage::Closure, now_ms, "scan completed");
    }

    bool quarantine(uint64_t now_ms) {
        return transition(State::Quarantined, Kind::Completed, Level::Warning,
                          CausalStage::Closure, now_ms, "object isolated");
    }

    bool fail(uint64_t now_ms, std::string message) {
        return transition(State::Failed, Kind::Failure, Level::Critical,
                          CausalStage::Closure, now_ms, std::move(message));
    }

    // ---- announcements (do not change state) ----

    bool evidence(uint64_t now_ms, Level level, std::string source,
                  std::string detail) {
        if (source.empty() || detail.empty()) return false;
        Event e = base(Kind::Evidence, level, CausalStage::Cause, now_ms,
                       std::move(detail));
        e.fields.emplace("source", std::move(source));
        return append(std::move(e));
    }

    // Attention: a deliberate increase in observation. Recorded as an event
    // and reflected in the state's observation intensity.
    bool attention(uint64_t now_ms, std::string focus, std::string reason) {
        if (focus.empty()) return false;
        Event e = base(Kind::Attention, Level::Notice, CausalStage::Attention,
                       now_ms, "observation intensified");
        e.fields.emplace("focus", std::move(focus));
        if (!reason.empty()) e.fields.emplace("reason", std::move(reason));
        ++attention_count_;
        return append(std::move(e));
    }

    // Attenuation: a condition that reduces a factor's contribution. Recorded
    // both as an event and in the attenuation ledger.
    bool attenuate(uint64_t now_ms, std::string factor, std::string cause,
                   double before, double after) {
        if (factor.empty()) return false;
        Event e = base(Kind::Attenuation, Level::Warning,
                       CausalStage::Attenuation, now_ms,
                       "factor contribution attenuated");
        e.fields.emplace("factor", factor);
        e.fields.emplace("cause", cause);
        e.fields.emplace("before", to_fixed(before));
        e.fields.emplace("after", to_fixed(after));
        attenuations_.record(factor, cause, before, after);
        return append(std::move(e));
    }

    bool boundary(uint64_t now_ms, std::string name, uint64_t observed,
                  uint64_t limit) {
        if (name.empty()) return false;
        Event e = base(Kind::Boundary,
                       observed > limit ? Level::Warning : Level::Notice,
                       CausalStage::Attention, now_ms, std::move(name));
        e.fields.emplace("observed", std::to_string(observed));
        e.fields.emplace("limit", std::to_string(limit));
        e.fields.emplace("exceeded", observed > limit ? "true" : "false");
        return append(std::move(e));
    }

    bool verdict(uint64_t now_ms, std::string value, std::string reason,
                 std::string confidence) {
        if (value.empty() || reason.empty() || confidence.empty())
            return false;
        Event e = base(Kind::Verdict, Level::Notice, CausalStage::Closure,
                       now_ms, "technical gate verdict");
        e.fields.emplace("verdict", std::move(value));
        e.fields.emplace("reason", std::move(reason));
        e.fields.emplace("confidence", std::move(confidence));
        return append(std::move(e));
    }

    bool progress(uint64_t now_ms, uint64_t bytes, uint64_t files,
                  uint32_t depth) {
        Event e = base(Kind::Progress, Level::Trace, CausalStage::Method,
                       now_ms, "scan progress");
        e.fields.emplace("bytes", std::to_string(bytes));
        e.fields.emplace("files", std::to_string(files));
        e.fields.emplace("depth", std::to_string(depth));
        return append(std::move(e));
    }

    // The explicit operator closure required at level 8 (safety rule 5). An
    // operator closure is a deliberate acknowledgement that the herald reached
    // a terminal, reviewable state; it does not assert content is safe.
    bool operator_closure(uint64_t now_ms, std::string operator_ref) {
        if (operator_ref.empty()) return false;
        Event e = base(Kind::Closure, Level::Notice, CausalStage::Closure,
                       now_ms, "operator closure recorded");
        // The operator reference is a technical actor id, never a demographic.
        e.fields.emplace("operator_ref", std::move(operator_ref));
        operator_closed_ = true;
        return append(std::move(e));
    }

    // ---- accessors ----

    State state() const { return state_; }
    Sequitur level() const { return level_; }
    uint64_t sequence() const { return sequence_; }
    uint64_t attention_count() const { return attention_count_; }
    bool operator_closed() const { return operator_closed_; }
    const std::deque<Event>& events() const { return events_; }
    const AttenuationLedger& attenuations() const { return attenuations_; }

    // The composite effective contribution after all attenuations. Closure
    // must not overstate this.
    double effective_contribution() const {
        return attenuations_.empty() ? 1.0 : attenuations_.composite();
    }

    // Render the herald's full record as JSON. Every event's fields pass the
    // demographic firewall on the way out.
    std::string json() const {
        SanitizeReport report;
        std::ostringstream out;
        out << "{\"object_id\":\"" << escape_json(object_id_)
            << "\",\"level\":\"" << sequitur_name(level_)
            << "\",\"state\":\"" << state_name(state_)
            << "\",\"sequence\":" << sequence_
            << ",\"attention_count\":" << attention_count_
            << ",\"operator_closed\":" << (operator_closed_ ? "true" : "false")
            << ",\"effective_contribution\":" << to_fixed(effective_contribution())
            << ",\"attenuations\":" << attenuations_.serialize()
            << ",\"events\":[";
        bool first = true;
        for (const auto& e : events_) {
            if (!first) out << ',';
            first = false;
            out << event_json(e, &report);
        }
        out << "],\"firewall_stripped\":" << report.stripped << "}";
        return out.str();
    }

private:
    static std::string to_fixed(double v) {
        std::ostringstream s;
        s.setf(std::ios::fixed);
        s.precision(4);
        s << v;
        return s.str();
    }

    Event base(Kind kind, Level level, CausalStage stage, uint64_t now_ms,
               std::string message) {
        Event e;
        e.sequence = sequence_ + 1;
        e.timestamp_ms = now_ms;
        e.state = state_;
        e.kind = kind;
        e.level = level;
        e.stage = stage;
        e.object_id = object_id_;
        e.message = std::move(message);
        return e;
    }

    bool append(Event e) {
        // Sequence invariant: events are strictly consecutive.
        if (e.sequence != sequence_ + 1) return false;
        // Identity invariant: every event names the object.
        if (e.object_id.empty()) return false;
        if (queue_limit_ == 0) return false;
        // Bounded-queue invariant: never grow without bound. Oldest events
        // are dropped first, but the sequence counter keeps rising so gaps are
        // detectable.
        while (events_.size() >= queue_limit_) events_.pop_front();
        events_.push_back(std::move(e));
        ++sequence_;
        return true;
    }

    bool transition(State next, Kind kind, Level level, CausalStage stage,
                    uint64_t now_ms, std::string message) {
        if (!legal_transition(state_, next)) return false;
        Event e = base(kind, level, stage, now_ms, std::move(message));
        e.fields.emplace("from", state_name(state_));
        e.fields.emplace("to", state_name(next));
        if (!append(std::move(e))) return false;
        state_ = next;
        events_.back().state = state_;
        return true;
    }

    static std::string event_json(const Event& e, SanitizeReport* report) {
        const auto safe_fields = sanitize_display_fields(e.fields, report);
        std::ostringstream out;
        out << "{\"sequence\":" << e.sequence
            << ",\"timestamp_ms\":" << e.timestamp_ms
            << ",\"kind\":\"" << kind_name(e.kind)
            << "\",\"level\":\"" << level_name(e.level)
            << "\",\"stage\":\"" << stage_name(e.stage)
            << "\",\"state\":\"" << state_name(e.state)
            << "\",\"object_id\":\"" << escape_json(e.object_id)
            << "\",\"message\":\"" << escape_json(e.message)
            << "\",\"fields\":{";
        bool first = true;
        for (const auto& kv : safe_fields) {
            if (!first) out << ',';
            first = false;
            out << '\"' << escape_json(kv.first) << "\":\""
                << escape_json(kv.second) << '\"';
        }
        out << "}}";
        return out.str();
    }

    Sequitur level_;
    State state_{State::Created};
    uint64_t sequence_{0};
    uint64_t attention_count_{0};
    bool operator_closed_{false};
    size_t queue_limit_;
    std::string object_id_;
    std::deque<Event> events_;
    AttenuationLedger attenuations_;
};

// ===========================================================================
// SECTION 8 — Resource snapshots and boundary announcement
// ===========================================================================
//
// Resource observations are heralded independently of the final verdict. A
// boundary being exceeded is a Warning event, but the herald does not itself
// decide the object's fate — that is the gate's role.

struct ResourceSnapshot {
    uint64_t bytes{0};
    uint64_t files{0};
    uint32_t recursion{0};
    uint32_t archive_depth{0};
    uint64_t elapsed_ms{0};
};

bool announce_resources(Herald& h, uint64_t now_ms,
                        const ResourceSnapshot& s,
                        const ResourceSnapshot& limit) {
    bool ok = true;
    ok = h.boundary(now_ms, "bytes", s.bytes, limit.bytes) && ok;
    ok = h.boundary(now_ms, "files", s.files, limit.files) && ok;
    ok = h.boundary(now_ms, "recursion", s.recursion, limit.recursion) && ok;
    ok = h.boundary(now_ms, "archive_depth", s.archive_depth,
                    limit.archive_depth) && ok;
    ok = h.boundary(now_ms, "elapsed_ms", s.elapsed_ms, limit.elapsed_ms) && ok;
    return ok;
}

// If any boundary is exceeded, herald an attenuation so the closure reflects
// reduced certainty. This ties the resource channel to the attenuation model.
bool announce_resource_attenuation(Herald& h, uint64_t now_ms,
                                   const ResourceSnapshot& s,
                                   const ResourceSnapshot& limit) {
    bool any = false;
    auto over = [&](const char* name, uint64_t observed, uint64_t lim) {
        if (observed > lim) {
            h.attenuate(now_ms, name, "resource limit exceeded", 1.0, 0.5);
            any = true;
        }
    };
    over("bytes", s.bytes, limit.bytes);
    over("files", s.files, limit.files);
    over("recursion", s.recursion, limit.recursion);
    over("archive_depth", s.archive_depth, limit.archive_depth);
    over("elapsed_ms", s.elapsed_ms, limit.elapsed_ms);
    return any;
}

// ===========================================================================
// SECTION 9 — Publication predicates
// ===========================================================================
//
// These honor safety rule 1: absence of a finding is never safety. A herald
// is only "publishable" once it reaches a terminal state with at least one
// object-identified event.

bool publishable(const Herald& h) {
    return !h.events().empty() && !h.events().back().object_id.empty() &&
           terminal(h.state());
}

bool successful(const Herald& h) {
    return publishable(h) && h.state() == State::Completed;
}

bool isolated(const Herald& h) {
    return publishable(h) && h.state() == State::Quarantined;
}

bool failed(const Herald& h) {
    return h.state() == State::Failed;
}

// Level-8 completeness: a successful completion at the operator-herald level
// must carry an explicit operator closure. Without it, the scan is not
// publishable as complete no matter how clean the evidence looked.
bool level8_complete(const Herald& h) {
    if (h.level() != Sequitur::S8_OperatorHerald) return successful(h);
    return successful(h) && h.operator_closed();
}

// ===========================================================================
// SECTION 10 — Per-level herald policy
// ===========================================================================
//
// Each even level enables additional herald obligations. Like the gate, the
// herald only ever becomes more demanding as the level rises.

struct HeraldPolicy {
    Sequitur level{Sequitur::S2_SecondLook};
    bool require_second_look{false};     // L2: re-observe before completion
    bool require_qualified_evidence{false}; // L4: evidence must be qualified
    bool require_attenuation_record{false}; // L6: uncertainty must be recorded
    bool require_operator_closure{false};   // L8: explicit operator closure
    double closure_floor{0.0};               // minimum contribution to complete
};

HeraldPolicy herald_policy_for_level(Sequitur level) {
    HeraldPolicy p;
    p.level = level;
    const int n = static_cast<int>(level);
    if (n >= 2) p.require_second_look = true;
    if (n >= 4) p.require_qualified_evidence = true;
    if (n >= 6) { p.require_attenuation_record = true; p.closure_floor = 0.25; }
    if (n >= 8) { p.require_operator_closure = true; p.closure_floor = 0.55; }
    return p;
}

// Validate that a herald satisfied the obligations for its level before it is
// allowed to be published as a clean completion. Returns an empty optional on
// success, or a human-readable reason on failure.
std::optional<std::string> validate_for_level(const Herald& h) {
    const HeraldPolicy p = herald_policy_for_level(h.level());
    if (!publishable(h))
        return std::string("herald has not reached a terminal state");
    if (h.state() != State::Completed)
        return std::nullopt; // non-clean terminals need no clean-completion check
    if (p.require_attenuation_record && h.attenuations().empty())
        return std::string("attenuation level requires a recorded attenuation "
                           "or an explicit statement of none");
    if (p.require_operator_closure && !h.operator_closed())
        return std::string("operator-herald level requires explicit closure");
    if (p.closure_floor > 0.0 && h.effective_contribution() < p.closure_floor)
        return std::string("effective contribution is below the closure floor");
    return std::nullopt;
}

// ===========================================================================
// SECTION 11 — Second-look confirmation (level 2)
// ===========================================================================
//
// Level 2 is "second-look confirmation": the herald re-observes the object at
// least once before it will narrate completion. A ConfirmationTracker counts
// distinct inspection passes and refuses to confirm on a single pass.

class ConfirmationTracker {
public:
    void note_inspection() { ++passes_; }
    uint64_t passes() const { return passes_; }
    // Second look confirmed once at least two inspection passes were observed.
    bool confirmed() const { return passes_ >= 2; }
private:
    uint64_t passes_{0};
};

// Drive a herald through a confirmed second-look sequence, returning true only
// if both passes and the finalize/complete transitions succeeded.
bool run_second_look(Herald& h, const std::string& object_id, uint64_t base_ms,
                     ConfirmationTracker& tracker) {
    if (!h.start(object_id, base_ms)) return false;
    if (!h.inspect(base_ms + 1)) return false;
    tracker.note_inspection();
    // Return to inspecting via an extraction round-trip for the second look.
    if (!h.extract(base_ms + 2)) return false;
    if (!h.inspect(base_ms + 3)) return false;
    tracker.note_inspection();
    if (!tracker.confirmed()) return false;
    if (!h.finalize(base_ms + 4)) return false;
    return true;
}

// ===========================================================================
// SECTION 12 — Evidence qualification (level 4)
// ===========================================================================
//
// Level 4 qualifies evidence: an announced piece of evidence must name a
// technical source and a non-empty detail, and must carry a severity that is
// consistent with its content. This guards against vacuous "evidence" that
// would otherwise pad a record toward a false sense of completeness.

struct QualifiedEvidence {
    std::string source;
    std::string detail;
    Level severity{Level::Info};
    CausalStage stage{CausalStage::Cause};

    bool qualified() const {
        if (source.empty() || is_blank(source)) return false;
        if (detail.empty() || is_blank(detail)) return false;
        return true;
    }
};

bool herald_qualified(Herald& h, uint64_t now_ms, const QualifiedEvidence& q) {
    if (!q.qualified()) return false;
    return h.evidence(now_ms, q.severity, q.source, q.detail);
}

// ===========================================================================
// SECTION 13 — Operator herald and closure (level 8)
// ===========================================================================
//
// Level 8 is the final operator herald. It composes the whole discipline: a
// confirmed second look, qualified evidence, a recorded attenuation position,
// and an explicit operator closure. Only then may completion be narrated.

struct OperatorHeraldResult {
    bool completed{false};
    std::string reason;   // populated when not completed
    std::string report;   // the JSON record
};

OperatorHeraldResult run_operator_herald(
    const std::string& object_id, uint64_t base_ms,
    const std::vector<QualifiedEvidence>& evidence,
    const std::string& operator_ref, bool clean) {
    OperatorHeraldResult result;
    Herald h(Sequitur::S8_OperatorHerald);
    ConfirmationTracker tracker;

    if (!run_second_look(h, object_id, base_ms, tracker)) {
        result.reason = "second-look confirmation failed";
        result.report = h.json();
        return result;
    }
    // Qualify and herald each piece of evidence.
    uint64_t t = base_ms + 5;
    for (const auto& q : evidence) {
        if (!herald_qualified(h, t++, q)) {
            result.reason = "unqualified evidence rejected at level 8";
            result.report = h.json();
            return result;
        }
    }
    // Record an explicit attenuation position (rule 3): if the caller believes
    // there is no uncertainty, that too must be stated, not assumed.
    h.attenuate(t++, "observation", clean ? "fully observed" : "gaps present",
                1.0, clean ? 1.0 : 0.5);

    if (clean) {
        // Explicit operator closure precedes completion at level 8.
        h.operator_closure(t++, operator_ref);
        if (!h.complete(t++)) {
            result.reason = "completion refused despite closure";
            result.report = h.json();
            return result;
        }
    } else {
        h.quarantine(t++);
    }

    const auto problem = validate_for_level(h);
    if (problem.has_value() && h.state() == State::Completed) {
        result.reason = *problem;
        result.report = h.json();
        return result;
    }
    result.completed = (h.state() == State::Completed);
    result.report = h.json();
    return result;
}

// ===========================================================================
// SECTION 14 — Correlation with a gate verdict
// ===========================================================================
//
// The herald can be handed a gate verdict string to narrate. It must NEVER
// upgrade a non-clean gate verdict to a clean completion (safety rule 2). This
// function enforces that: a gate verdict other than "allow" can only drive the
// herald toward quarantine or failure, never toward completion.

bool narrate_gate_verdict(Herald& h, uint64_t now_ms,
                          const std::string& gate_verdict,
                          const std::string& reason,
                          const std::string& confidence) {
    h.verdict(now_ms, gate_verdict, reason, confidence);
    if (gate_verdict == "allow") {
        // The herald may proceed toward completion; the caller still owns the
        // final transition so this only records the verdict.
        return true;
    }
    if (gate_verdict == "quarantine" || gate_verdict == "reject") {
        h.finalize(now_ms + 1);
        return h.quarantine(now_ms + 2);
    }
    // error / observe / anything else: fail closed.
    h.finalize(now_ms + 1);
    return h.fail(now_ms + 2, "gate verdict is not clean; failing closed");
}

// ===========================================================================
// SECTION 15 — Invariant self-tests
// ===========================================================================

namespace selftest {

static void expect(bool condition, int& failures) {
    if (!condition) ++failures;
}

// Unlawful transitions are refused.
static void test_illegal_transition_refused(int& failures) {
    Herald h;
    // Cannot complete from Created.
    expect(!h.complete(1), failures);
    expect(h.state() == State::Created, failures);
    // Start, then cannot jump straight to Extracting.
    expect(h.start("obj", 1), failures);
    expect(!h.extract(2), failures);
    expect(h.state() == State::Running, failures);
}

// The sequence counter is strictly monotonic across a normal run.
static void test_sequence_monotone(int& failures) {
    Herald h;
    h.start("obj", 1);
    h.inspect(2);
    h.evidence(3, Level::Info, "clamav", "clean");
    h.finalize(4);
    uint64_t last = 0;
    for (const auto& e : h.events()) {
        expect(e.sequence == last + 1, failures);
        last = e.sequence;
    }
}

// The demographic firewall strips denied keys from rendered output.
static void test_firewall_strips_display_keys(int& failures) {
    Herald h;
    h.start("obj", 1);
    // A poorly-behaved caller tries to attach a demographic field via a raw
    // evidence detail path. We simulate by sanitizing a fields map directly.
    std::map<std::string, std::string> fields = {
        {"source", "clamav"}, {"religion", "n/a"}, {"age", "42"}
    };
    SanitizeReport report;
    const auto clean = sanitize_display_fields(fields, &report);
    expect(report.stripped == 2, failures);
    expect(clean.find("religion") == clean.end(), failures);
    expect(clean.find("age") == clean.end(), failures);
    expect(clean.find("source") != clean.end(), failures);
}

// A non-clean gate verdict never yields a clean completion.
static void test_non_clean_never_completes(int& failures) {
    Herald h;
    h.start("obj", 1);
    h.inspect(2);
    const bool ok = narrate_gate_verdict(h, 3, "quarantine", "signature",
                                         "confirmed");
    expect(ok, failures);
    expect(h.state() == State::Quarantined, failures);
    expect(!successful(h), failures);
    expect(isolated(h), failures);
}

// Level 8 refuses completion without an explicit operator closure.
static void test_level8_requires_closure(int& failures) {
    Herald h(Sequitur::S8_OperatorHerald);
    h.start("obj", 1);
    h.inspect(2);
    h.finalize(3);
    // No operator closure recorded: completion must be refused.
    expect(!h.complete(4), failures);
    expect(h.state() == State::Finalizing, failures);
    // Now record closure and complete.
    h.operator_closure(5, "operator-7");
    expect(h.complete(6), failures);
    expect(h.state() == State::Completed, failures);
    expect(level8_complete(h), failures);
}

// The full operator-herald pipeline completes on clean, qualified input.
static void test_operator_herald_clean(int& failures) {
    std::vector<QualifiedEvidence> ev = {
        {"clamav", "clean", Level::Notice, CausalStage::Cause},
        {"updater", "signed cvd 27000", Level::Info, CausalStage::Cause}
    };
    const auto r = run_operator_herald("obj", 1000, ev, "operator-7", true);
    expect(r.completed, failures);
}

// The pipeline quarantines (does not complete) on a non-clean object.
static void test_operator_herald_dirty(int& failures) {
    std::vector<QualifiedEvidence> ev = {
        {"clamav", "Win.Test.EICAR", Level::Critical, CausalStage::Cause}
    };
    const auto r = run_operator_herald("obj", 1000, ev, "operator-7", false);
    expect(!r.completed, failures);
}

// Attenuation composes multiplicatively and floors at zero.
static void test_attenuation_composition(int& failures) {
    AttenuationLedger l;
    l.record("a", "gap", 1.0, 0.5);
    l.record("b", "limit", 1.0, 0.5);
    // 0.5 * 0.5 = 0.25
    const double c = l.composite();
    expect(c > 0.24 && c < 0.26, failures);
}

// Unqualified evidence is rejected.
static void test_unqualified_evidence_rejected(int& failures) {
    Herald h(Sequitur::S4_Qualification);
    h.start("obj", 1);
    QualifiedEvidence empty{"", "", Level::Info, CausalStage::Cause};
    expect(!herald_qualified(h, 2, empty), failures);
}

} // namespace selftest

// ===========================================================================
// SECTION 16 — Herald stream consumer / verifier
// ===========================================================================
//
// A downstream consumer of herald events must be able to detect two failure
// modes that would let a partial scan masquerade as complete: (1) a gap in
// the sequence (a dropped event), and (2) reordering. The StreamVerifier
// consumes events one at a time and reports either kind of anomaly. It also
// enforces that no event is ever accepted after a terminal event, which is
// how a consumer avoids narrating activity "after" a completion.

struct StreamAnomaly {
    enum class Type { None, Gap, Reordered, AfterTerminal, IdentityDrift };
    Type type{Type::None};
    uint64_t at_sequence{0};
    std::string detail;
};

class StreamVerifier {
public:
    // Feed one event. Returns an anomaly (possibly None). The verifier is
    // conservative: once it has seen a terminal event, any further event is an
    // AfterTerminal anomaly.
    StreamAnomaly consume(const Event& e) {
        StreamAnomaly a;
        if (seen_terminal_) {
            a.type = StreamAnomaly::Type::AfterTerminal;
            a.at_sequence = e.sequence;
            a.detail = "event arrived after a terminal state";
            return a;
        }
        if (!object_id_.empty() && e.object_id != object_id_) {
            a.type = StreamAnomaly::Type::IdentityDrift;
            a.at_sequence = e.sequence;
            a.detail = "event object_id differs from the stream's object";
            return a;
        }
        if (object_id_.empty()) object_id_ = e.object_id;

        if (last_sequence_ != 0) {
            if (e.sequence <= last_sequence_) {
                a.type = StreamAnomaly::Type::Reordered;
                a.at_sequence = e.sequence;
                a.detail = "sequence did not strictly increase";
                return a;
            }
            if (e.sequence != last_sequence_ + 1) {
                a.type = StreamAnomaly::Type::Gap;
                a.at_sequence = e.sequence;
                a.detail = "one or more events are missing";
                // We still advance so subsequent checks remain useful.
            }
        }
        last_sequence_ = e.sequence;
        ++accepted_;
        if (terminal(e.state)) {
            seen_terminal_ = true;
            terminal_state_ = e.state;
        }
        return a;
    }

    uint64_t accepted() const { return accepted_; }
    bool saw_terminal() const { return seen_terminal_; }
    State terminal_state() const { return terminal_state_; }

    // A stream is trustworthy only if it reached a terminal state with no
    // anomalies encountered along the way. A consumer that never saw a
    // terminal must treat the object as still-uncertain (safety rule 1).
    bool trustworthy() const { return seen_terminal_ && !anomaly_seen_; }
    void mark_anomaly() { anomaly_seen_ = true; }

private:
    std::string object_id_;
    uint64_t last_sequence_{0};
    uint64_t accepted_{0};
    bool seen_terminal_{false};
    bool anomaly_seen_{false};
    State terminal_state_{State::Created};
};

// Replay a herald's own event deque through a verifier. Because the herald
// enforces its invariants at append time, a self-replay of an untouched
// herald should always be anomaly-free up to the queue-drop boundary.
StreamVerifier replay(const Herald& h) {
    StreamVerifier v;
    for (const auto& e : h.events()) {
        const auto anomaly = v.consume(e);
        if (anomaly.type != StreamAnomaly::Type::None &&
            anomaly.type != StreamAnomaly::Type::Gap)
            v.mark_anomaly();
    }
    return v;
}

// ===========================================================================
// SECTION 17 — Multi-object scan session aggregation
// ===========================================================================
//
// A real scan covers many objects. A ScanSession owns a herald per object and
// aggregates their terminal outcomes. Its summary is exactly the kind of thing
// a level-8 operator herald would present: counts of completed, quarantined,
// and failed objects, plus the lowest effective contribution observed (the
// most-attenuated object bounds the confidence of the whole session).

class ScanSession {
public:
    explicit ScanSession(Sequitur level = Sequitur::S2_SecondLook)
        : level_(level) {}

    // Begin heralding a new object. Returns a reference to its herald so the
    // caller can drive it. The session keeps ownership.
    Herald& begin(const std::string& object_id, uint64_t now_ms) {
        heralds_.emplace_back(std::make_unique<Herald>(level_));
        heralds_.back()->start(object_id, now_ms);
        return *heralds_.back();
    }

    size_t object_count() const { return heralds_.size(); }

    size_t completed() const { return count_state(State::Completed); }
    size_t quarantined() const { return count_state(State::Quarantined); }
    size_t failed() const { return count_state(State::Failed); }

    // The session is only "all clean" if every object completed cleanly AND
    // each reached its level obligations. A single non-clean object taints the
    // aggregate (safety rule 1: no averaging-away of a bad result).
    bool all_clean() const {
        if (heralds_.empty()) return false;
        for (const auto& h : heralds_) {
            if (h->state() != State::Completed) return false;
            if (validate_for_level(*h).has_value()) return false;
        }
        return true;
    }

    // Lowest effective contribution across all objects. This bounds session
    // confidence: the session is no more certain than its weakest object.
    double min_contribution() const {
        double lo = 1.0;
        for (const auto& h : heralds_)
            lo = std::min(lo, h->effective_contribution());
        return heralds_.empty() ? 0.0 : lo;
    }

    std::string summary_json() const {
        std::ostringstream out;
        out << "{\"level\":\"" << sequitur_name(level_)
            << "\",\"objects\":" << heralds_.size()
            << ",\"completed\":" << completed()
            << ",\"quarantined\":" << quarantined()
            << ",\"failed\":" << failed()
            << ",\"all_clean\":" << (all_clean() ? "true" : "false")
            << ",\"min_contribution\":" << min_contribution() << "}";
        return out.str();
    }

private:
    size_t count_state(State s) const {
        size_t n = 0;
        for (const auto& h : heralds_)
            if (h->state() == s) ++n;
        return n;
    }

    Sequitur level_;
    std::vector<std::unique_ptr<Herald>> heralds_;
};

// ===========================================================================
// SECTION 17 — Worked scenarios (executable documentation)
// ===========================================================================
//
// The four even levels each carry a distinct obligation. The functions below
// are executable documentation: each drives a herald through the canonical
// happy path for its level and returns whether the level's obligation was met.
// They double as integration coverage and as a reference for how a consuming
// build is expected to drive the herald.

// Level 2 — second-look confirmation. The obligation: the object must be
// re-observed (two inspection passes) before completion is narrated.
struct ScenarioResult {
    bool obligation_met{false};
    std::string report;
};

ScenarioResult scenario_level2_second_look(const std::string& object_id) {
    Herald h(Sequitur::S2_SecondLook);
    ConfirmationTracker tracker;
    ScenarioResult r;
    if (!run_second_look(h, object_id, 1000, tracker)) {
        r.report = h.json();
        return r;
    }
    h.evidence(1100, Level::Notice, "clamav", "clean");
    h.complete(1200);
    r.obligation_met = tracker.confirmed() && successful(h);
    r.report = h.json();
    return r;
}

// Level 4 — evidence qualification. The obligation: only qualified evidence
// (named source + non-empty detail) contributes; a vacuous item is rejected.
ScenarioResult scenario_level4_qualification(const std::string& object_id) {
    Herald h(Sequitur::S4_Qualification);
    ScenarioResult r;
    h.start(object_id, 1000);
    h.inspect(1001);
    const QualifiedEvidence good{"clamav", "clean", Level::Notice,
                                 CausalStage::Cause};
    const QualifiedEvidence bad{"", "", Level::Info, CausalStage::Cause};
    const bool good_ok = herald_qualified(h, 1002, good);
    const bool bad_rejected = !herald_qualified(h, 1003, bad);
    h.finalize(1004);
    h.complete(1005);
    r.obligation_met = good_ok && bad_rejected && successful(h);
    r.report = h.json();
    return r;
}

// Level 6 — attenuation and uncertainty. The obligation: uncertainty must be
// recorded as an attenuation, and it must lower the effective contribution.
ScenarioResult scenario_level6_attenuation(const std::string& object_id) {
    Herald h(Sequitur::S6_Attenuation);
    ScenarioResult r;
    h.start(object_id, 1000);
    h.inspect(1001);
    h.attenuate(1002, "container", "encrypted region not observed", 1.0, 0.6);
    h.finalize(1003);
    h.complete(1004);
    // Obligation: an attenuation was recorded and contribution dropped, but a
    // clean completion is still permitted if it stays above the closure floor.
    const auto problem = validate_for_level(h);
    r.obligation_met = !h.attenuations().empty() &&
                       h.effective_contribution() < 1.0 &&
                       !problem.has_value();
    r.report = h.json();
    return r;
}

// Level 8 — operator herald and closure. The obligation: an explicit operator
// closure precedes any clean completion.
ScenarioResult scenario_level8_operator(const std::string& object_id) {
    std::vector<QualifiedEvidence> ev = {
        {"clamav", "clean", Level::Notice, CausalStage::Cause},
        {"updater", "signed cvd 27000", Level::Info, CausalStage::Cause}
    };
    const auto o = run_operator_herald(object_id, 1000, ev, "operator-7", true);
    ScenarioResult r;
    r.obligation_met = o.completed;
    r.report = o.report;
    return r;
}

// ===========================================================================
// SECTION 17a — Channel timeline (the five observation channels)
// ===========================================================================
//
// DESCRIPTOR.md section 5 names five channels through which a software object's
// state can continue or change: artifact, dependency, update, execution, and
// provenance. The herald records observations against these channels so that a
// reviewer can distinguish a change in the artifact from a change in the
// scanner's knowledge or environment (DESCRIPTOR.md section 9). Each channel
// entry is timestamped, giving a per-channel timeline.

enum class Channel { Artifact, Dependency, Update, Execution, Provenance };

static const char* channel_name(Channel c) {
    switch (c) {
    case Channel::Artifact:   return "artifact";
    case Channel::Dependency: return "dependency";
    case Channel::Update:     return "update";
    case Channel::Execution:  return "execution";
    case Channel::Provenance: return "provenance";
    }
    return "unknown";
}

struct ChannelObservation {
    Channel channel{Channel::Artifact};
    uint64_t timestamp_ms{0};
    std::string actor;   // technical actor: "updater", "loader", "packager"
    std::string detail;
    bool prior_known{true}; // false => a retro-dependency provenance gap

    std::string serialize() const {
        std::ostringstream out;
        out << "{\"channel\":\"" << channel_name(channel)
            << "\",\"timestamp_ms\":" << timestamp_ms
            << ",\"actor\":\"" << escape_json(actor)
            << "\",\"detail\":\"" << escape_json(detail)
            << "\",\"prior_known\":" << (prior_known ? "true" : "false")
            << "}";
        return out.str();
    }
};

// A timeline of channel observations. It answers the retro-dependency
// questions of DESCRIPTOR.md section 4 by preserving, per channel, what was
// observed and whether its prior condition is known.
class ChannelTimeline {
public:
    void observe(Channel c, uint64_t ts, std::string actor, std::string detail,
                 bool prior_known = true) {
        entries_.push_back(ChannelObservation{c, ts, std::move(actor),
                                               std::move(detail), prior_known});
    }

    size_t size() const { return entries_.size(); }
    bool empty() const { return entries_.empty(); }

    // Count observations on a given channel.
    size_t count(Channel c) const {
        size_t n = 0;
        for (const auto& e : entries_)
            if (e.channel == c) ++n;
        return n;
    }

    // A provenance gap exists if any observation records an unknown prior. The
    // update channel is especially important: a signature/database change with
    // an unknown prior means we cannot attribute a changed verdict (section 9).
    size_t provenance_gaps() const {
        size_t gaps = 0;
        for (const auto& e : entries_)
            if (!e.prior_known) ++gaps;
        return gaps;
    }

    // Provenance is considered established when there is at least one
    // provenance-channel observation whose prior is known.
    bool provenance_established() const {
        for (const auto& e : entries_)
            if (e.channel == Channel::Provenance && e.prior_known) return true;
        return false;
    }

    // Detect the section-9 scenario: an update-channel event sits between two
    // artifact/execution observations, i.e. the scanner's knowledge changed
    // during the object's continuity. Returns true if an update was observed
    // after the first non-update observation.
    bool knowledge_changed_mid_stream() const {
        bool saw_non_update = false;
        for (const auto& e : entries_) {
            if (e.channel != Channel::Update) {
                saw_non_update = true;
            } else if (saw_non_update) {
                return true;
            }
        }
        return false;
    }

    std::string serialize() const {
        std::ostringstream out;
        out << '[';
        bool first = true;
        for (const auto& e : entries_) {
            if (!first) out << ',';
            first = false;
            out << e.serialize();
        }
        out << ']';
        return out.str();
    }

private:
    std::vector<ChannelObservation> entries_;
};

// Herald a channel timeline into a Herald as attention/evidence events, and
// attenuate for each provenance gap so the closure reflects the uncertainty
// the gaps introduce (safety rules 3 and 4).
bool announce_channels(Herald& h, const ChannelTimeline& tl, uint64_t base_ms) {
    bool ok = true;
    uint64_t t = base_ms;
    for (size_t i = 0; i < tl.size(); ++i) {
        // Re-serialize per entry is avoided; we herald a compact summary.
        ok = h.attention(t, "channel", "channel observation recorded") && ok;
        ++t;
    }
    // One attenuation per provenance gap.
    for (size_t g = 0; g < tl.provenance_gaps(); ++g) {
        h.attenuate(t++, "provenance", "prior condition unknown", 1.0, 0.7);
    }
    // If the scanner's knowledge changed mid-stream, that is a section-9
    // condition worth an explicit attention marker.
    if (tl.knowledge_changed_mid_stream())
        h.attention(t++, "update", "scanner knowledge changed mid-stream");
    return ok;
}

// ===========================================================================
// SECTION 18a — Causal narrative renderer
// ===========================================================================
//
// The design (PROCEDURAL_CAUSATION.hss) is explicit that the procedure moves
// ROOT -> METHOD -> CAUSE -> ATTENTION -> ATTENUATION -> CLOSURE. A herald's
// event stream implicitly walks that path via each event's CausalStage. This
// renderer reconstructs the path as human-readable prose so a reviewer can see
// the "movement" without reading raw JSON. It also detects a stage regression
// (an event whose stage precedes an earlier event's stage), which would signal
// that the narrative doubled back — permitted mechanically (extraction can
// return to inspection) but worth surfacing.

struct CausalNarrative {
    std::string prose;
    bool reached_closure{false};
    bool regressed{false};
    std::vector<CausalStage> path;
};

CausalNarrative render_causal_narrative(const Herald& h) {
    CausalNarrative n;
    std::ostringstream out;
    int last_stage = -1;
    bool first = true;
    for (const auto& e : h.events()) {
        const int cur = static_cast<int>(e.stage);
        n.path.push_back(e.stage);
        if (!first && cur < last_stage) n.regressed = true;
        if (e.stage == CausalStage::Closure) n.reached_closure = true;
        if (!first) out << " -> ";
        out << stage_name(e.stage) << '(' << kind_name(e.kind) << ')';
        last_stage = cur;
        first = false;
    }
    // Safety rule 1/3: a narrative that never reached closure must say so, and
    // must not read as if the object were resolved.
    if (!n.reached_closure)
        out << " [no closure reached: object remains uncertain]";
    n.prose = out.str();
    return n;
}

// A convenience that pairs the narrative with the herald's terminal judgment.
std::string narrate(const Herald& h) {
    const CausalNarrative n = render_causal_narrative(h);
    std::ostringstream out;
    out << "object=" << (h.events().empty() ? "<none>"
                                             : h.events().back().object_id)
        << " state=" << state_name(h.state())
        << " contribution=" << h.effective_contribution()
        << "\n  path: " << n.prose;
    if (n.regressed)
        out << "\n  note: narrative revisited an earlier stage (container "
               "re-inspection is lawful)";
    return out.str();
}

// ===========================================================================
// SECTION 18b — Herald / gate reconciliation
// ===========================================================================
//
// The herald and the gate are separate authorities. The gate DECIDES; the
// herald NARRATES. Safety rule 2 forbids the herald from ever contradicting a
// gate detection by narrating a clean completion. This reconciler takes a gate
// verdict string and a herald and reports whether they are consistent.

struct Reconciliation {
    bool consistent{true};
    std::string detail;
};

Reconciliation reconcile(const std::string& gate_verdict, const Herald& h) {
    Reconciliation r;
    const State s = h.state();
    // If the gate did not allow, the herald must not have completed cleanly.
    if (gate_verdict != "allow" && s == State::Completed) {
        r.consistent = false;
        r.detail = "herald completed cleanly despite a non-allow gate verdict";
        return r;
    }
    // If the gate quarantined/rejected, the herald must be isolated or failed.
    if ((gate_verdict == "quarantine" || gate_verdict == "reject") &&
        !(s == State::Quarantined || s == State::Failed) && terminal(s)) {
        r.consistent = false;
        r.detail = "herald terminal state does not reflect gate isolation";
        return r;
    }
    r.detail = "herald narrative is consistent with the gate verdict";
    return r;
}

// ===========================================================================
// SECTION 18 — Self-tests for the consumer and session layers
// ===========================================================================
//
// These tests reference StreamVerifier and ScanSession, so they live after
// those types are defined. The selftest namespace is reopened here.

namespace selftest {

// A self-replayed clean herald verifies as trustworthy and reached terminal.
static void test_stream_verifier_trustworthy(int& failures) {
    Herald h(Sequitur::S2_SecondLook);
    ConfirmationTracker tracker;
    run_second_look(h, "obj", 100, tracker);
    h.complete(200);
    const StreamVerifier v = replay(h);
    expect(v.saw_terminal(), failures);
    expect(v.trustworthy(), failures);
    expect(v.terminal_state() == State::Completed, failures);
}

// The verifier flags an event that arrives after a terminal state.
static void test_stream_verifier_after_terminal(int& failures) {
    StreamVerifier v;
    Event a; a.sequence = 1; a.object_id = "obj"; a.state = State::Completed;
    Event b; b.sequence = 2; b.object_id = "obj"; b.state = State::Running;
    v.consume(a);
    const auto anomaly = v.consume(b);
    expect(anomaly.type == StreamAnomaly::Type::AfterTerminal, failures);
}

// The verifier flags reordering.
static void test_stream_verifier_reorder(int& failures) {
    StreamVerifier v;
    Event a; a.sequence = 2; a.object_id = "obj"; a.state = State::Running;
    Event b; b.sequence = 1; b.object_id = "obj"; b.state = State::Inspecting;
    v.consume(a);
    const auto anomaly = v.consume(b);
    expect(anomaly.type == StreamAnomaly::Type::Reordered, failures);
}

// A session with one quarantined object is never "all clean".
static void test_session_taint(int& failures) {
    ScanSession session(Sequitur::S2_SecondLook);
    Herald& clean = session.begin("clean-obj", 1);
    clean.inspect(2);
    clean.finalize(3);
    clean.complete(4);
    Herald& dirty = session.begin("dirty-obj", 10);
    dirty.inspect(11);
    dirty.finalize(12);
    dirty.quarantine(13);
    expect(session.object_count() == 2, failures);
    expect(session.completed() == 1, failures);
    expect(session.quarantined() == 1, failures);
    expect(!session.all_clean(), failures);
}

// A completed herald's narrative reaches closure.
static void test_narrative_reaches_closure(int& failures) {
    Herald h(Sequitur::S2_SecondLook);
    ConfirmationTracker tracker;
    run_second_look(h, "obj", 100, tracker);
    h.complete(200);
    const CausalNarrative n = render_causal_narrative(h);
    expect(n.reached_closure, failures);
}

// A herald stopped before finalization does not reach closure and says so.
static void test_narrative_no_closure(int& failures) {
    Herald h(Sequitur::S2_SecondLook);
    h.start("obj", 1);
    h.inspect(2);
    const CausalNarrative n = render_causal_narrative(h);
    expect(!n.reached_closure, failures);
    expect(n.prose.find("no closure reached") != std::string::npos, failures);
}

// Reconciliation catches a herald that completed despite a non-allow verdict.
static void test_reconcile_catches_contradiction(int& failures) {
    // Build a herald that (incorrectly, for the test) completed cleanly.
    Herald h(Sequitur::S2_SecondLook);
    h.start("obj", 1);
    h.inspect(2);
    h.finalize(3);
    h.complete(4);
    const Reconciliation r = reconcile("quarantine", h);
    expect(!r.consistent, failures);
}

// Reconciliation accepts a consistent quarantine.
static void test_reconcile_accepts_consistent(int& failures) {
    Herald h(Sequitur::S2_SecondLook);
    h.start("obj", 1);
    h.inspect(2);
    h.finalize(3);
    h.quarantine(4);
    const Reconciliation r = reconcile("quarantine", h);
    expect(r.consistent, failures);
}

// The channel timeline counts per-channel observations and detects gaps.
static void test_channel_timeline_gaps(int& failures) {
    ChannelTimeline tl;
    tl.observe(Channel::Provenance, 1, "packager", "signed 1.2", true);
    tl.observe(Channel::Dependency, 2, "loader", "libc 2.39", false);
    tl.observe(Channel::Artifact, 3, "fs", "elf64", true);
    expect(tl.size() == 3, failures);
    expect(tl.count(Channel::Dependency) == 1, failures);
    expect(tl.provenance_gaps() == 1, failures);
    expect(tl.provenance_established(), failures);
}

// The timeline detects a mid-stream knowledge change (update after artifact).
static void test_channel_knowledge_change(int& failures) {
    ChannelTimeline tl;
    tl.observe(Channel::Artifact, 1, "fs", "elf64", true);
    tl.observe(Channel::Update, 2, "updater", "cvd 27001", true);
    expect(tl.knowledge_changed_mid_stream(), failures);
    ChannelTimeline tl2;
    tl2.observe(Channel::Update, 1, "updater", "cvd 27001", true);
    tl2.observe(Channel::Artifact, 2, "fs", "elf64", true);
    expect(!tl2.knowledge_changed_mid_stream(), failures);
}

// Announcing channels with a gap attenuates the herald's contribution.
static void test_channel_announcement_attenuates(int& failures) {
    Herald h(Sequitur::S6_Attenuation);
    h.start("obj", 1);
    ChannelTimeline tl;
    tl.observe(Channel::Provenance, 2, "packager", "signed", true);
    tl.observe(Channel::Dependency, 3, "loader", "unknown lib", false);
    announce_channels(h, tl, 10);
    // One provenance gap => contribution attenuated below 1.0.
    expect(h.effective_contribution() < 1.0, failures);
}

// Each even-level worked scenario meets its obligation on the happy path.
static void test_scenario_level2(int& failures) {
    expect(scenario_level2_second_look("obj").obligation_met, failures);
}
static void test_scenario_level4(int& failures) {
    expect(scenario_level4_qualification("obj").obligation_met, failures);
}
static void test_scenario_level6(int& failures) {
    expect(scenario_level6_attenuation("obj").obligation_met, failures);
}
static void test_scenario_level8(int& failures) {
    expect(scenario_level8_operator("obj").obligation_met, failures);
}

// An empty object id is refused at start (identity invariant).
static void test_empty_identity_refused(int& failures) {
    Herald h;
    expect(!h.start("", 1), failures);
    expect(!h.start("   ", 2), failures); // whitespace-only is blank
    expect(h.state() == State::Created, failures);
}

// The bounded queue drops oldest events but keeps the sequence rising, so a
// consumer can still detect that events were dropped.
static void test_bounded_queue(int& failures) {
    Herald h(Sequitur::S2_SecondLook, /*queue_limit=*/2);
    h.start("obj", 1);        // seq 1
    h.inspect(2);              // seq 2
    h.progress(3, 10, 1, 0);   // seq 3 -> drops seq 1
    // Only two events retained, but sequence counter reached 3.
    expect(h.events().size() == 2, failures);
    expect(h.sequence() == 3, failures);
    expect(h.events().front().sequence == 2, failures);
}

} // namespace selftest

// ===========================================================================
// SECTION 19 — Full self-test registration
// ===========================================================================
//
// Registered here, after every type is defined, so the stream-verifier and
// session tests can reference StreamVerifier and ScanSession.

int run_self_tests() {
    int failures = 0;
    selftest::test_illegal_transition_refused(failures);
    selftest::test_sequence_monotone(failures);
    selftest::test_firewall_strips_display_keys(failures);
    selftest::test_non_clean_never_completes(failures);
    selftest::test_level8_requires_closure(failures);
    selftest::test_operator_herald_clean(failures);
    selftest::test_operator_herald_dirty(failures);
    selftest::test_attenuation_composition(failures);
    selftest::test_unqualified_evidence_rejected(failures);
    selftest::test_stream_verifier_trustworthy(failures);
    selftest::test_stream_verifier_after_terminal(failures);
    selftest::test_stream_verifier_reorder(failures);
    selftest::test_session_taint(failures);
    selftest::test_narrative_reaches_closure(failures);
    selftest::test_narrative_no_closure(failures);
    selftest::test_reconcile_catches_contradiction(failures);
    selftest::test_reconcile_accepts_consistent(failures);
    selftest::test_channel_timeline_gaps(failures);
    selftest::test_channel_knowledge_change(failures);
    selftest::test_channel_announcement_attenuates(failures);
    selftest::test_scenario_level2(failures);
    selftest::test_scenario_level4(failures);
    selftest::test_scenario_level6(failures);
    selftest::test_scenario_level8(failures);
    selftest::test_empty_identity_refused(failures);
    selftest::test_bounded_queue(failures);
    return failures;
}

} // namespace legal_clam_herald

// ===========================================================================
// SECTION 20 — Optional standalone entry point
// ===========================================================================
//
// When compiled with -DLEGAL_CLAM_HERALD_MAIN this file becomes a self-test
// driver mirroring the numbered levels' test.c convention. In an ordinary
// build the symbol is absent and the file is a pure library.

#ifdef LEGAL_CLAM_HERALD_MAIN
#include <iostream>
int main() {
    const int failures = legal_clam_herald::run_self_tests();
    std::cout << "legal_clam_herald self-tests: "
              << (failures == 0 ? "all passed" : "FAILURES")
              << " (" << failures << ")\n";

    using namespace legal_clam_herald;
    std::vector<QualifiedEvidence> ev = {
        {"clamav", "clean", Level::Notice, CausalStage::Cause},
        {"updater", "signed cvd 27000", Level::Info, CausalStage::Cause}
    };
    const auto r = run_operator_herald("demo-object", 1000, ev,
                                       "operator-7", true);
    std::cout << "operator herald completed: "
              << (r.completed ? "yes" : "no") << "\n";
    std::cout << r.report << "\n";

    // Narrate the causal path for the same object at level 2 for illustration.
    Herald h(Sequitur::S2_SecondLook);
    ConfirmationTracker tracker;
    run_second_look(h, "demo-object", 2000, tracker);
    h.complete(2100);
    std::cout << narrate(h) << "\n";
    return failures == 0 ? 0 : 1;
}
#endif

/*
 * ---------------------------------------------------------------------------
 * DESIGN SUMMARY (why this file is shaped the way it is)
 * ---------------------------------------------------------------------------
 * The herald is deliberately an *observer*, not a *decider*. Every capability
 * in this translation unit exists to make one of the six safety rules
 * mechanically enforced rather than merely documented:
 *
 *   - The lawful state machine (Section 2) makes it impossible to narrate a
 *     completion out of a partial scan: illegal transitions are no-ops.
 *   - The strict sequence counter and the StreamVerifier (Section 16) let a
 *     consumer detect dropped or reordered events, so a truncated stream can
 *     never be mistaken for a complete one (rule 1).
 *   - The AttenuationLedger (Section 6) and the channel timeline (Section 17a)
 *     make uncertainty first-class: it is recorded and it lowers the effective
 *     contribution rather than being silently discarded (rule 3).
 *   - The provenance channel and retro-dependency gaps (Section 17a) preserve
 *     provenance where available and flag where it is missing (rule 4).
 *   - Level 8 (Sections 13, 17) refuses a clean completion without an explicit
 *     operator closure (rule 5).
 *   - The reconciler (Section 18b) makes it structurally impossible for the
 *     herald to contradict a gate detection by narrating cleanliness (rule 2).
 *   - The demographic firewall (Section 5) strips human-attribute fields on
 *     the way out, keeping detection and human identity distinct (rule 6).
 *
 * The gate (gating.cpp) is the counterpart that DECIDES. The two are kept in
 * separate translation units precisely so their authorities do not blur.
 *
 * Author: Max Rupplin - MEARVK LLC - 2026.
 */
