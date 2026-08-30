/*
 * System-centric admission gate for ClamAV.US.Legal.Edition.
 *
 * Original implementation.  It follows the scanner discipline we want from
 * the ClamAV reference: explicit terminal states, bounded work, separation of
 * technical evidence from policy, and fail-closed handling of scanner errors.
 * No human identity, reputation, demographic attribute, or affiliation is a
 * malware indicator and none enters the gate decision path.
 */
#include <algorithm>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace legal_clam_gate {

enum class Verdict { Allow, Observe, Quarantine, Reject, Error };
enum class Reason {
    None, MissingIdentity, IdentityMismatch, Signature, Heuristic,
    ResourceLimit, ScannerError, PathBoundary, Incomplete, Policy
};

enum class EvidenceKind {
    Identity, Provenance, Signature, Heuristic, Resource, Permission,
    Transform, Scanner
};

enum class Confidence { None, Low, Medium, High, Confirmed };

struct Evidence {
    EvidenceKind kind{EvidenceKind::Scanner};
    Confidence confidence{Confidence::None};
    std::string source;
    std::string detail;
};

struct Observation {
    std::string object_id;
    std::string canonical_path;
    uint64_t size_bytes{0};
    uint64_t scanned_bytes{0};
    uint64_t scanned_files{0};
    uint32_t recursion_depth{0};
    uint32_t archive_depth{0};
    uint64_t elapsed_ms{0};
    bool scan_completed{false};
    bool signature_hit{false};
    bool heuristic_hit{false};
    bool scanner_error{false};
    bool permission_denied{false};
    bool transform_failed{false};
    bool path_rejected{false};
    std::vector<Evidence> evidence;
};

struct Policy {
    uint64_t max_bytes{1024ULL * 1024ULL * 1024ULL};
    uint64_t max_files{100000};
    uint32_t max_recursion{32};
    uint32_t max_archive_depth{16};
    uint64_t max_elapsed_ms{300000};
    bool fail_closed_on_scanner_error{true};
    bool require_completion{true};
    bool quarantine_signature_hits{true};
    bool reject_path_escape{true};
    bool reject_permission_denied{false};
    bool quarantine_transform_failure{true};
    uint32_t minimum_high_confidence_kinds{1};
};

struct Decision {
    Verdict verdict{Verdict::Error};
    Reason reason{Reason::Policy};
    Confidence confidence{Confidence::None};
    std::string object_id;
    std::string explanation;
    std::vector<Evidence> supporting;
};

static bool safe_relative_path(std::string_view path) {
    if (path.empty() || path.front() == '/') return false;
    size_t start = 0;
    while (start < path.size()) {
        size_t end = path.find('/', start);
        if (end == std::string_view::npos) end = path.size();
        const auto component = path.substr(start, end - start);
        if (component.empty() || component == "..") return false;
        start = end + 1;
    }
    return true;
}

static bool resource_budget_ok(const Observation& o, const Policy& p) {
    return o.size_bytes <= p.max_bytes &&
           o.scanned_bytes <= p.max_bytes &&
           o.scanned_files <= p.max_files &&
           o.recursion_depth <= p.max_recursion &&
           o.archive_depth <= p.max_archive_depth &&
           o.elapsed_ms <= p.max_elapsed_ms;
}

static size_t high_confidence_kinds(const Observation& o) {
    bool seen[8] = {};
    size_t count = 0;
    for (const auto& e : o.evidence) {
        if (e.confidence < Confidence::High) continue;
        const auto index = static_cast<size_t>(e.kind);
        if (index < 8 && !seen[index]) {
            seen[index] = true;
            ++count;
        }
    }
    return count;
}

static Decision make_decision(Verdict v, Reason r, Confidence c,
                              const Observation& o, std::string text) {
    Decision d;
    d.verdict = v;
    d.reason = r;
    d.confidence = c;
    d.object_id = o.object_id;
    d.explanation = std::move(text);
    d.supporting = o.evidence;
    return d;
}

