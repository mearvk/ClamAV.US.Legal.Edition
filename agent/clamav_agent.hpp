/*
 * clamav_agent.hpp — System agent that sits co-concerned with ClamAV.
 *
 * Author: Max Rupplin - MEARVK LLC - 2026.
 *
 * ---------------------------------------------------------------------------
 * WHAT THIS IS
 * ---------------------------------------------------------------------------
 * The gate (gating.cpp) DECIDES and the herald (herald.cpp) NARRATES, but both
 * are in-process reference engines. This unit lets them sit *linkedly relevant*
 * to the actual ClamAV services that a running system operates:
 *
 *   - clamd        the resident scanning daemon (LocalSocket + PidFile),
 *   - freshclam    the signature-database updater,
 *   - clamonacc    on-access scanning (depends on clamd),
 *   - clamav-milter the optional mail filter.
 *
 * The agent OBSERVES those services through a control backend (systemctl or a
 * clamavctl shim), maps what it observes into the same procedural vocabulary
 * the gate and herald already speak, and proposes — never silently performs —
 * corrective service actions.
 *
 * ---------------------------------------------------------------------------
 * SAFETY POSTURE (inherits the six rules; adds operational ones)
 * ---------------------------------------------------------------------------
 * The agent is additive to ClamAV, exactly like the rest of this layer:
 *
 *   O1. It NEVER suppresses or clears a ClamAV detection. It can keep services
 *       alive; it cannot make a dirty object look clean.
 *   O2. If clamd is not confirmably running, object safety is UNKNOWN, and
 *       UNKNOWN is fail-closed (never "clean").
 *   O3. A stale signature database ATTENUATES confidence; it does not by itself
 *       produce a clean verdict.
 *   O4. clamonacc active while clamd is not active is an inconsistent posture
 *       and is treated as a quarantine-grade condition, not ignored.
 *   O5. Every mutating control command (start/stop/restart/reload/enable) is
 *       DRY-RUN by default. Real execution requires an explicit opt-in AND a
 *       present backend binary. The plan is always shown before it could run.
 *   O6. Unknown/undetectable state is treated as the more cautious state.
 *
 * ---------------------------------------------------------------------------
 * DEPENDENCIES
 * ---------------------------------------------------------------------------
 * Self-contained C++17, standard library only, so it compiles and is reviewed
 * in isolation. It does not #include the gate or herald; instead it defines a
 * small neutral "bridge" vocabulary (AgentObservation / AgentPosture) that the
 * gate and herald adapters map to/from. This keeps each translation unit
 * independently buildable while remaining co-concerned by design.
 */

#ifndef MEARVK_CLAMAV_AGENT_HPP
#define MEARVK_CLAMAV_AGENT_HPP

#include <array>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace legal_clam_agent {

// ===========================================================================
// SECTION 1 — Service identity and lifecycle state
// ===========================================================================

// The ClamAV services the agent is co-concerned with.
enum class ServiceKind { Clamd, Freshclam, Clamonacc, Milter };

// systemd's ActiveState values, plus an Unknown for undetectable state. Per
// safety rule O6, Unknown is treated as the more cautious (not-running) case.
enum class ActiveState {
    Unknown,
    Active,
    Reloading,
    Inactive,
    Failed,
    Activating,
    Deactivating
};

// systemd's SubState is service-type specific; we keep the ones that matter
// for a forking/simple daemon plus Unknown.
enum class SubState {
    Unknown,
    Running,
    Exited,
    Dead,
    FailedSub,
    Start,
    Stop,
    AutoRestart
};

// Whether the unit is wired to start at boot.
enum class EnableState { Unknown, Enabled, Disabled, Static, Masked };

static const char* service_kind_name(ServiceKind k) {
    switch (k) {
    case ServiceKind::Clamd:     return "clamd";
    case ServiceKind::Freshclam: return "freshclam";
    case ServiceKind::Clamonacc: return "clamonacc";
    case ServiceKind::Milter:    return "clamav-milter";
    }
    return "unknown";
}

static const char* active_state_name(ActiveState s) {
    switch (s) {
    case ActiveState::Unknown:      return "unknown";
    case ActiveState::Active:       return "active";
    case ActiveState::Reloading:    return "reloading";
    case ActiveState::Inactive:     return "inactive";
    case ActiveState::Failed:       return "failed";
    case ActiveState::Activating:   return "activating";
    case ActiveState::Deactivating: return "deactivating";
    }
    return "unknown";
}

static const char* sub_state_name(SubState s) {
    switch (s) {
    case SubState::Unknown:     return "unknown";
    case SubState::Running:     return "running";
    case SubState::Exited:      return "exited";
    case SubState::Dead:        return "dead";
    case SubState::FailedSub:   return "failed";
    case SubState::Start:       return "start";
    case SubState::Stop:        return "stop";
    case SubState::AutoRestart: return "auto-restart";
    }
    return "unknown";
}

static const char* enable_state_name(EnableState s) {
    switch (s) {
    case EnableState::Unknown:  return "unknown";
    case EnableState::Enabled:  return "enabled";
    case EnableState::Disabled: return "disabled";
    case EnableState::Static:   return "static";
    case EnableState::Masked:   return "masked";
    }
    return "unknown";
}

// ===========================================================================
// SECTION 2 — Text utilities (dependency-free, deterministic)
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

static std::string trim(std::string_view s) {
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == ' ' || s[b] == '\t' || s[b] == '\r' ||
                     s[b] == '\n'))
        ++b;
    while (e > b && (s[e - 1] == ' ' || s[e - 1] == '\t' || s[e - 1] == '\r' ||
                     s[e - 1] == '\n'))
        --e;
    return std::string(s.substr(b, e - b));
}

static bool iequals(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        char ca = a[i], cb = b[i];
        if (ca >= 'A' && ca <= 'Z') ca = static_cast<char>(ca - 'A' + 'a');
        if (cb >= 'A' && cb <= 'Z') cb = static_cast<char>(cb - 'A' + 'a');
        if (ca != cb) return false;
    }
    return true;
}

// ===========================================================================
// SECTION 3 — Distro unit-name mapping
// ===========================================================================
//
// Different distributions name the units differently. Debian/Ubuntu use
// clamav-daemon.service and clamav-freshclam.service; Fedora/RHEL use
// clamd@scan.service and freshclam.service (or clamav-freshclam). The agent
// keeps a mapping keyed by a Distro hint and falls back to the most common
// name. This is data, not logic, so it is easy to review and extend.

