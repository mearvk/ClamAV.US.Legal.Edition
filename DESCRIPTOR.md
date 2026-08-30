# ClamAV — Procedural and Retro-Dependency Descriptor

**Max Rupplin - MEARVK LLC 2026.**

## 1. Purpose

This document describes a conceptual procedural-law model for examining the continued state of software after installation. It distinguishes the actual ClamAV scanning engine from an additional analytical layer concerned with provenance, dependency history, subsequent modification, and the conditions under which software continues to operate.

The phrase **iterative process** is used here in the procedural sense: an installed software object remains subject to continuing states, events, inputs, updates, dependencies, and obligations. It is not a definition of Man or a metaphysical statement about human nature.

## 2. Actual ClamAV scanning model

ClamAV is an antivirus engine built around `libclamav`, with command-line and daemon interfaces. `clamscan` creates an engine, loads its signature database, scans supplied files or directories, reports results, and exits. `clamd` provides a persistent multi-threaded scanning service. On Linux, ClamOnAcc can provide on-access scanning through ClamD. citeturn0search1turn0search4

Detection is principally driven by signature databases. ClamAV supports hash signatures, body-based signatures, logical signatures, container metadata signatures, YARA rules, and bytecode signatures. Its signed CVD/CLD databases provide the normal trusted signature distribution mechanism. citeturn0search3

Bytecode signatures extend the scanner with more complex matching routines. They can inspect sample data and metadata during processing and can produce a detection when their conditions are satisfied. citeturn0search0

## 3. Procedural continuity

For this project, an installed software object is treated as having a continuing procedural state:

```text
creation → installation → initialization → execution → update → modification
       → dependency change → subsequent execution → retirement/removal
```

The important proposition is **continuity of the object through successive states**. Installation does not terminate the history of the software. It begins a new procedural phase in which the executable, configuration, signature databases, libraries, services, sockets, update mechanisms, and surrounding operating-system state may change.

This is the intended meaning of the legal/procedural idea that an object or obligation **continues**. The model records what happened to the object rather than treating its original creation as the end of its identity.

## 4. Retro-dependency analysis

A **retro-dependency** is an analytical relation that points backward from a present software state toward an earlier condition required to explain that state.

Examples include:

| Present observation | Retro-dependency question |
|---|---|
| Executable loads a library | Which library/version supplied the required interface? |
| Scanner loads a signature database | Which update process supplied the database? |
| Bytecode executes | Which signed database supplied the bytecode? |
| Service listens on a socket | Which configuration and process state established the listener? |
| File changes after installation | Which package, updater, administrator, or process produced the change? |
| Detection behavior changes | Which signature, engine, configuration, or dependency changed? |

This is not a claim that ClamAV currently performs a general-purpose historical dependency audit. It is a proposed **analysis layer around ClamAV observations**.

## 5. Channels of observation

The model treats software channels as observable pathways through which state can continue or change:

1. **Artifact channel** — executable, library, archive, script, configuration, and data files.
2. **Dependency channel** — shared libraries, runtime components, package relationships, and external resources.
3. **Update channel** — signature databases, package updates, configuration changes, and other authorized update operations.
4. **Execution channel** — processes, services, sockets, file access, and runtime behavior.
5. **Provenance channel** — source, package origin, signer, version, timestamp, and change history.

ClamAV already exposes several relevant technical observations through its scanning and signature systems; the retro-dependency layer organizes those observations into a continuing procedural record. ClamAV's signed databases and bytecode-security controls are especially important to this provenance model. citeturn0search3turn0search7

## 6. State-of-nature terminology

Here, **state of nature** is used only as a software-state metaphor: the condition of an object before the next procedural intervention is applied.

For an installed program, that state may be represented as:

```text
S(t) = {artifact, dependencies, configuration, signatures,
        privileges, execution state, provenance, observations}
```

An event transforms the state:

```text
S(t+1) = F(S(t), event, authority, environment)
```

The useful legal/procedural question is therefore not "what is Man?" but:

> **What continuing condition of the software object is established by the prior procedure, and what event lawfully or technically changes that condition?**

## 7. Disposition, frequency, and intervention

Software normally exists as a disposition toward execution rather than as continuous execution itself. A file can remain installed and available while a process is inactive. When an execution event occurs, the software enters a new runtime state.

The model therefore records frequency as an observational property:

- installation frequency — how often the object is installed or replaced;
- update frequency — how often its inputs or signatures change;
- execution frequency — how often its executable state is invoked;
- scan frequency — how often material is presented for inspection;
- dependency frequency — how often dependency state changes.

The **hand of man** is treated here as an attribution category for an observable intervention—such as installation, configuration, authorization, replacement, or removal—not as a philosophical definition of humanity.

## 8. Legal/procedural continuity test

The proposed test has five stages:

### A. Identify

Establish the software object, version, path, hash, package identity, and relevant provenance.

### B. Establish the prior state

Record the dependencies, configuration, signatures, privileges, and execution conditions that existed immediately before the event.

### C. Identify the intervening event

Record an installation, update, scan, execution, dependency change, configuration change, or removal event.

### D. Establish the resulting state

Record what changed and what remained continuous.

### E. Determine procedural consequence

Determine whether the event created, continued, modified, suspended, or terminated the relevant procedural condition.

This test is descriptive. It does not itself determine a legal right, duty, liability, or jurisdiction.

## 9. Relationship to ClamAV

The conceptual layer should not be confused with ClamAV's detection engine. ClamAV scans content and applies detection mechanisms; this descriptor proposes recording the **history and conditions surrounding that content and the scanner state**.

For example, if a signature database changes and the scanner subsequently produces a different verdict, the procedural record can preserve:

```text
previous engine state
        ↓
signature/update event
        ↓
new engine state
        ↓
same sample rescanned
        ↓
changed observation
```

That sequence permits a reviewer to distinguish a change in the scanned artifact from a change in the scanner's knowledge or execution environment.

## 10. Security boundary

Retro-dependency analysis must not weaken ClamAV's existing trust boundaries. In particular, ClamAV documentation warns against running unsigned bytecode from untrusted sources because it can result in arbitrary code execution. The analytical layer should therefore observe and record trust state rather than bypass it. citeturn0search6turn0search7

## 11. Status

This document defines a **conceptual and procedural analysis model**. It does not claim that the upstream ClamAV engine implements every retro-dependency or procedural-law operation described here.

The distinction is deliberate:

- **ClamAV fact:** scanning, signatures, bytecode, databases, extraction, daemon operation, and on-access scanning are implemented capabilities. citeturn0search2turn0search3
- **Project model:** continuity, retro-dependencies, procedural state, intervention attribution, and historical state comparison are analytical constructs proposed by this repository.

---

**Max Rupplin - MEARVK LLC 2026.**
