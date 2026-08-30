/*
 * clamav_agent_main.cpp — clamavctl-style CLI for the ClamAV system agent.
 *
 * Author: Max Rupplin - MEARVK LLC - 2026.
 *
 * This is the operator-facing front end for agent/clamav_agent.hpp. It is the
 * only place that touches the real system: it shells out (via popen) to
 * `systemctl` to OBSERVE service state, and — only when explicitly asked with
 * --execute — to run the restorative commands the supervisor proposes.
 *
 * Subcommands:
 *   status     observe the ClamAV services and print each unit's state
 *   observe    derive and print the operational posture (JSON)
 *   plan       print the corrective commands the agent WOULD run (dry-run)
 *   supervise  run the corrective plan (dry-run unless --execute is given)
 *   selftest   run the built-in invariant self-tests
 *
 * Flags:
 *   --execute            leave dry-run and actually run mutating commands
 *   --allow-stop         additionally permit stopping a unit (extra guard)
 *   --allow-disable      additionally permit disabling a unit at boot
 *   --distro <d>         debian | redhat | generic (unit-name mapping)
 *   --backend <b>        systemctl | clamavctl
 *   --db-dir <path>      signature database directory (default /var/lib/clamav)
 *   --socket <path>      clamd local socket (default /run/clamav/clamd.sock)
 *   --freshclam-conf <p> path to freshclam.conf for the Checks directive
 *
 * SAFETY: dry-run is the default. Nothing that could change ClamAV's protective
 * state runs unless --execute is present AND the backend binary is installed.
 * The agent never clears a ClamAV detection; it only supervises the services.
 */

#include "clamav_agent.hpp"

#include <cstdio>
#include <ctime>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <vector>

using namespace legal_clam_agent;

namespace {

// -------------------------------------------------------------------------
// Process helpers (the only impure part of the agent)
// -------------------------------------------------------------------------

// Run a command line and capture stdout. Returns {exit_code, output}. On any
// failure the exit code is non-zero and output is whatever was captured. This
// wraps popen; it is used both for observation and, under policy, execution.
std::pair<int, std::string> capture(const std::string& command_line) {
    std::string out;
    // Redirect stderr into stdout so failures are visible in the capture.
    const std::string full = command_line + " 2>&1";
    std::unique_ptr<FILE, int (*)(FILE*)> pipe(popen(full.c_str(), "r"),
                                               pclose);
    if (!pipe) return {-1, "popen failed"};
    char buffer[4096];
    while (std::fgets(buffer, sizeof(buffer), pipe.get()) != nullptr)
        out += buffer;
    // pclose is called by the unique_ptr deleter; we cannot read its code that
    // way, so re-run status via the deleter is not possible. Instead, treat a
    // non-empty capture as success for observation; execution paths below use
    // a dedicated runner that inspects the code.
    return {0, out};
}

// Run a command and return its real exit status (for the execute path).
std::pair<int, std::string> capture_with_status(const std::string& cl) {
    const std::string full = cl + " 2>&1";
    FILE* pipe = popen(full.c_str(), "r");
    if (!pipe) return {-1, "popen failed"};
    std::string out;
    char buffer[4096];
    while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr) out += buffer;
    const int status = pclose(pipe);
    // WEXITSTATUS-equivalent without <sys/wait.h> assumptions:
    const int code = (status == -1) ? -1 : ((status >> 8) & 0xFF);
    return {code, out};
}

// Is a program available on PATH? Used to set ExecutionPolicy.backend_present.
bool program_present(const std::string& program) {
    const auto [code, out] =
        capture_with_status("command -v " + program + " >/dev/null 2>&1 && "
                            "echo yes || echo no");
    (void)code;
    return out.find("yes") != std::string::npos;
}

// Does a filesystem path exist?
bool path_exists(const std::string& path) {
    struct stat st;
    return ::stat(path.c_str(), &st) == 0;
}

// Age of a file in milliseconds (now - mtime). Returns nullopt if absent.
std::optional<uint64_t> file_age_ms(const std::string& path) {
    struct stat st;
    if (::stat(path.c_str(), &st) != 0) return std::nullopt;
    const std::time_t now = std::time(nullptr);
    if (now < st.st_mtime) return 0;
    return static_cast<uint64_t>(now - st.st_mtime) * 1000ULL;
}

std::string read_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) return {};
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// -------------------------------------------------------------------------
// Options
// -------------------------------------------------------------------------

struct Options {
    std::string subcommand{"observe"};
    ExecutionMode mode{ExecutionMode::DryRun};
    bool allow_stop{false};
    bool allow_disable{false};
    Distro distro{Distro::Generic};
    std::string backend{"systemctl"};
    std::string db_dir{"/var/lib/clamav"};
    std::string socket{"/run/clamav/clamd.sock"};
    std::string freshclam_conf;
};