enum class Distro { Generic, Debian, RedHat };

struct UnitNaming {
    std::string clamd;
    std::string freshclam;
    std::string clamonacc;
    std::string milter;
};

static UnitNaming unit_naming(Distro distro) {
    switch (distro) {
    case Distro::Debian:
        return UnitNaming{"clamav-daemon.service",
                          "clamav-freshclam.service",
                          "clamav-clamonacc.service",
                          "clamav-milter.service"};
    case Distro::RedHat:
        return UnitNaming{"clamd@scan.service",
                          "freshclam.service",
                          "clamonacc.service",
                          "clamav-milter.service"};
    case Distro::Generic:
    default:
        return UnitNaming{"clamd.service",
                          "freshclam.service",
                          "clamonacc.service",
                          "clamav-milter.service"};
    }
}

static std::string unit_name_for(ServiceKind kind, Distro distro) {
    const UnitNaming n = unit_naming(distro);
    switch (kind) {
    case ServiceKind::Clamd:     return n.clamd;
    case ServiceKind::Freshclam: return n.freshclam;
    case ServiceKind::Clamonacc: return n.clamonacc;
    case ServiceKind::Milter:    return n.milter;
    }
    return "unknown.service";
}

// ===========================================================================
// SECTION 4 — ServiceUnit model
// ===========================================================================
//
// A ServiceUnit is the agent's observed picture of one unit at one moment.
// Everything here is a measurable system fact; nothing about a person.

struct ServiceUnit {
    ServiceKind kind{ServiceKind::Clamd};
    std::string unit_name;
    ActiveState active{ActiveState::Unknown};
    SubState sub{SubState::Unknown};
    EnableState enabled{EnableState::Unknown};
    uint64_t main_pid{0};
    uint64_t since_ms{0};       // ActiveEnterTimestamp, epoch ms if known
    bool observed{false};       // did we actually read state, or guess?

    // Fail-closed liveness (O2/O6): a unit is "confirmably running" only if we
    // actually observed it Active/Running. Unknown never counts as running.
    bool confirmably_running() const {
        return observed && active == ActiveState::Active &&
               (sub == SubState::Running || sub == SubState::Exited);
    }

    bool confirmably_failed() const {
        return observed && (active == ActiveState::Failed ||
                            sub == SubState::FailedSub);
    }

    std::string serialize() const {
        std::ostringstream out;
        out << "{\"kind\":\"" << service_kind_name(kind)
            << "\",\"unit\":\"" << escape_json(unit_name)
            << "\",\"active\":\"" << active_state_name(active)
            << "\",\"sub\":\"" << sub_state_name(sub)
            << "\",\"enabled\":\"" << enable_state_name(enabled)
            << "\",\"main_pid\":" << main_pid
            << ",\"since_ms\":" << since_ms
            << ",\"observed\":" << (observed ? "true" : "false")
            << ",\"confirmably_running\":"
            << (confirmably_running() ? "true" : "false") << "}";
        return out.str();
    }
};

// ===========================================================================
// SECTION 5 — Parsing systemctl output
// ===========================================================================
//
// The agent parses two kinds of output:
//   (a) `systemctl is-active <unit>`   -> a single word,
//   (b) `systemctl show <unit> -p ...` -> KEY=VALUE lines.
// Parsing is total: unrecognized input maps to Unknown, never to a running
// state (O6).

static ActiveState parse_active_state(std::string_view word) {
    const std::string w = trim(word);
    if (iequals(w, "active"))       return ActiveState::Active;
    if (iequals(w, "reloading"))    return ActiveState::Reloading;
    if (iequals(w, "inactive"))     return ActiveState::Inactive;
    if (iequals(w, "failed"))       return ActiveState::Failed;
    if (iequals(w, "activating"))   return ActiveState::Activating;
    if (iequals(w, "deactivating")) return ActiveState::Deactivating;
    return ActiveState::Unknown;
}

static SubState parse_sub_state(std::string_view word) {
    const std::string w = trim(word);
    if (iequals(w, "running"))      return SubState::Running;
    if (iequals(w, "exited"))       return SubState::Exited;
    if (iequals(w, "dead"))         return SubState::Dead;
    if (iequals(w, "failed"))       return SubState::FailedSub;
    if (iequals(w, "start"))        return SubState::Start;
    if (iequals(w, "stop"))         return SubState::Stop;
    if (iequals(w, "auto-restart")) return SubState::AutoRestart;
    return SubState::Unknown;
}

static EnableState parse_enable_state(std::string_view word) {
    const std::string w = trim(word);
    if (iequals(w, "enabled"))  return EnableState::Enabled;
    if (iequals(w, "disabled")) return EnableState::Disabled;
    if (iequals(w, "static"))   return EnableState::Static;
    if (iequals(w, "masked"))   return EnableState::Masked;
    return EnableState::Unknown;
}

// Parse KEY=VALUE lines from `systemctl show`. Only the keys we care about are
// retained; everything else is ignored. Returns a map for the caller to fold
// into a ServiceUnit.
static std::map<std::string, std::string>
parse_show_properties(std::string_view text) {
    std::map<std::string, std::string> kv;
    size_t i = 0;
    const size_t n = text.size();
    while (i < n) {
        size_t eol = text.find('\n', i);
        if (eol == std::string_view::npos) eol = n;
        const auto line = text.substr(i, eol - i);
        const size_t eq = line.find('=');
        if (eq != std::string_view::npos) {
            const std::string key = trim(line.substr(0, eq));
            const std::string val = trim(line.substr(eq + 1));
            if (!key.empty()) kv[key] = val;
        }
        i = eol + 1;
    }
    return kv;
}

// Fold a parsed property map into a ServiceUnit. Missing properties leave the
// corresponding field Unknown (O6).
static ServiceUnit unit_from_properties(
    ServiceKind kind, std::string unit_name,
    const std::map<std::string, std::string>& kv) {
    ServiceUnit u;
    u.kind = kind;
    u.unit_name = std::move(unit_name);
    u.observed = !kv.empty();
    if (auto it = kv.find("ActiveState"); it != kv.end())
        u.active = parse_active_state(it->second);
    if (auto it = kv.find("SubState"); it != kv.end())
        u.sub = parse_sub_state(it->second);
    if (auto it = kv.find("UnitFileState"); it != kv.end())
        u.enabled = parse_enable_state(it->second);
    if (auto it = kv.find("MainPID"); it != kv.end())
        u.main_pid = static_cast<uint64_t>(std::atoll(it->second.c_str()));
    if (auto it = kv.find("ActiveEnterTimestampMonotonic"); it != kv.end())
        u.since_ms = static_cast<uint64_t>(
                         std::atoll(it->second.c_str())) / 1000ULL;
    return u;
}