Decision evaluate(const Observation& o, const Policy& p) {
    if (o.object_id.empty())
        return make_decision(Verdict::Error, Reason::MissingIdentity,
                             Confidence::Low, o,
                             "technical object identity is required");

    if (p.reject_path_escape &&
        (o.path_rejected || !safe_relative_path(o.canonical_path)))
        return make_decision(Verdict::Reject, Reason::PathBoundary,
                             Confidence::High, o,
                             "canonical path crossed the configured boundary");

    if (!resource_budget_ok(o, p))
        return make_decision(Verdict::Quarantine, Reason::ResourceLimit,
                             Confidence::High, o,
                             "scan resource budget was exceeded");

    if (o.permission_denied && p.reject_permission_denied)
        return make_decision(Verdict::Quarantine, Reason::Policy,
                             Confidence::High, o,
                             "permission boundary prevented inspection");

    if (o.transform_failed && p.quarantine_transform_failure)
        return make_decision(Verdict::Quarantine, Reason::Policy,
                             Confidence::High, o,
                             "required transformation failed");

    if (o.scanner_error)
        return p.fail_closed_on_scanner_error
            ? make_decision(Verdict::Error, Reason::ScannerError,
                            Confidence::Low, o,
                            "scanner execution failed; safety is unknown")
            : make_decision(Verdict::Observe, Reason::ScannerError,
                            Confidence::Low, o,
                            "scanner execution failed; result is uncertain");

    if (p.require_completion && !o.scan_completed)
        return make_decision(Verdict::Observe, Reason::Incomplete,
                             Confidence::Medium, o,
                             "scan did not reach a terminal observation");

    if (o.signature_hit)
        return p.quarantine_signature_hits
            ? make_decision(Verdict::Quarantine, Reason::Signature,
                            Confidence::Confirmed, o,
                            "signature evidence requires isolation")
            : make_decision(Verdict::Reject, Reason::Signature,
                            Confidence::Confirmed, o,
                            "signature evidence rejected by policy");

    if (o.heuristic_hit)
        return make_decision(Verdict::Observe, Reason::Heuristic,
                             Confidence::Medium, o,
                             "heuristic evidence is not treated as proof");

    if (p.minimum_high_confidence_kinds > 0 &&
        high_confidence_kinds(o) < p.minimum_high_confidence_kinds)
        return make_decision(Verdict::Observe, Reason::Incomplete,
                             Confidence::Medium, o,
                             "insufficient independent technical evidence");

    return make_decision(Verdict::Allow, Reason::None, Confidence::High, o,
                         "technical evidence satisfies the admission policy");
}

static const char* verdict_name(Verdict v) {
    switch (v) {
    case Verdict::Allow: return "allow";
    case Verdict::Observe: return "observe";
    case Verdict::Quarantine: return "quarantine";
    case Verdict::Reject: return "reject";
    case Verdict::Error: return "error";
    }
    return "unknown";
}

static const char* reason_name(Reason r) {
    switch (r) {
    case Reason::None: return "none";
    case Reason::MissingIdentity: return "missing_identity";
    case Reason::IdentityMismatch: return "identity_mismatch";
    case Reason::Signature: return "signature";
    case Reason::Heuristic: return "heuristic";
    case Reason::ResourceLimit: return "resource_limit";
    case Reason::ScannerError: return "scanner_error";
    case Reason::PathBoundary: return "path_boundary";
    case Reason::Incomplete: return "incomplete";
    case Reason::Policy: return "policy";
    }
    return "unknown";
}

static const char* confidence_name(Confidence c) {
    switch (c) {
    case Confidence::None: return "none";
    case Confidence::Low: return "low";
    case Confidence::Medium: return "medium";
    case Confidence::High: return "high";
    case Confidence::Confirmed: return "confirmed";
    }
    return "unknown";
}

std::string serialize(const Decision& d) {
    std::ostringstream out;
    out << "{\"object_id\":\"" << d.object_id
        << "\",\"verdict\":\"" << verdict_name(d.verdict)
        << "\",\"reason\":\"" << reason_name(d.reason)
        << "\",\"confidence\":\"" << confidence_name(d.confidence)
        << "\",\"explanation\":\"";
    for (const char c : d.explanation) {
        if (c == '\"' || c == '\\') out << '\\';
        out << c;
    }
    out << "\"}";
    return out.str();
}

/* Explicit firewall: operator or demographic assertions never enter policy. */
Observation system_only(Observation o) {
    std::vector<Evidence> technical;
    for (const auto& e : o.evidence) {
        if (e.kind != EvidenceKind::Permission || e.source != "operator")
            technical.push_back(e);
    }
    o.evidence = std::move(technical);
    return o;
}

Decision evaluate_system_only(const Observation& o, const Policy& p) {
    return evaluate(system_only(o), p);
}

struct AuditRecord {
    uint64_t sequence{0};
    Verdict verdict{Verdict::Error};
    Reason reason{Reason::Policy};
    std::string object_id;
    std::string explanation;
};

class AuditLedger {
public:
    Decision record(const Decision& d) {
        AuditRecord r;
        r.sequence = ++sequence_;
        r.verdict = d.verdict;
        r.reason = d.reason;
        r.object_id = d.object_id;
        r.explanation = d.explanation;
        records_.push_back(std::move(r));
        return d;
    }
    const std::vector<AuditRecord>& records() const { return records_; }
private:
    uint64_t sequence_{0};
    std::vector<AuditRecord> records_;
};

} // namespace legal_clam_gate
