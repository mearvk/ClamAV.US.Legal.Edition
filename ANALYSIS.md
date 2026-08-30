# ANALYSIS — A More Formidable Model of ClamAV's Design Philosophy

**Max Rupplin - MEARVK LLC 2026.**

## Purpose

This document challenges, rather than merely restates, the design philosophy visible in ClamAV's source. The objective is to identify the assumptions that make a mature antivirus engine formidable, identify the assumptions that remain contingent, and propose a stronger conceptual attachment between an observed software object, its causes, its transformations, and the procedure by which danger is determined.

This is a software-analysis and systems-law model. It is not a legal opinion and does not claim that ClamAV itself implements the proposed legal concepts.

## 1. The formidable assumptions already present

ClamAV does not treat a file as a single undifferentiated byte sequence. Its current scan-layer model records type, size, file map, recursion levels, object identity, metadata, evidence, verdict, temporary-directory state, and a parent layer. That is a strong architectural assumption: **the object under examination has structure and history inside the scanning procedure.**

The scanner also treats archives and other containers procedurally. It can inspect metadata, apply scan limits, extract an object, and submit that resulting object to another scan. Consequently, a present detection may depend upon the route by which the object became observable.

The architecture further assumes that knowledge is plural. Signatures, file-type recognition, parsers, heuristics, hashes, trust decisions, limits, and callbacks can all affect the result. A scan is therefore better represented as a controlled procedure than as a single predicate `dangerous(file)`.

## 2. The assumption that should be challenged

A conventional antivirus abstraction is tempted toward:

`file -> scan -> verdict`

A more exact model is:

`(object, ancestry, representation, procedure, knowledge, configuration, time) -> evidence -> verdict`

The distinction matters. Two procedures can inspect materially related representations and produce different evidentiary states because they had different limits, parsers, signatures, trust settings, or recursion paths.

This does **not** mean that a detection is arbitrary. It means that the meaning of a detection should include the conditions under which it was obtained.

## 3. Primacy of the software object

The proposed stronger philosophy gives the software object primacy over the scanner's first representation of it.

An object may be:

1. received as bytes;
2. identified as a container or executable;
3. decompressed or decoded;
4. separated into child objects;
5. normalized into another representation;
6. subjected to signatures or heuristics;
7. updated or transformed; and
8. observed again.

The scanner should therefore preserve enough provenance to answer: **what was this object, what caused the current representation, and what procedural operations made the evidence observable?**

## 4. Liquidacy: software as a changing object

Here “liquidacy” is a proposed engineering term for the fact that software can remain materially continuous while changing representation or state.

Examples include:

- an archive yielding an executable;
- an installer yielding a library;
- a document containing an embedded script;
- an update replacing a previously trusted component;
- a generated temporary file becoming the next scan object;
- a signature database changing the knowledge available to the same engine.

The stronger model should distinguish **identity continuity** from **byte continuity**. A file does not have to remain byte-for-byte identical for its causal identity or dependency history to remain relevant.

## 5. Cause attachment

For every significant verdict, a formidable provenance record should attempt to attach five causes:

| Cause | Question |
|---|---|
| Origin | Where did the observed object enter the procedure? |
| Transformation | What operation changed its representation? |
| Dependency | Which parent, child, library, database, or external condition mattered? |
| Evidence | What concrete observation produced the finding? |
| Procedure | Which enabled rule, parser, heuristic, or limit determined the path? |

This creates a **cause-attached verdict** rather than a naked verdict.

## 6. Procedural co-dependence

Danger determination is co-dependent on both the object and the search procedure.

That should be stated precisely. It does not mean that the scanner invents danger. Rather, the scanner determines what evidence is available to the decision procedure under its configured search space.

For example, if an encrypted archive cannot be inspected, the correct result is not necessarily “clean.” It may be “not fully observable,” “encrypted content encountered,” or another explicitly qualified state. The distinction between absence of evidence and evidence of absence is central to a strong scanner.

## 7. Retro-dependency

The existing recursive architecture gives a foundation for a formal retro-dependency record:

`current object <- transformation <- parent object <- acquisition/update event`

A retro-dependency is not an accusation about the object's author. It is a backward traversal of observable software relationships.

The proposed record should contain:

- object ID;
- parent object ID;
- transformation type;
- originating representation;
- resulting representation;
- timestamp where available;
- scanner configuration relevant to the transition;
- evidence attached to the layer;
- final and intermediate verdicts.

## 8. A stronger “law” of scanning

The proposed engineering principle is:

> **No consequential software verdict should be interpreted independently of the observable causal and procedural conditions that produced it.**

This can be operationalized as six stages:

**Identify → Preserve → Trace → Observe → Evaluate → Qualify**

- **Identify:** assign stable identity to the object and its layer.
- **Preserve:** retain relevant source and representation information.
- **Trace:** follow parent/child and transformation relationships.
- **Observe:** collect signatures, metadata, parser results, and heuristics.
- **Evaluate:** apply the configured decision procedure.
- **Qualify:** state what was and was not observable.

## 9. “Strongish ape-law” translated into engineering terms

The informal phrase “strongish ape-law” is interpreted here as a demand for a more formidable, plain, durable rule rather than as a scientific or legal doctrine.

The engineering equivalent is **causal conservatism**:

> Do not discard a causal relationship merely because the object has changed representation.

An executable extracted from an archive remains related to that archive. A generated temporary representation remains related to the operation that generated it. An updated signature database changes the knowledge state under which a later observation occurs.

The scanner should preserve these relationships without automatically treating relationship as guilt.

## 10. Authorship and responsibility

Source authorship should remain distinct from object provenance and detection evidence.

The source tree contains upstream copyright and author notices. A stronger analysis should preserve those notices rather than replacing them with an undifferentiated project attribution. **Max Rupplin - MEARVK LLC 2026.** identifies this analytical work; it does not rewrite upstream authorship or ownership.

Likewise, a detected malicious artifact should not be attributed to a programmer merely because the artifact passed through code written by that programmer. Attribution requires its own evidence.

## 11. Proposed extension to ClamAV-style architecture

A future experimental layer could add a provenance ledger beside the existing scan-layer stack:

```text
Object
  |
  +-- Parent / Origin
  |
  +-- Transformation
  |
  +-- Representation
  |
  +-- Evidence
  |
  +-- Procedure / Configuration
  |
  +-- Verdict
  |
  +-- Qualification
```

This should be additive. Existing detection semantics should remain understandable and stable while the provenance layer supplies stronger explanatory context.

## 12. Limits of the model

The proposed framework cannot infer an object's complete real-world history from bytes alone. Provenance may be missing, timestamps may be unavailable or unreliable, transformations may occur outside the scanner, and a scanner's inability to inspect content is not proof of safety or maliciousness.

The model therefore favors explicit uncertainty over invented certainty.

## Conclusion

ClamAV's mature architecture is already formidable because it recognizes recursive objects, multiple representations, signatures, parsers, heuristics, evidence, trust, and procedural limits. The stronger design proposed here does not discard that architecture. It makes one additional commitment: **attach consequential observations to their observable causes and procedural conditions.**

That gives software analysis a more durable basis for understanding not only *what was detected*, but *what object was observed, how it arrived there, what transformed it, what could be examined, and why the procedure reached its stated result*.