// ===========================================================================
// SECTION 6 — Planned commands and the execution policy
// ===========================================================================
//
// The agent never runs a mutating command as a side effect of observing. Every
// action is materialized as a PlannedCommand that a caller can inspect, log,
// and — only if the ExecutionPolicy allows — hand to the backend to run.

enum class Action { Status, IsActive, Show, Start, Stop, Restart, Reload,
                    Enable, Disable };

static const char* action_name(Action a) {
    switch (a) {
    case Action::Status:   return "status";
    case Action::IsActive: return "is-active";
    case Action::Show:     return "show";
    case Action::Start:    return "start";
    case Action::Stop:     return "stop";
    case Action::Restart:  return "restart";
    case Action::Reload:   return "reload";
    case Action::Enable:   return "enable";
    case Action::Disable:  return "disable";
    }
    return "unknown";
}

// An action is "mutating" if it can change service state. Observational
// actions (status/is-active/show) are always safe to run.
static bool action_is_mutating(Action a) {
    switch (a) {
    case Action::Start:
    case Action::Stop:
    case Action::Restart:
    case Action::Reload:
    case Action::Enable:
    case Action::Disable:
        return true;
    default:
        return false;
    }
}

struct PlannedCommand {
    Action action{Action::Status};
    std::string program;              // "systemctl" or "clamavctl"
    std::vector<std::string> argv;    // full argument vector, program first
    std::string unit;                 // target unit (if any)
    std::string rationale;            // why the agent proposes this

    // The command line as a single reviewable string. This is what a dry-run
    // prints; it is NOT executed by this type.
    std::string command_line() const {
        std::ostringstream out;
        bool first = true;
        for (const auto& a : argv) {
            if (!first) out << ' ';
            first = false;
            // Quote args containing spaces so the printed line is copyable.
            if (a.find(' ') != std::string::npos)
                out << '"' << a << '"';
            else
                out << a;
        }
        return out.str();
    }

    std::string serialize() const {
        std::ostringstream out;
        out << "{\"action\":\"" << action_name(action)
            << "\",\"mutating\":" << (action_is_mutating(action) ? "true"
                                                                 : "false")
            << ",\"unit\":\"" << escape_json(unit)
            << "\",\"command\":\"" << escape_json(command_line())
            << "\",\"rationale\":\"" << escape_json(rationale) << "\"}";
        return out.str();
    }
};

// The execution mode. DryRun is the default and the safe posture (O5).
enum class ExecutionMode { DryRun, Execute };

struct ExecutionPolicy {
    ExecutionMode mode{ExecutionMode::DryRun};
    bool backend_present{false};   // is the control binary actually installed?
    bool allow_stop{false};        // stopping ClamAV is extra-guarded
    bool allow_disable{false};     // disabling at boot is extra-guarded

    // The central gate for whether a planned command may actually run. This is
    // the single chokepoint every executor must pass through.
    bool may_execute(const PlannedCommand& cmd) const {
        // Observational commands are always permitted (they cannot change
        // ClamAV's protective state).
        if (!action_is_mutating(cmd.action)) return true;
        // Mutating commands require Execute mode AND a present backend.
        if (mode != ExecutionMode::Execute) return false;
        if (!backend_present) return false;
        // Stopping or disabling ClamAV weakens protection; those need their
        // own explicit opt-in on top of Execute mode (defense in depth).
        if (cmd.action == Action::Stop && !allow_stop) return false;
        if (cmd.action == Action::Disable && !allow_disable) return false;
        return true;
    }

    std::string serialize() const {
        std::ostringstream out;
        out << "{\"mode\":\""
            << (mode == ExecutionMode::Execute ? "execute" : "dry-run")
            << "\",\"backend_present\":" << (backend_present ? "true" : "false")
            << ",\"allow_stop\":" << (allow_stop ? "true" : "false")
            << ",\"allow_disable\":" << (allow_disable ? "true" : "false")
            << "}";
        return out.str();
    }
};

// ===========================================================================
// SECTION 7 — Control backend abstraction
// ===========================================================================
//
// A ControlBackend knows how to PLAN commands for a given backend program. It
// deliberately does NOT run them; running is the ControlRunner's job and is
// gated by the ExecutionPolicy. Two backends are provided: systemctl and a
// clamavctl shim. Both produce identical PlannedCommand shapes so the rest of
// the agent is backend-agnostic.

class ControlBackend {
public:
    virtual ~ControlBackend() = default;
    virtual std::string program() const = 0;
    virtual PlannedCommand plan(Action action, const std::string& unit,
                                std::string rationale) const = 0;
};

class SystemctlBackend : public ControlBackend {
public:
    explicit SystemctlBackend(bool use_sudo = false) : use_sudo_(use_sudo) {}

    std::string program() const override { return "systemctl"; }

    PlannedCommand plan(Action action, const std::string& unit,
                        std::string rationale) const override {
        PlannedCommand cmd;
        cmd.action = action;
        cmd.program = "systemctl";
        cmd.unit = unit;
        cmd.rationale = std::move(rationale);
        if (use_sudo_ && action_is_mutating(action))
            cmd.argv.push_back("sudo");
        cmd.argv.push_back("systemctl");
        switch (action) {
        case Action::Status:
            cmd.argv.push_back("status");
            cmd.argv.push_back(unit);
            break;
        case Action::IsActive:
            cmd.argv.push_back("is-active");
            cmd.argv.push_back(unit);
            break;
        case Action::Show:
            cmd.argv.push_back("show");
            cmd.argv.push_back(unit);
            cmd.argv.push_back("-p");
            cmd.argv.push_back("ActiveState,SubState,UnitFileState,MainPID,"
                               "ActiveEnterTimestampMonotonic");
            break;
        case Action::Start:   cmd.argv.push_back("start");   cmd.argv.push_back(unit); break;
        case Action::Stop:    cmd.argv.push_back("stop");    cmd.argv.push_back(unit); break;
        case Action::Restart: cmd.argv.push_back("restart"); cmd.argv.push_back(unit); break;
        case Action::Reload:  cmd.argv.push_back("reload");  cmd.argv.push_back(unit); break;
        case Action::Enable:  cmd.argv.push_back("enable");  cmd.argv.push_back(unit); break;
        case Action::Disable: cmd.argv.push_back("disable"); cmd.argv.push_back(unit); break;
        }
        return cmd;
    }

private:
    bool use_sudo_;
};

