/*
 * System-centric evidence herald for ClamAV.US.Legal.Edition.
 *
 * The herald announces observations; it does not manufacture the malware
 * verdict. It preserves scan state, technical evidence, resource boundaries,
 * uncertainty, and terminal outcomes so downstream consumers cannot mistake a
 * partial scan for a clean scan. Human identity and reputation are deliberately
 * excluded from the security evidence channel.
 */
#include <cstdint>
#include <deque>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>

namespace legal_clam_herald {

enum class State { Created, Running, Inspecting, Extracting, Finalizing,
                   Completed, Quarantined, Failed };
enum class Kind { Started, Progress, Evidence, Boundary, Verdict, Failure,
                  Completed };

enum class Level { Trace, Info, Notice, Warning, Critical };

struct Event {
    uint64_t sequence{0};
    uint64_t timestamp_ms{0};
    State state{State::Created};
    Kind kind{Kind::Started};
    Level level{Level::Info};
    std::string object_id;
    std::string message;
    std::map<std::string, std::string> fields;
};

static bool terminal(State s) {
    return s == State::Completed || s == State::Quarantined ||
           s == State::Failed;
}

static bool legal_transition(State from, State to) {
    if (from == State::Created && to == State::Running) return true;
    if (from == State::Running && to == State::Inspecting) return true;
    if (from == State::Running && to == State::Failed) return true;
    if (from == State::Inspecting && to == State::Extracting) return true;
    if (from == State::Inspecting && to == State::Finalizing) return true;
    if (from == State::Inspecting && to == State::Failed) return true;
    if (from == State::Extracting && to == State::Inspecting) return true;
    if (from == State::Extracting && to == State::Finalizing) return true;
    if (from == State::Extracting && to == State::Failed) return true;
    if (from == State::Finalizing && terminal(to)) return true;
    return false;
}

static const char* state_name(State s) {
    switch (s) {
    case State::Created: return "created";
    case State::Running: return "running";
    case State::Inspecting: return "inspecting";
    case State::Extracting: return "extracting";
    case State::Finalizing: return "finalizing";
    case State::Completed: return "completed";
    case State::Quarantined: return "quarantined";
    case State::Failed: return "failed";
    }
    return "unknown";
}

static const char* kind_name(Kind k) {
    switch (k) {
    case Kind::Started: return "started";
    case Kind::Progress: return "progress";
    case Kind::Evidence: return "evidence";
    case Kind::Boundary: return "boundary";
    case Kind::Verdict: return "verdict";
    case Kind::Failure: return "failure";
    case Kind::Completed: return "completed";
    }
    return "unknown";
}

static const char* level_name(Level l) {
    switch (l) {
    case Level::Trace: return "trace";
    case Level::Info: return "info";
    case Level::Notice: return "notice";
    case Level::Warning: return "warning";
    case Level::Critical: return "critical";
    }
    return "unknown";
}

static std::string escape(std::string_view value) {
    std::ostringstream out;
    for (const unsigned char c : value) {
        if (c == '\"') out << "\\\"";
        else if (c == '\\') out << "\\\\";
        else if (c == '\n') out << "\\n";
        else if (c == '\r') out << "\\r";
        else if (c == '\t') out << "\\t";
        else out << static_cast<char>(c);
    }
    return out.str();
}

class Herald {
public:
    explicit Herald(size_t queue_limit = 4096) : queue_limit_(queue_limit) {}

    bool start(std::string object_id, uint64_t now_ms) {
        if (object_id.empty() || state_ != State::Created) return false;
        object_id_ = std::move(object_id);
        return transition(State::Running, Kind::Started, Level::Info,
                          now_ms, "scan started");
    }

    bool inspect(uint64_t now_ms) {
        return transition(State::Inspecting, Kind::Progress, Level::Info,
                          now_ms, "inspection started");
    }

    bool extract(uint64_t now_ms) {
        return transition(State::Extracting, Kind::Progress, Level::Info,
                          now_ms, "container extraction started");
    }

    bool finalize(uint64_t now_ms) {
        return transition(State::Finalizing, Kind::Progress, Level::Info,
                          now_ms, "scan finalization started");
    }

    bool complete(uint64_t now_ms) {
        return transition(State::Completed, Kind::Completed, Level::Notice,
                          now_ms, "scan completed");
    }

    bool quarantine(uint64_t now_ms) {
        return transition(State::Quarantined, Kind::Completed,
                          Level::Warning, now_ms, "object isolated");
    }

