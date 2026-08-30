# The ClamAV Runtime Algorithm — Pronounced, Linkedly

**Max Rupplin - MEARVK LLC - 2026.**

*A source-grounded pronouncement of the scanning algorithm as it actually runs
on the operating system, traced through this repository's `libclamav/`,
`clamd/`, and the surrounding front-ends, and linked to the procedural gate,
herald, and system-agent layer this project adds around it.*

*Weighed, as requested, on five axes: whether each movement is **main** (on the
true execution path, not decoration), **appropriate** (doing the fitting thing
for its station), **safe** (fails toward caution), **conceded** (the kind of
claim a college course on systems or security would accept as accurate), and
**attention** (a matter a thinking person can find genuinely, and divisively,
interesting).*

---

## I. The engine is built before it is ever asked a question

Before a single byte of a suspected file is examined, ClamAV constructs a
*compiled engine*, and the order is not incidental. `cl_engine_new()`
(`libclamav/others.c:471`) allocates the `struct cl_engine`, seeds it with the
defaults from `libclamav/default.h`, and prepares the array of matcher roots.
`cl_load()` (`libclamav/readdb.c:5250`) then ingests signature databases — and
it *refuses* to load anything once the engine is already compiled, returning an
error rather than mutating a finalized state. `cl_engine_compile()`
(`libclamav/readdb.c:5937`) builds the Aho-Corasick tries, compiles the PCRE
regexes, fixes up the hash matchers, and only then sets `CL_DB_COMPILED`. This
is *main*: nothing scans until this ladder is climbed. It is *conceded* because
it is textbook — a recognizer must be constructed before it recognizes — and it
is *appropriate* because the immutability of a compiled engine is what lets many
threads share it safely.

## II. Compilation is a one-way door, and that door is a safety property

The detail that `cl_load()` will not accept new databases after compilation, and
that `cl_engine_free()` (`libclamav/readdb.c:5594`) tears the engine down only
when a reference count reaches zero under a mutex, is more than housekeeping. It
means the authority that decides "malicious or not" is a fixed artifact for the
duration of a scan campaign: its knowledge cannot shift underfoot mid-file. This
is *safe* in the precise sense our procedural layer cares about — it is exactly
the retro-dependency the `DESCRIPTOR.md` model names, where a changed verdict
can be attributed to a changed *engine state* rather than a changed *file*. It
is *attention*-worthy because it quietly refutes a naive intuition: an antivirus
is not a live oracle continuously reconsidering the world; it is a snapshot of
knowledge, deliberately frozen so that its answers are reproducible.

## III. Every scan is funneled through one narrow throat

However a file arrives — `cl_scanfile`, `cl_scandesc`, or `cl_scanmap`
(`libclamav/scanners.c`, ~6175–6611) — the public wrappers all pour into a
single private driver, `scan_common()` (`libclamav/scanners.c:5640`). This is
the architectural spine: rather than many parallel scanning idioms, there is one
place where the scan context is born and one place where the verdict is read
back. It is *main* by definition, and *appropriate* as engineering — a single
throat is a single place to enforce invariants. Our own gate mirrors this
consciously: the sixteen-step evaluator in `gating/gating.cpp` is likewise the
*one* narrow throat through which every admission decision passes, so that no
later, more permissive branch can be reached once a stop has fired.

## IV. The scan context carries a stack, because a file is a tree

`scan_common()` builds an on-stack `cli_ctx` and, crucially, allocates a
*recursion stack* sized to `engine->max_recursion_level`, seeding layer zero
with the top-level map. This encodes a deep truth about the problem: a file
presented to an antivirus is rarely a flat thing. It is a ZIP inside an email
inside a stream, a macro inside a document inside an archive. The algorithm is
therefore not a loop over bytes but a *walk over a tree of extracted objects*,
each pushed as a new layer. This is *conceded* systems knowledge — containers
recurse — and it is *attention*-worthy because the security-relevant object is
almost never the one the user handed over; it is something several
transformations deep, which is precisely why the naive "just scan the file"
mental model fails.

## V. The per-layer core types first, then decides how to look

`cli_magic_scan()` (`libclamav/scanners.c:4504`) is the heart that beats once
per layer. Its order is deliberate and worth pronouncing exactly: it checks
limits, then *determines the file type* from magic bytes rather than trusting
any extension, records the type onto the current recursion layer, consults the
clean-file cache, and only then dispatches to the format-specific parser. Type
identification precedes interpretation. This is *appropriate* — you cannot
correctly unpack what you have misidentified — and it is *safe*, because a file
lying about what it is (an executable wearing a `.txt` name) is defeated at this
step. It is *attention*-dividing in a subtle way: it locates trust in the
*content's own structure*, not in any human-supplied label, which is the same
firewall our layer enforces when it refuses to let operator assertions enter the
decision path.

## VI. Limits are checked before work, and the check is fail-toward-skip