// A clamavctl backend: a thin project-specific control shim that fronts the
// same actions. In deployments without systemd (containers, minimal images),
// clamavctl can wrap direct clamd/freshclam invocations. Here we model its
// command surface; the shell shim in tools/ provides the actual dispatch.
class ClamavctlBackend : public ControlBackend {
public:
    std::string program() const override { return "clamavctl"; }

    PlannedCommand plan(Action action, const std::string& unit,
                        std::string rationale) const override {
        PlannedCommand cmd;
        cmd.action = action;
        cmd.program = "clamavctl";
        cmd.unit = unit;
        cmd.rationale = std::move(rationale);
        cmd.argv.push_back("clamavctl");
        cmd.argv.push_back(action_name(action));
        cmd.argv.push_back(unit);
        return cmd;
    }
};

// The result of (possibly) running a planned command. In dry-run, ran==false
// and the command_line is preserved for the operator to inspect.
struct ControlResult {
    PlannedCommand command;
    bool ran{false};
    bool permitted{false};
    int exit_code{-1};
    std::string note;

    std::string serialize() const {
        std::ostringstream out;
        out << "{\"permitted\":" << (permitted ? "true" : "false")
            << ",\"ran\":" << (ran ? "true" : "false")
            << ",\"exit_code\":" << exit_code
            << ",\"note\":\"" << escape_json(note)
            << "\",\"command\":" << command.serialize() << "}";
        return out.str();
    }
};

// The runner takes a policy and a command executor. The executor is injected
// so that production code can supply a real process spawner while tests supply
// a deterministic stub. The runner itself only ever runs a command the policy
// permits — this is the single execution chokepoint (O5).
using CommandExecutor =
    std::function<std::pair<int, std::string>(const PlannedCommand&)>;

class ControlRunner {
public:
    ControlRunner(ExecutionPolicy policy, CommandExecutor executor)
        : policy_(std::move(policy)), executor_(std::move(executor)) {}

    ControlResult run(const PlannedCommand& cmd) {
        ControlResult r;
        r.command = cmd;
        r.permitted = policy_.may_execute(cmd);
        if (!r.permitted) {
            r.note = action_is_mutating(cmd.action)
                         ? "mutating command withheld by execution policy "
                           "(dry-run or guard)"
                         : "command not run";
            return r;
        }
        if (!executor_) {
            r.note = "no executor bound; command not run";
            return r;
        }
        const auto [code, note] = executor_(cmd);
        r.ran = true;
        r.exit_code = code;
        r.note = note;
        return r;
    }

    const ExecutionPolicy& policy() const { return policy_; }

private:
    ExecutionPolicy policy_;
    CommandExecutor executor_;
};

// ===========================================================================
// SECTION 8 — Database freshness (freshclam co-concern)
// ===========================================================================
//
// A running clamd with a stale database is not the same as a protected system.
// The agent models database freshness explicitly so that staleness ATTENUATES
// confidence (O3) rather than being ignored.

struct DatabaseState {
    std::string directory;      // e.g. /var/lib/clamav
    uint64_t main_cvd_age_ms{0};// age of the main signature set
    uint64_t daily_age_ms{0};   // age of the daily delta
    uint64_t max_age_ms{0};     // freshness threshold (from Checks/day policy)
    bool present{false};        // are the databases present at all?

    bool stale() const {
        if (!present) return true;      // absent is worse than stale
        if (max_age_ms == 0) return false;
        return main_cvd_age_ms > max_age_ms || daily_age_ms > max_age_ms;
    }

    std::string serialize() const {
        std::ostringstream out;
        out << "{\"directory\":\"" << escape_json(directory)
            << "\",\"present\":" << (present ? "true" : "false")
            << ",\"main_age_ms\":" << main_cvd_age_ms
            << ",\"daily_age_ms\":" << daily_age_ms
            << ",\"max_age_ms\":" << max_age_ms
            << ",\"stale\":" << (stale() ? "true" : "false") << "}";
        return out.str();
    }
};

// Translate a freshclam "Checks per day" setting into a max acceptable age.
// If freshclam checks N times a day, a database older than roughly two check
// intervals is treated as stale (one missed check is tolerated).
static uint64_t max_age_from_checks_per_day(uint32_t checks_per_day) {
    if (checks_per_day == 0) return 0; // no policy => no staleness gate
    const uint64_t day_ms = 24ULL * 60ULL * 60ULL * 1000ULL;
    const uint64_t interval = day_ms / checks_per_day;
    return interval * 2ULL;
}

// ===========================================================================
// SECTION 9 — clamd socket liveness (execution-channel co-concern)
// ===========================================================================
//
// systemd may report clamd Active before it is actually answering on its
// socket. The agent therefore models socket liveness separately: a service is
// only treated as fully live when BOTH the unit is confirmably running AND its
// socket answers. Absent a probe, socket liveness is Unknown (O6).

enum class SocketLiveness { Unknown, Answering, Silent, NoSocket };

static const char* socket_liveness_name(SocketLiveness s) {
    switch (s) {
    case SocketLiveness::Unknown:   return "unknown";
    case SocketLiveness::Answering: return "answering";
    case SocketLiveness::Silent:    return "silent";
    case SocketLiveness::NoSocket:  return "no-socket";
    }
    return "unknown";
}

struct ClamdEndpoint {
    std::string local_socket;   // e.g. /run/clamav/clamd.sock
    uint32_t tcp_port{0};       // e.g. 3310, 0 if unused
    SocketLiveness liveness{SocketLiveness::Unknown};

    // A probe (e.g. sending "PING" expecting "PONG") sets this. The probe
    // itself lives outside the header so this stays dependency-free.
    bool answering() const { return liveness == SocketLiveness::Answering; }

    std::string serialize() const {
        std::ostringstream out;
        out << "{\"local_socket\":\"" << escape_json(local_socket)
            << "\",\"tcp_port\":" << tcp_port
            << ",\"liveness\":\"" << socket_liveness_name(liveness) << "\"}";
        return out.str();
    }
};