    bool fail(uint64_t now_ms, std::string message) {
        return transition(State::Failed, Kind::Failure, Level::Critical,
                          now_ms, std::move(message));
    }

    bool evidence(uint64_t now_ms, Level level, std::string source,
                  std::string detail) {
        if (source.empty() || detail.empty()) return false;
        Event e = base(Kind::Evidence, level, now_ms, std::move(detail));
        e.fields.emplace("source", std::move(source));
        return append(std::move(e));
    }

    bool boundary(uint64_t now_ms, std::string name, uint64_t observed,
                  uint64_t limit) {
        if (name.empty()) return false;
        Event e = base(Kind::Boundary,
                       observed > limit ? Level::Warning : Level::Notice,
                       now_ms, std::move(name));
        e.fields.emplace("observed", std::to_string(observed));
        e.fields.emplace("limit", std::to_string(limit));
        return append(std::move(e));
    }

    bool verdict(uint64_t now_ms, std::string value, std::string reason,
                 std::string confidence) {
        if (value.empty() || reason.empty() || confidence.empty()) return false;
        Event e = base(Kind::Verdict, Level::Notice, now_ms,
                       "technical gate verdict");
        e.fields.emplace("verdict", std::move(value));
        e.fields.emplace("reason", std::move(reason));
        e.fields.emplace("confidence", std::move(confidence));
        return append(std::move(e));
    }

    bool progress(uint64_t now_ms, uint64_t bytes, uint64_t files,
                  uint32_t depth) {
        Event e = base(Kind::Progress, Level::Trace, now_ms, "scan progress");
        e.fields.emplace("bytes", std::to_string(bytes));
        e.fields.emplace("files", std::to_string(files));
        e.fields.emplace("depth", std::to_string(depth));
        return append(std::move(e));
    }

    State state() const { return state_; }
    uint64_t sequence() const { return sequence_; }
    const std::deque<Event>& events() const { return events_; }

    std::string json() const {
        std::ostringstream out;
        out << "{\"object_id\":\"" << escape(object_id_)
            << "\",\"state\":\"" << state_name(state_)
            << "\",\"sequence\":" << sequence_ << ",\"events\":[";
        bool first = true;
        for (const auto& e : events_) {
            if (!first) out << ',';
            first = false;
            out << event_json(e);
        }
        out << "]}";
        return out.str();
    }

private:
    Event base(Kind kind, Level level, uint64_t now_ms, std::string message) {
        Event e;
        e.sequence = sequence_ + 1;
        e.timestamp_ms = now_ms;
        e.state = state_;
        e.kind = kind;
        e.level = level;
        e.object_id = object_id_;
        e.message = std::move(message);
        return e;
    }

    bool append(Event e) {
        if (e.sequence != sequence_ + 1) return false;
        if (e.object_id.empty()) return false;
        if (queue_limit_ == 0) return false;
        while (events_.size() >= queue_limit_) events_.pop_front();
        events_.push_back(std::move(e));
        ++sequence_;
        return true;
    }

    bool transition(State next, Kind kind, Level level, uint64_t now_ms,
                    std::string message) {
        if (!legal_transition(state_, next)) return false;
        Event e = base(kind, level, now_ms, std::move(message));
        e.fields.emplace("from", state_name(state_));
        e.fields.emplace("to", state_name(next));
        if (!append(std::move(e))) return false;
        state_ = next;
        events_.back().state = state_;
        return true;
    }

    static std::string event_json(const Event& e) {
        std::ostringstream out;
        out << "{\"sequence\":" << e.sequence
            << ",\"timestamp_ms\":" << e.timestamp_ms
            << ",\"kind\":\"" << kind_name(e.kind)
            << "\",\"level\":\"" << level_name(e.level)
            << "\",\"state\":\"" << state_name(e.state)
            << "\",\"object_id\":\"" << escape(e.object_id)
            << "\",\"message\":\"" << escape(e.message) << "\"}";
        return out.str();
    }

    State state_{State::Created};
    uint64_t sequence_{0};
    size_t queue_limit_;
    std::string object_id_;
    std::deque<Event> events_;
};

/* Resource observations are heralded independently of the final verdict. */
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

/* Security evidence remains technical even when rendered for a human. */
std::map<std::string, std::string>
sanitize_display_fields(std::map<std::string, std::string> fields) {
    static const char* denied[] = {
        "age", "height", "celebrity", "religion", "politics",
        "reputation", "membership"
    };
    for (const char* key : denied) fields.erase(key);
    return fields;
}

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

} // namespace legal_clam_herald