Before a layer does anything expensive, `cli_updatelimits()` and beneath it
`cli_checklimits()` (`libclamav/others.c:1215`) test the scan against a battery
of bounds — total scan size (`CLI_DEFAULT_MAXSCANSIZE`, 400 MB), per-file size
(100 MB), file count (`CLI_DEFAULT_MAXFILES`, 10000), and a wall-clock time
limit — in a fixed order, the time limit first because an unbounded scan is the
most dangerous failure of all. These constants are not arbitrary; they are the
codified admission that a scanner which never finishes protects nothing. This is
profoundly *safe* and *main*: it is on every scan's hot path. Our gate borrows
the very same discipline — its `resource_budget_ok()` quarantines an object
whose scan exceeded its budget, on the stated principle that *an unbounded scan
is an unreliable scan*, which is a direct paraphrase of what this code enforces.

## VII. Recursion is bounded at the moment of descent, not in hope

The push onto the recursion stack, `cli_recursion_stack_push()`
(`libclamav/others.c:1763`), performs the limit check *first* and then, if the
stack is already at `recursion_stack_size - 1`, returns `CL_EMAXREC` and
declines to descend. The bound is enforced at the exact instant of descent, not
audited afterward. This matters because the archive bomb — the innocuously small
file that expands into ruinous depth — is a real adversary, and the only correct
defense is to refuse the next layer, not to notice the damage once done. It is
*safe*, *appropriate*, and *conceded* (bounded recursion is first-week material
in any serious systems course), and it is *attention*-worthy because it is a
concrete place where a security engine and a compiler-theory abstraction — the
depth-limited traversal — turn out to be the same thing.

## VIII. Matching is three engines in a fixed order behind a prefilter

Within a layer, `cli_scan_fmap()` (`libclamav/matcher.c:1065`) slides a window
across the bytes, and for each window `matcher_run()` (`libclamav/matcher.c:99`)
consults, *in order*: a bloom-like prefilter that narrows where a match could
even begin, then Boyer-Moore, then Aho-Corasick (the primary multi-pattern
trie), then byte-comparison and PCRE. The ordering is an economy: the cheap
filter rejects most positions so the expensive automata run rarely. This is
*main* — it is literally where detection happens — and it is *conceded* as a
faithful application of classical string-matching theory. Its *attention* lies
in the honesty of the engineering: the famous algorithms (Aho-Corasick,
Boyer-Moore) are not chosen for elegance but sequenced for *cost*, the fast
approximate gate guarding the slow exact ones — the same shape, again, as a gate
in front of a herald.

## IX. The sliding window overlaps on purpose

A small, easily-overlooked line governs correctness: after each window the
offset advances by `bytes - maxpatlen`, deliberately re-reading the last
`maxpatlen` bytes. Without this overlap, a signature that happened to straddle a
buffer boundary would be silently missed. It is a one-line defense against a
whole class of false negatives. This is *appropriate* and *safe*, and it earns
*attention* out of proportion to its size, because it is the kind of detail that
separates code that *appears* to work on every test file from code that is
actually correct on the adversarial input designed to land on the seam. A
thinking person should find it sobering that the difference between secure and
insecure can be a single subtracted term.

## X. Hashes, logical signatures, and bytecode escalate the question

Pattern matching is only the first register. Over the same pass, `cli_scan_fmap`
finalizes cryptographic hashes and checks them against the hash matchers
(`cli_hm_scan`), then `cli_exp_eval` / `cli_lsig_eval` (`libclamav/matcher.c`,
~940–1031) evaluate *logical signatures* — boolean combinations of sub-patterns
with conditions — and, where a logical signature carries a bytecode index,
`cli_bytecode_runlsig()` executes a small sandboxed program that can inspect the
sample directly. The algorithm thus climbs a ladder of expressive power: exact
hash, then combinational logic, then Turing-capable bytecode. This is *main* for
modern detection and *attention*-rich, because it is where a "signature scanner"
quietly becomes a *programmable* analysis engine — and where our
`PROCEDURAL_GATING.md` warning about running untrusted bytecode earns its
keep, since that same power is a liability if the databases are not trusted.

## XI. A finding becomes evidence, not merely a boolean

When something matches, `cli_append_virus()` (`libclamav/others.c:1730`) does not
flip a global flag; it *classifies* the finding by name prefix — `Heuristics.`
and `PUA.` become potentially-unwanted indicators, `Weak.` becomes a weak
indicator, and everything else a strong one — and appends it to the layer's
evidence. This modern "evidence" model is the single most important structural
sympathy between upstream ClamAV and the layer this project adds. It is
*appropriate* and *conceded*: mature detectors distinguish a confirmed signature
from a mere heuristic suspicion. Our herald and gate speak the very same
dialect — `Confidence`, `EvidenceKind`, attenuation — precisely because the
engine underneath already refuses to collapse "suspicious" and "confirmed" into
one bit.

## XII. Strong evidence aborts; weak evidence is merely remembered