// ===========================================================================
// SECTION 10 — The neutral bridge vocabulary
// ===========================================================================
//
// Rather than couple to the gate/herald headers, the agent produces a neutral
// AgentObservation that adapters (Section 14) translate into the gate's
// Observation and the herald's events. This keeps every translation unit
// independently buildable while remaining co-concerned.

// The overall operational posture the agent derives. Note the deliberate
// alignment with the gate's verdict language, so the adapter is a near-direct
// mapping.
enum class Posture {
    Protected,     // clamd live + fresh db  -> gate may proceed
    Degraded,      // running but stale/uncertain -> attenuate, do not clear
    Unprotected,   // clamd not confirmably running -> fail closed (UNKNOWN)
    Inconsistent,  // e.g. clamonacc without clamd -> quarantine-grade
    Failed         // a required unit is in failed state
};

static const char* posture_name(Posture p) {
    switch (p) {
    case Posture::Protected:    return "protected";
    case Posture::Degraded:     return "degraded";
    case Posture::Unprotected:  return "unprotected";
    case Posture::Inconsistent: return "inconsistent";
    case Posture::Failed:       return "failed";
    }
    return "unknown";
}

struct AgentObservation {
    ServiceUnit clamd;
    ServiceUnit freshclam;
    ServiceUnit clamonacc;
    ClamdEndpoint endpoint;
    DatabaseState database;
    Posture posture{Posture::Unprotected};
    std::string explanation;
    std::vector<std::string> attenuations;   // reasons confidence is reduced
    std::vector<PlannedCommand> proposals;   // corrective actions proposed

    std::string serialize() const {
        std::ostringstream out;
        out << "{\"posture\":\"" << posture_name(posture)
            << "\",\"explanation\":\"" << escape_json(explanation)
            << "\",\"clamd\":" << clamd.serialize()
            << ",\"freshclam\":" << freshclam.serialize()
            << ",\"clamonacc\":" << clamonacc.serialize()
            << ",\"endpoint\":" << endpoint.serialize()
            << ",\"database\":" << database.serialize()
            << ",\"attenuations\":[";
        bool first = true;
        for (const auto& a : attenuations) {
            if (!first) out << ',';
            first = false;
            out << '"' << escape_json(a) << '"';
        }
        out << "],\"proposals\":[";
        first = true;
        for (const auto& p : proposals) {
            if (!first) out << ',';
            first = false;
            out << p.serialize();
        }
        out << "]}";
        return out.str();
    }
};

// ===========================================================================
// SECTION 11 — Posture derivation (the co-concern logic)
// ===========================================================================
//
// This is where the agent becomes co-concerned with ClamAV. It reads the
// observed units, endpoint, and database, and derives a Posture under the
// operational safety rules. The derivation is ordered most-severe first so it
// can only become more cautious as it proceeds (mirroring the gate's evaluator
// discipline).

class PostureEngine {
public:
    explicit PostureEngine(Distro distro = Distro::Generic) : distro_(distro) {}

    AgentObservation derive(ServiceUnit clamd, ServiceUnit freshclam,
                            ServiceUnit clamonacc, ClamdEndpoint endpoint,
                            DatabaseState database) const {
        AgentObservation o;
        o.clamd = std::move(clamd);
        o.freshclam = std::move(freshclam);
        o.clamonacc = std::move(clamonacc);
        o.endpoint = std::move(endpoint);
        o.database = std::move(database);

        const SystemctlBackend backend;

        // Rule: a failed required unit is the most severe posture.
        if (o.clamd.confirmably_failed()) {
            o.posture = Posture::Failed;
            o.explanation = "clamd unit is in a failed state";
            o.proposals.push_back(backend.plan(
                Action::Restart, o.clamd.unit_name,
                "restart clamd to restore resident scanning"));
            return o;
        }

        // O4: clamonacc active while clamd is NOT confirmably running is an
        // inconsistent, quarantine-grade posture: on-access scanning cannot
        // function without the daemon, so the system may believe it is
        // protected when it is not.
        if (o.clamonacc.confirmably_running() &&
            !o.clamd.confirmably_running()) {
            o.posture = Posture::Inconsistent;
            o.explanation = "clamonacc is active but clamd is not confirmably "
                            "running; on-access protection is illusory";
            o.proposals.push_back(backend.plan(
                Action::Start, o.clamd.unit_name,
                "start clamd so on-access scanning has a working backend"));
            return o;
        }

        // O2: if clamd is not confirmably running, or its socket is not
        // answering, object safety is UNKNOWN, which is fail-closed.
        if (!o.clamd.confirmably_running()) {
            o.posture = Posture::Unprotected;
            o.explanation = "clamd is not confirmably running; scan results "
                            "are unavailable and safety is unknown";
            o.proposals.push_back(backend.plan(
                Action::Start, o.clamd.unit_name,
                "start clamd to make resident scanning available"));
            return o;
        }
        if (o.endpoint.liveness == SocketLiveness::Silent ||
            o.endpoint.liveness == SocketLiveness::NoSocket) {
            o.posture = Posture::Unprotected;
            o.explanation = "clamd unit is active but its socket is not "
                            "answering; treat safety as unknown";
            o.proposals.push_back(backend.plan(
                Action::Restart, o.clamd.unit_name,
                "restart clamd to recover a non-answering socket"));
            return o;
        }

        // O3: a running clamd with a stale or absent database is DEGRADED, not
        // protected. Staleness attenuates confidence; it never clears anything.
        if (o.database.stale()) {
            o.posture = Posture::Degraded;
            o.explanation = o.database.present
                ? "clamd is running but the signature database is stale"
                : "clamd is running but the signature database is absent";
            o.attenuations.push_back(
                o.database.present ? "stale signature database"
                                   : "absent signature database");
            o.proposals.push_back(backend.plan(
                Action::Start, o.freshclam.unit_name,
                "run freshclam to refresh the signature database"));
            return o;
        }

        // If freshclam is not running/enabled, the database will drift over
        // time. That is a soft degradation: still protected now, but the agent
        // proposes enabling the updater and records an attenuation.
        if (!o.freshclam.confirmably_running() &&
            o.freshclam.enabled != EnableState::Enabled) {
            o.posture = Posture::Degraded;
            o.explanation = "clamd is protected now, but freshclam is neither "
                            "running nor enabled; protection will drift";
            o.attenuations.push_back("updater not scheduled");
            o.proposals.push_back(backend.plan(
                Action::Enable, o.freshclam.unit_name,
                "enable freshclam so signatures stay current"));
            return o;
        }

        // All clear: clamd live, socket answering, database fresh.
        o.posture = Posture::Protected;
        o.explanation = "clamd is live, its socket answers, and the signature "
                        "database is fresh";
        return o;
    }

private:
    Distro distro_;
};