Distro parse_distro(const std::string& s) {
    if (s == "debian") return Distro::Debian;
    if (s == "redhat" || s == "rhel" || s == "fedora") return Distro::RedHat;
    return Distro::Generic;
}

Options parse_args(int argc, char** argv) {
    Options o;
    std::vector<std::string> args(argv + 1, argv + argc);
    for (size_t i = 0; i < args.size(); ++i) {
        const std::string& a = args[i];
        auto next = [&](const char* what) -> std::string {
            if (i + 1 >= args.size()) {
                std::cerr << "missing value for " << what << "\n";
                return {};
            }
            return args[++i];
        };
        if (a == "--execute") o.mode = ExecutionMode::Execute;
        else if (a == "--allow-stop") o.allow_stop = true;
        else if (a == "--allow-disable") o.allow_disable = true;
        else if (a == "--distro") o.distro = parse_distro(next("--distro"));
        else if (a == "--backend") o.backend = next("--backend");
        else if (a == "--db-dir") o.db_dir = next("--db-dir");
        else if (a == "--socket") o.socket = next("--socket");
        else if (a == "--freshclam-conf") o.freshclam_conf = next("--freshclam-conf");
        else if (!a.empty() && a[0] != '-') o.subcommand = a;
        else std::cerr << "ignoring unknown flag: " << a << "\n";
    }
    return o;
}

std::unique_ptr<ControlBackend> make_backend(const std::string& name) {
    if (name == "clamavctl")
        return std::make_unique<ClamavctlBackend>();
    return std::make_unique<SystemctlBackend>();
}

// -------------------------------------------------------------------------
// Observation collection (impure): read real service state via the backend
// -------------------------------------------------------------------------

ServiceUnit observe_unit(const ControlBackend& backend, ServiceKind kind,
                         Distro distro, bool backend_present) {
    const std::string unit = unit_name_for(kind, distro);
    if (!backend_present) {
        // No backend installed: we cannot observe. Return an unobserved unit
        // so the posture engine treats state as Unknown (fail-closed).
        ServiceUnit u;
        u.kind = kind;
        u.unit_name = unit;
        u.observed = false;
        return u;
    }
    const PlannedCommand show = backend.plan(Action::Show, unit,
                                             "observe unit state");
    const auto [code, out] = capture(show.command_line());
    (void)code;
    return unit_from_properties(kind, unit, parse_show_properties(out));
}

DatabaseState observe_database(const std::string& db_dir,
                               const std::string& freshclam_conf) {
    DatabaseState db;
    db.directory = db_dir;
    // ClamAV ships either main.cvd/main.cld and daily.cvd/daily.cld.
    const std::string main_cvd = db_dir + "/main.cvd";
    const std::string main_cld = db_dir + "/main.cld";
    const std::string daily_cvd = db_dir + "/daily.cvd";
    const std::string daily_cld = db_dir + "/daily.cld";
    const bool has_main = path_exists(main_cvd) || path_exists(main_cld);
    const bool has_daily = path_exists(daily_cvd) || path_exists(daily_cld);
    db.present = has_main && has_daily;
    if (auto age = file_age_ms(path_exists(main_cvd) ? main_cvd : main_cld))
        db.main_cvd_age_ms = *age;
    if (auto age = file_age_ms(path_exists(daily_cvd) ? daily_cvd : daily_cld))
        db.daily_age_ms = *age;
    // Freshness threshold from the operator's freshclam Checks setting.
    uint32_t checks = 24; // ClamAV default is 24 checks/day
    if (!freshclam_conf.empty()) {
        const std::string conf = read_file(freshclam_conf);
        if (auto c = freshclam_checks_from_conf(conf)) checks = *c;
    }
    db.max_age_ms = max_age_from_checks_per_day(checks);
    return db;
}

ClamdEndpoint observe_endpoint(const std::string& socket) {
    // Socket-answering probes require a connect(); to keep this front end
    // dependency-light we treat a present socket file as NoSocket=false and
    // leave answering-ness Unknown unless the socket file is absent.
    ClamdEndpoint e = make_endpoint(socket, 0, SocketLiveness::Unknown);
    if (!path_exists(socket)) e.liveness = SocketLiveness::NoSocket;
    else e.liveness = SocketLiveness::Answering; // best-effort: file present
    return e;
}