The break logic is exact and deserves pronouncing. In all-match mode the scan
*never* breaks — it keeps going to enumerate every hit. Otherwise, a strong
indicator sets the status to `CL_VIRUS` *and* sets `ctx->abort_scan = true`, a
belt-and-suspenders guarantee that the scan halts even if the return code were
lost in deep recursion; potentially-unwanted and weak indicators leave the
status clean and are reported later. This is *safe* in the strongest sense: the
most serious finding is made the hardest to accidentally discard. It is
*attention*-dividing because it inverts a lazy assumption — the system is not
built to *trust* that a `CL_VIRUS` will propagate correctly through the call
stack; it is built to *distrust* propagation and force the stop redundantly.
That distrust of one's own plumbing is the mark of software written by people
who have been burned.

## XIII. Verdicts roll upward, and a clean layer is cached, not assumed

As each layer finishes, its evidence is condensed into a per-layer verdict, and
`cli_recursion_stack_pop()` (`libclamav/others.c:2135`) merges a child's
indicators into its parent, so a threat found three containers deep surfaces at
the top. A layer that is genuinely clean is *added to the clean cache* so
identical content is not re-scanned. The distinction between *observed-clean*
(scanned, nothing found, cache it) and *unknown* (not scanned) is maintained
throughout. This is *main*, *appropriate*, and *conceded*, and it is the exact
principle our safety rules elevate to a commandment: *absence of a finding is
never proof of safety*. ClamAV caches what it *observed* to be clean; it does
not cache the unscanned as safe, and neither may anything built around it.

## XIV. The daemon is the same algorithm wearing a socket

At runtime on a server, `clamd` accepts a request — `parse_command()` and
`command()` in `clamd/session.c` decode `SCAN`, `INSTREAM`, `MULTISCAN`,
`ALLMATCHSCAN` and their kin — and dispatches to `scan_callback` / `scanfd` in
`clamd/scanner.c`, which call the very same `cl_scandesc_callback` /
`cl_scanfile_callback` and therefore the very same `scan_common`. The reply is
the terse, classic protocol: `<file>: <name> FOUND`, `<file>: OK`, or
`<file>: <error> ERROR`. This is *conceded* and *main*: the daemon adds
concurrency, streaming, and a wire protocol, but the *judgment* is identical to
the command-line scanner's. It is *attention*-worthy because it is a clean
lesson in separating *transport* from *decision* — the same lesson our system
agent takes to heart when it observes `clamd` via `systemctl` yet never presumes
to make ClamAV's ruling itself.

## XV. On-access scanning closes the loop, and reveals where our agent stands

`clamonacc` arms the Linux kernel's fanotify interface, and on each file-access
event it acts as a *client* of `clamd`, shipping the file to the daemon and
permitting or denying access based on the `FOUND`/`OK` answer. Here the whole
chain is visible at once: kernel event → on-access broker → daemon → the shared
`scan_common` core → verdict → allow-or-block. This is exactly the seam our
`agent/` layer addresses. The system agent watches whether `clamd` is
confirmably running, whether its socket answers, and whether the signature
database is fresh — and if the daemon is down while `clamonacc` believes itself
active, it declares an *inconsistent, quarantine-grade* posture, because
on-access protection without a live backend is protection in name only. That
condition is not hypothetical; it is a direct reading of this very
client-server dependency, and refusing to paper over it is the *safe* choice.

## XVI. The pronouncement, and why a thinking person should care

Pronounced whole, the algorithm is this: *build a frozen recognizer from trusted
databases; funnel every file through one throat; walk it as a bounded tree of
typed, size-limited layers; at each layer run a cost-ordered cascade of
matchers, hashes, logical rules, and sandboxed bytecode; convert findings into
graded evidence; let strong evidence abort and clean layers cache; roll the
verdict up the tree; and deliver it identically whether by command line, socket,
or kernel event.* Every movement in that sentence is *main* — I have cited the
function and line for each — and every one is *safe* by construction, failing
toward "skip, quarantine, or declare unknown" rather than toward a false clean.
It is *appropriate* throughout, each component doing the fitting thing for its
station, and it is *conceded*: nothing here would draw an objection from a
serious course in operating systems or security, because it is simply what the
code does. What earns *attention* — what should divide and hold a thinking mind —
is the consistent moral posture of the design: it distrusts labels, distrusts
its own propagation, bounds its own appetite, and never confuses silence for
safety. The procedural gate, herald, and system agent this repository adds do
not improve upon that posture; they *transcribe* it into an explicit, reviewable
vocabulary and extend it from the scanned object outward to the services that do
the scanning — so that the same caution ClamAV exercises over a file, the system
now exercises over ClamAV itself. The upstream engine remains the authority; our
layer is its faithful, and deliberately more talkative, conscience.

---

*Cross-references: `DESCRIPTOR.md` (retro-dependency and continuity model),
`PROCEDURAL_GATING.md` (the six safety rules), `PROCEDURAL_CAUSATION.hss`
(ROOT → METHOD → CAUSE → ATTENTION → ATTENUATION → CLOSURE), `AGENT.md`
(the service co-concern layer). Source anchors: `libclamav/scanners.c`,
`libclamav/matcher.c`, `libclamav/others.c`, `libclamav/readdb.c`,
`libclamav/default.h`, `clamd/session.c`, `clamd/scanner.c`, `clamonacc/`.*

**Max Rupplin - MEARVK LLC - 2026.**