// ===========================================================================
// SECTION 12 — Supervisor (reconcile observed vs desired)
// ===========================================================================
//
// The supervisor turns a Posture into a set of PlannedCommands that would move
// the system toward the desired protected state — but it hands them to a
// ControlRunner, which only runs what the ExecutionPolicy permits. Thus the
// supervisor can "keep services alive" (O1) while being structurally unable to
// weaken protection or clear a detection.

struct SupervisionResult {
    AgentObservation observation;
    std::vector<ControlResult> actions;
    bool converged{false};   // is the system already in the protected posture?

    std::string serialize() const {
        std::ostringstream out;
        out << "{\"converged\":" << (converged ? "true" : "false")
            << ",\"observation\":" << observation.serialize()
            << ",\"actions\":[";
        bool first = true;
        for (const auto& a : actions) {
            if (!first) out << ',';
            first = false;
            out << a.serialize();
        }
        out << "]}";
        return out.str();
    }
};

class Supervisor {
public:
    Supervisor(ExecutionPolicy policy, CommandExecutor executor)
        : runner_(std::move(policy), std::move(executor)) {}

    SupervisionResult supervise(const AgentObservation& obs) {
        SupervisionResult result;
        result.observation = obs;
        result.converged = (obs.posture == Posture::Protected);

        // O1 restated in code: the supervisor only ever runs the corrective
        // proposals the posture engine emitted. Those proposals are, by
        // construction, restorative (start/restart/reload/enable) — never a
        // stop/disable of clamd, and never anything that touches a scan verdict.
        for (const auto& proposal : obs.proposals) {
            // Defensive: refuse to run a proposal that would weaken clamd, even
            // if one somehow appeared. Protection is monotonic here.
            if ((proposal.action == Action::Stop ||
                 proposal.action == Action::Disable) &&
                proposal.unit == obs.clamd.unit_name) {
                ControlResult r;
                r.command = proposal;
                r.permitted = false;
                r.note = "refused: supervisor never weakens clamd protection";
                result.actions.push_back(std::move(r));
                continue;
            }
            result.actions.push_back(runner_.run(proposal));
        }
        return result;
    }

    const ExecutionPolicy& policy() const { return runner_.policy(); }

private:
    ControlRunner runner_;
};

// ===========================================================================
// SECTION 13 — Freshclam config reading (Checks -> max age)
// ===========================================================================
//
// A small helper to read the "Checks" directive from a freshclam.conf so the
// database freshness threshold reflects the operator's own update cadence.
// Parsing is line-oriented and tolerant: comments (#) and unknown directives
// are ignored.

static std::optional<uint32_t>
freshclam_checks_from_conf(std::string_view conf_text) {
    size_t i = 0;
    const size_t n = conf_text.size();
    while (i < n) {
        size_t eol = conf_text.find('\n', i);
        if (eol == std::string_view::npos) eol = n;
        const std::string line = trim(conf_text.substr(i, eol - i));
        i = eol + 1;
        if (line.empty() || line[0] == '#') continue;
        // Expect: "Checks <n>"
        if (line.size() > 6 && iequals(std::string_view(line).substr(0, 6),
                                       "Checks")) {
            const std::string rest = trim(std::string_view(line).substr(6));
            if (!rest.empty()) {
                const int v = std::atoi(rest.c_str());
                if (v > 0) return static_cast<uint32_t>(v);
            }
        }
    }
    return std::nullopt;
}

// ===========================================================================
// SECTION 14 — Adapters to the gate and herald vocabularies
// ===========================================================================
//
// These adapters emit the SAME neutral strings the gate and herald use, so the
// bridge .cpp (agent/bridge.cpp) can wire them without either engine header
// being visible here. Keeping the mapping as pure functions of the posture
// makes the co-concern auditable in one place.

// The gate-facing recommendation: what the agent believes the gate's stance
// toward NEW objects should be, given the service posture. This never touches
// an existing per-object verdict (O1) — it only conditions how much the gate
// should trust a "clean" scan produced under this posture.
enum class GateStance { MayProceed, ProceedAttenuated, HoldFailClosed,
                        QuarantineGrade };

static const char* gate_stance_name(GateStance s) {
    switch (s) {
    case GateStance::MayProceed:        return "may_proceed";
    case GateStance::ProceedAttenuated: return "proceed_attenuated";
    case GateStance::HoldFailClosed:    return "hold_fail_closed";
    case GateStance::QuarantineGrade:   return "quarantine_grade";
    }
    return "unknown";
}

static GateStance gate_stance_for(Posture p) {
    switch (p) {
    case Posture::Protected:    return GateStance::MayProceed;
    case Posture::Degraded:     return GateStance::ProceedAttenuated;
    case Posture::Unprotected:  return GateStance::HoldFailClosed;
    case Posture::Inconsistent: return GateStance::QuarantineGrade;
    case Posture::Failed:       return GateStance::HoldFailClosed;
    }
    return GateStance::HoldFailClosed;
}

// The herald-facing severity for narrating this posture.
enum class HeraldSeverity { Trace, Info, Notice, Warning, Critical };

static const char* herald_severity_name(HeraldSeverity s) {
    switch (s) {
    case HeraldSeverity::Trace:    return "trace";
    case HeraldSeverity::Info:     return "info";
    case HeraldSeverity::Notice:   return "notice";
    case HeraldSeverity::Warning:  return "warning";
    case HeraldSeverity::Critical: return "critical";
    }
    return "info";
}

static HeraldSeverity herald_severity_for(Posture p) {
    switch (p) {
    case Posture::Protected:    return HeraldSeverity::Notice;
    case Posture::Degraded:     return HeraldSeverity::Warning;
    case Posture::Unprotected:  return HeraldSeverity::Critical;
    case Posture::Inconsistent: return HeraldSeverity::Critical;
    case Posture::Failed:       return HeraldSeverity::Critical;
    }
    return HeraldSeverity::Info;
}

