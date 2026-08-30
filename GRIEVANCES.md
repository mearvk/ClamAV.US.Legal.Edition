# GRIEVANCES — ClamAV.US.Legal.Edition

**Max Rupplin - MEARVK LLC 2026.**

## Scope

This document records design and engineering grievances identified in the repository's analysis. “Grievance” means a question, limitation, tension, or proposed improvement in the design; it does not mean that ClamAV is defective, negligent, unlawful, or that an upstream author is responsible for a particular consequence.

No upstream author attribution is altered by this document.

## 1. Verdict without sufficient provenance

A conventional antivirus result can be easier to consume than to explain. A user may receive a detection while having limited visibility into the exact object representation, parent container, transformation, signature/heuristic path, or configuration that produced it.

**Grievance:** consequential results should have an inspectable provenance record whenever practical.

**Proposed remedy:** attach object identity, parent/child relationship, transformation, evidence class, procedure, configuration, and qualification to the result.

## 2. “Not observed” versus “safe”

A scanner can encounter encrypted, unsupported, truncated, malformed, oversized, or otherwise incompletely observable content.

**Grievance:** downstream systems can mistakenly collapse incomplete observability into a clean result.

**Proposed remedy:** preserve an explicit qualification state for content that could not be completely examined.

## 3. Recursive depth and resource limits

Recursive scanning necessarily operates under resource limits. Limits are essential for availability and denial-of-service resistance, but they also constrain the observable search space.

**Grievance:** the existence of a scan limit is itself relevant evidence about what was actually examined.

**Proposed remedy:** record the applicable limits and whether a limit materially affected the scan path.

## 4. Representation dependence

The same underlying software relationship can appear as a file, archive member, executable section, document object, decompressed stream, or generated temporary object.

**Grievance:** representation boundaries can obscure causal continuity.

**Proposed remedy:** preserve a stable object/layer identity across supported transformations.

## 5. Knowledge-state dependence

Detection depends upon available signatures, parsers, heuristics, bytecode, YARA rules, hash databases, configuration, and other engine knowledge.

**Grievance:** a verdict can be read as an intrinsic property of the file when it is actually a result of an object-plus-knowledge procedure.

**Proposed remedy:** record the relevant database/version/configuration identity with consequential detections.

## 6. Time dependence

Software changes, definitions update, parsers improve, and configuration changes. Therefore the same artifact may be evaluated differently at different times.

**Grievance:** a timeless label can hide an important temporal condition.

**Proposed remedy:** associate significant evaluations with engine, database, configuration, and observation time where available.

## 7. Heuristic precedence

Heuristics can influence whether scanning continues or terminates on a finding.

**Grievance:** precedence can affect the evidentiary path and therefore deserves to be visible in an explanatory result.

**Proposed remedy:** record whether a heuristic finding terminated the relevant scan path or whether scanning continued.

## 8. Attribution is not causation

An artifact can be created, copied, transformed, signed, packaged, transported, or modified by different parties.

**Grievance:** provenance can be confused with authorship, and detection can be confused with attribution.

**Proposed remedy:** maintain separate fields for technical provenance, authorship claims, ownership claims, and detection evidence.

## 9. Update continuity

An installed security system is not a static program. Engine updates, signature updates, configuration changes, operating-system changes, and library changes alter the environment in which future scans occur.

**Grievance:** treating installation as the end of the software object's history loses an important part of operational continuity.

**Proposed remedy:** maintain an update/event chain sufficient to reconstruct the relevant scan environment.

## 10. Retro-dependency visibility

Recursive objects naturally produce relationships of the form:

`current object <- transformation <- parent object <- acquisition/update event`

**Grievance:** ordinary scan output may not expose this backward relationship even though it can be useful for incident reconstruction.

**Proposed remedy:** provide an optional provenance graph or machine-readable ledger.

## 11. Procedural co-dependence

Danger analysis is not simply a property lookup. It is a procedure operating on an object under particular capabilities and constraints.

**Grievance:** a binary “clean/dangerous” interface can hide important procedural conditions.

**Proposed remedy:** expose a qualified result model such as:

`observed evidence + procedure + confidence/qualification + verdict`

rather than relying exclusively on an unqualified boolean.

## 12. Historical continuity

The repository contains a long-lived software lineage, but current source should not automatically be treated as evidence that every present implementation detail existed unchanged throughout the entire historical period.

**Grievance:** historical claims can become stronger than the available source evidence.

**Proposed remedy:** distinguish historical documentation, version-specific source evidence, and present architecture in research documents.

## 13. Complexity as an explanatory cost

A mature antivirus engine necessarily contains many parsers, formats, recursive paths, heuristics, signatures, and safety controls.

**Grievance:** greater detection capability can produce greater difficulty in explaining a particular result.

**Proposed remedy:** build an explanation layer that summarizes the actual path taken without weakening the underlying detection engine.

## 14. Proposed stronger principle

The principal grievance can be reduced to one engineering proposition:

> **A consequential security verdict should preserve enough causal and procedural context to explain what was observed, under what conditions, and what remained unobservable.**

This is a proposed design principle, not a statement that upstream ClamAV fails to provide any such information.

## 15. Priority order

| Priority | Area | Desired improvement |
|---|---|---|
| 1 | Provenance | Stable object and parent/child identity |
| 2 | Qualification | Explicit incomplete-observation states |
| 3 | Procedure | Record material scan configuration and limits |
| 4 | Knowledge | Record engine/database identity |
| 5 | Temporal state | Preserve update and observation context |
| 6 | Explanation | Produce a readable causal scan narrative |
| 7 | Attribution | Separate provenance from authorship and responsibility |

## Closing

These grievances are intended to make the software-analysis model more formidable by making its assumptions explicit. They do not replace signatures, heuristics, parsers, recursion controls, or other established antivirus mechanisms. They propose an additional explanatory and provenance layer around them.

**Max Rupplin - MEARVK LLC 2026.**