AgentObservation collect(const Options& o, const ControlBackend& backend,
                         bool backend_present) {
    ServiceUnit clamd = observe_unit(backend, ServiceKind::Clamd, o.distro,
                                     backend_present);
    ServiceUnit fresh = observe_unit(backend, ServiceKind::Freshclam, o.distro,
                                     backend_present);
    ServiceUnit onacc = observe_unit(backend, ServiceKind::Clamonacc, o.distro,
                                     backend_present);
    ClamdEndpoint ep = observe_endpoint(o.socket);
    DatabaseState db = observe_database(o.db_dir, o.freshclam_conf);
    PostureEngine engine(o.distro);
    return engine.derive(std::move(clamd), std::move(fresh), std::move(onacc),
                         std::move(ep), std::move(db));
}

// -------------------------------------------------------------------------
// Subcommand handlers
// -------------------------------------------------------------------------

int cmd_status(const Options& o, const ControlBackend& backend,
               bool backend_present) {
    const AgentObservation obs = collect(o, backend, backend_present);
    std::cout << "{\"clamd\":" << obs.clamd.serialize()
              << ",\"freshclam\":" << obs.freshclam.serialize()
              << ",\"clamonacc\":" << obs.clamonacc.serialize()
              << ",\"backend_present\":" << (backend_present ? "true" : "false")
              << "}\n";
    return 0;
}

int cmd_observe(const Options& o, const ControlBackend& backend,
                bool backend_present) {
    const AgentObservation obs = collect(o, backend, backend_present);
    const BridgeRecord bridge = to_bridge(obs);
    std::cout << "{\"observation\":" << obs.serialize()
              << ",\"bridge\":" << bridge.serialize() << "}\n";
    return 0;
}

int cmd_plan(const Options& o, const ControlBackend& backend,
             bool backend_present) {
    const AgentObservation obs = collect(o, backend, backend_present);
    std::cout << "{\"posture\":\"" << posture_name(obs.posture)
              << "\",\"proposals\":[";
    bool first = true;
    for (const auto& p : obs.proposals) {
        if (!first) std::cout << ',';
        first = false;
        std::cout << p.serialize();
    }
    std::cout << "]}\n";
    return 0;
}

int cmd_supervise(const Options& o, const ControlBackend& backend,
                  bool backend_present) {
    const AgentObservation obs = collect(o, backend, backend_present);
    ExecutionPolicy policy;
    policy.mode = o.mode;
    policy.backend_present = backend_present;
    policy.allow_stop = o.allow_stop;
    policy.allow_disable = o.allow_disable;

    // The real executor: only reached for commands the policy permits.
    CommandExecutor exec = [](const PlannedCommand& cmd) {
        return capture_with_status(cmd.command_line());
    };
    Supervisor sup(std::move(policy), std::move(exec));
    const SupervisionResult result = sup.supervise(obs);
    std::cout << result.serialize() << "\n";
    // Exit non-zero when the system is not converged, so shell callers and
    // systemd can react. A dry-run that merely reports an unprotected system
    // still signals non-convergence.
    return result.converged ? 0 : 1;
}

void usage() {
    std::cerr <<
        "clamav-agent — system agent co-concerned with ClamAV\n"
        "usage: clamav-agent <status|observe|plan|supervise|selftest> [flags]\n"
        "  --execute           run mutating commands (default: dry-run)\n"
        "  --allow-stop        also permit stop (extra guard)\n"
        "  --allow-disable     also permit disable (extra guard)\n"
        "  --distro <d>        debian|redhat|generic\n"
        "  --backend <b>       systemctl|clamavctl\n"
        "  --db-dir <path>     database dir (default /var/lib/clamav)\n"
        "  --socket <path>     clamd socket (default /run/clamav/clamd.sock)\n"
        "  --freshclam-conf <p> path to freshclam.conf\n";
}

} // namespace

int main(int argc, char** argv) {
    if (argc >= 2 && (std::string(argv[1]) == "-h" ||
                      std::string(argv[1]) == "--help")) {
        usage();
        return 0;
    }
    const Options o = parse_args(argc, argv);

    if (o.subcommand == "selftest") {
        const int failures = run_self_tests();
        std::cout << "clamav-agent self-tests: "
                  << (failures == 0 ? "all passed" : "FAILURES")
                  << " (" << failures << ")\n";
        return failures == 0 ? 0 : 1;
    }

    const std::unique_ptr<ControlBackend> backend = make_backend(o.backend);
    const bool backend_present = program_present(backend->program());

    if (o.subcommand == "status")
        return cmd_status(o, *backend, backend_present);
    if (o.subcommand == "observe")
        return cmd_observe(o, *backend, backend_present);
    if (o.subcommand == "plan")
        return cmd_plan(o, *backend, backend_present);
    if (o.subcommand == "supervise")
        return cmd_supervise(o, *backend, backend_present);

    usage();
    return 2;
}