// A compact, self-describing bridge record the wiring layer can hand to either
// engine. It intentionally uses strings so the two engines need not share ABI.
struct BridgeRecord {
    std::string posture;
    std::string gate_stance;
    std::string herald_severity;
    std::string explanation;
    double confidence_multiplier{1.0}; // <1.0 when degraded (attenuation)

    std::string serialize() const {
        std::ostringstream out;
        out << "{\"posture\":\"" << escape_json(posture)
            << "\",\"gate_stance\":\"" << escape_json(gate_stance)
            << "\",\"herald_severity\":\"" << escape_json(herald_severity)
            << "\",\"confidence_multiplier\":" << confidence_multiplier
            << ",\"explanation\":\"" << escape_json(explanation) << "\"}";
        return out.str();
    }
};

static BridgeRecord to_bridge(const AgentObservation& o) {
    BridgeRecord b;
    b.posture = posture_name(o.posture);
    b.gate_stance = gate_stance_name(gate_stance_for(o.posture));
    b.herald_severity = herald_severity_name(herald_severity_for(o.posture));
    b.explanation = o.explanation;
    // Attenuation: each recorded attenuation multiplies confidence by 0.7,
    // and Degraded posture never reaches full confidence. Unprotected /
    // Inconsistent / Failed collapse confidence to zero (fail-closed).
    switch (o.posture) {
    case Posture::Protected:
        b.confidence_multiplier = 1.0;
        break;
    case Posture::Degraded: {
        double m = 0.9;
        for (size_t i = 0; i < o.attenuations.size(); ++i) m *= 0.7;
        b.confidence_multiplier = m;
        break;
    }
    default:
        b.confidence_multiplier = 0.0; // fail-closed postures earn no trust
        break;
    }
    return b;
}

// ===========================================================================
// SECTION 15 — Observation intake helpers
// ===========================================================================
//
// Convenience constructors that assemble ServiceUnits and endpoints from the
// raw command outputs the CLI collects. These make the wiring layer terse and
// keep parsing in one reviewable place.

static ServiceUnit observe_unit_from_show(ServiceKind kind, Distro distro,
                                          std::string_view show_output) {
    const std::string unit = unit_name_for(kind, distro);
    const auto kv = parse_show_properties(show_output);
    return unit_from_properties(kind, unit, kv);
}

static ServiceUnit observe_unit_from_is_active(ServiceKind kind, Distro distro,
                                               std::string_view is_active_word) {
    ServiceUnit u;
    u.kind = kind;
    u.unit_name = unit_name_for(kind, distro);
    u.active = parse_active_state(is_active_word);
    u.observed = !trim(is_active_word).empty();
    // is-active does not report sub-state; infer Running when Active so a bare
    // is-active check can still yield confirmably_running().
    if (u.active == ActiveState::Active) u.sub = SubState::Running;
    return u;
}

// Build a ClamdEndpoint from a clamd.conf-derived socket path and an optional
// probe result. Absent a probe, liveness stays Unknown (O6).
static ClamdEndpoint make_endpoint(std::string local_socket, uint32_t tcp_port,
                                   SocketLiveness liveness) {
    ClamdEndpoint e;
    e.local_socket = std::move(local_socket);
    e.tcp_port = tcp_port;
    e.liveness = liveness;
    return e;
}

// ===========================================================================
// SECTION 16 — Invariant self-tests
// ===========================================================================

namespace selftest {

static void expect(bool condition, int& failures) {
    if (!condition) ++failures;
}

// Parsing is total and fail-closed: garbage never yields a running state.
static void test_parse_fail_closed(int& failures) {
    expect(parse_active_state("garbage") == ActiveState::Unknown, failures);
    ServiceUnit u = observe_unit_from_is_active(ServiceKind::Clamd,
                                                Distro::Generic, "garbage");
    expect(!u.confirmably_running(), failures);
}

// A confirmed active+running clamd is confirmably running.
static void test_confirmably_running(int& failures) {
    ServiceUnit u = observe_unit_from_show(
        ServiceKind::Clamd, Distro::Debian,
        "ActiveState=active\nSubState=running\nMainPID=42\n");
    expect(u.confirmably_running(), failures);
    expect(u.unit_name == "clamav-daemon.service", failures);
    expect(u.main_pid == 42, failures);
}

// O2: clamd down => Unprotected => fail-closed (zero confidence).
static void test_clamd_down_fails_closed(int& failures) {
    PostureEngine engine(Distro::Generic);
    ServiceUnit clamd; clamd.kind = ServiceKind::Clamd;
    clamd.unit_name = "clamd.service"; clamd.observed = true;
    clamd.active = ActiveState::Inactive; clamd.sub = SubState::Dead;
    ServiceUnit fresh; ServiceUnit onacc;
    const AgentObservation o = engine.derive(
        clamd, fresh, onacc, ClamdEndpoint{}, DatabaseState{});
    expect(o.posture == Posture::Unprotected, failures);
    const BridgeRecord b = to_bridge(o);
    expect(b.confidence_multiplier == 0.0, failures);
    expect(b.gate_stance == "hold_fail_closed", failures);
}

// O4: clamonacc up while clamd down => Inconsistent (quarantine-grade).
static void test_onacc_without_clamd_inconsistent(int& failures) {
    PostureEngine engine;
    ServiceUnit clamd; clamd.unit_name = "clamd.service"; clamd.observed = true;
    clamd.active = ActiveState::Inactive; clamd.sub = SubState::Dead;
    ServiceUnit onacc; onacc.kind = ServiceKind::Clamonacc;
    onacc.unit_name = "clamonacc.service"; onacc.observed = true;
    onacc.active = ActiveState::Active; onacc.sub = SubState::Running;
    const AgentObservation o = engine.derive(
        clamd, ServiceUnit{}, onacc, ClamdEndpoint{}, DatabaseState{});
    expect(o.posture == Posture::Inconsistent, failures);
    expect(gate_stance_for(o.posture) == GateStance::QuarantineGrade, failures);
}

// O3: running clamd + answering socket + stale db => Degraded (attenuated).
static void test_stale_db_degrades(int& failures) {
    PostureEngine engine;
    ServiceUnit clamd; clamd.unit_name = "clamd.service"; clamd.observed = true;
    clamd.active = ActiveState::Active; clamd.sub = SubState::Running;
    ClamdEndpoint ep = make_endpoint("/run/clamav/clamd.sock", 0,
                                     SocketLiveness::Answering);
    DatabaseState db; db.present = true; db.max_age_ms = 1000;
    db.main_cvd_age_ms = 5000; // older than max
    const AgentObservation o = engine.derive(
        clamd, ServiceUnit{}, ServiceUnit{}, ep, db);
    expect(o.posture == Posture::Degraded, failures);
    const BridgeRecord b = to_bridge(o);
    expect(b.confidence_multiplier < 1.0, failures);
    expect(b.confidence_multiplier > 0.0, failures);
}

// Fully protected posture yields full confidence and may_proceed.
static void test_protected_posture(int& failures) {
    PostureEngine engine;
    ServiceUnit clamd; clamd.unit_name = "clamd.service"; clamd.observed = true;
    clamd.active = ActiveState::Active; clamd.sub = SubState::Running;
    ServiceUnit fresh; fresh.kind = ServiceKind::Freshclam;
    fresh.observed = true; fresh.active = ActiveState::Active;
    fresh.sub = SubState::Running; fresh.enabled = EnableState::Enabled;
    ClamdEndpoint ep = make_endpoint("/run/clamav/clamd.sock", 0,
                                     SocketLiveness::Answering);
    DatabaseState db; db.present = true; db.max_age_ms = 100000;
    db.main_cvd_age_ms = 10; db.daily_age_ms = 10;
    const AgentObservation o = engine.derive(clamd, fresh, ServiceUnit{}, ep,
                                             db);
    expect(o.posture == Posture::Protected, failures);
    const BridgeRecord b = to_bridge(o);
    expect(b.confidence_multiplier == 1.0, failures);
    expect(b.gate_stance == "may_proceed", failures);
}

// O5: a mutating command is withheld in dry-run mode.
static void test_dry_run_withholds_mutation(int& failures) {
    ExecutionPolicy policy; // DryRun by default
    policy.backend_present = true;
    SystemctlBackend backend;
    const PlannedCommand start = backend.plan(Action::Start,
                                              "clamd.service", "test");
    expect(!policy.may_execute(start), failures);
    // Observational commands are always allowed.
    const PlannedCommand show = backend.plan(Action::Show,
                                             "clamd.service", "test");
    expect(policy.may_execute(show), failures);
}

// O5: execute mode with present backend permits a start, but stop still needs
// its own guard.
static void test_execute_mode_guards(int& failures) {
    ExecutionPolicy policy;
    policy.mode = ExecutionMode::Execute;
    policy.backend_present = true;
    SystemctlBackend backend;
    expect(policy.may_execute(backend.plan(Action::Start, "u", "")), failures);
    // Stop is withheld unless allow_stop is set.
    expect(!policy.may_execute(backend.plan(Action::Stop, "u", "")), failures);
    policy.allow_stop = true;
    expect(policy.may_execute(backend.plan(Action::Stop, "u", "")), failures);
}

// The supervisor runs restorative proposals but refuses to weaken clamd.
static void test_supervisor_never_weakens_clamd(int& failures) {
    // Executor that records what it was asked to run.
    int runs = 0;
    CommandExecutor exec = [&](const PlannedCommand&) {
        ++runs;
        return std::make_pair(0, "ok");
    };
    ExecutionPolicy policy;
    policy.mode = ExecutionMode::Execute;
    policy.backend_present = true;
    Supervisor sup(policy, exec);

    // Build an Unprotected observation whose proposal is a clamd start.
    PostureEngine engine;
    ServiceUnit clamd; clamd.unit_name = "clamd.service"; clamd.observed = true;
    clamd.active = ActiveState::Inactive; clamd.sub = SubState::Dead;
    AgentObservation o = engine.derive(clamd, ServiceUnit{}, ServiceUnit{},
                                       ClamdEndpoint{}, DatabaseState{});
    // Inject a hostile proposal to prove the supervisor refuses it.
    SystemctlBackend backend;
    o.proposals.push_back(backend.plan(Action::Stop, "clamd.service",
                                       "hostile"));
    const SupervisionResult r = sup.supervise(o);
    // The start proposal ran; the stop proposal was refused (never ran).
    bool saw_refused_stop = false;
    for (const auto& a : r.actions) {
        if (a.command.action == Action::Stop) {
            expect(!a.ran, failures);
            expect(!a.permitted, failures);
            saw_refused_stop = true;
        }
    }
    expect(saw_refused_stop, failures);
}

// Freshclam Checks parsing and max-age derivation.
static void test_freshclam_checks(int& failures) {
    const auto checks = freshclam_checks_from_conf(
        "# comment\nDatabaseDirectory /var/lib/clamav\nChecks 24\n");
    expect(checks.has_value() && *checks == 24, failures);
    const uint64_t max_age = max_age_from_checks_per_day(24);
    // 24 checks/day => interval 1h => max age 2h.
    expect(max_age == 2ULL * 60ULL * 60ULL * 1000ULL, failures);
    // Zero checks => no staleness gate.
    expect(max_age_from_checks_per_day(0) == 0, failures);
}

// A failed clamd unit yields the Failed posture with a restart proposal.
static void test_failed_unit(int& failures) {
    PostureEngine engine;
    ServiceUnit clamd; clamd.unit_name = "clamd.service"; clamd.observed = true;
    clamd.active = ActiveState::Failed; clamd.sub = SubState::FailedSub;
    const AgentObservation o = engine.derive(clamd, ServiceUnit{},
                                             ServiceUnit{}, ClamdEndpoint{},
                                             DatabaseState{});
    expect(o.posture == Posture::Failed, failures);
    expect(!o.proposals.empty(), failures);
    expect(o.proposals.front().action == Action::Restart, failures);
}

} // namespace selftest

inline int run_self_tests() {
    int failures = 0;
    selftest::test_parse_fail_closed(failures);
    selftest::test_confirmably_running(failures);
    selftest::test_clamd_down_fails_closed(failures);
    selftest::test_onacc_without_clamd_inconsistent(failures);
    selftest::test_stale_db_degrades(failures);
    selftest::test_protected_posture(failures);
    selftest::test_dry_run_withholds_mutation(failures);
    selftest::test_execute_mode_guards(failures);
    selftest::test_supervisor_never_weakens_clamd(failures);
    selftest::test_freshclam_checks(failures);
    selftest::test_failed_unit(failures);
    return failures;
}

} // namespace legal_clam_agent

#endif // MEARVK_CLAMAV_AGENT_HPP
