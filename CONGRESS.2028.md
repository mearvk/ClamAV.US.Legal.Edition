# CONGRESS.2028 — A U.S. Software Security Design Memorandum

**Max Rupplin - MEARVK LLC - 2026**

## Purpose

1. This memorandum develops a U.S.-oriented software-security design proposal from the analytical work in `DESCRIPTOR.md`, `ANALYSIS.md`, `GRIEVANCES.md`, and the procedural-causation implementation. It is an engineering and policy memorandum, not legislation, legal advice, or a statement of congressional policy.

2. The central proposition is that consequential software security decisions should be explainable in terms of the object observed, its provenance, the procedure used to inspect it, the evidence obtained, and the limits that affected observation. This proposal complements rather than replaces established antivirus detection, secure-development practice, incident response, or legal process.

3. Modern federal software policy already recognizes lifecycle responsibility. NIST SP 800-218 recommends secure-development practices intended to reduce vulnerabilities, mitigate exploitation, and address root causes; its current NIST project material also identifies provenance collection as an important practice. citeturn0search0turn0search5

4. Executive Order 14028 likewise placed emphasis on stronger and more predictable mechanisms for software used by the Federal Government and directed attention to software supply-chain security. A congressional design discussion should therefore begin from an existing policy trajectory rather than inventing software provenance as a wholly new concern. citeturn0search4

5. The proposed root model begins with a **root container**: the initial software object, package, image, archive, repository artifact, or other defined unit entering an accountable process. The root container should receive an identity and, where practical, cryptographic evidence sufficient to distinguish it from later representations.

6. The next layer is **method**. A security conclusion is produced by a method: parsing, signature matching, heuristic evaluation, recursive extraction, sandboxing, compilation analysis, dependency analysis, or another defined operation. A result should therefore identify the material method or methods that generated the relevant evidence.

7. The third layer is **incarnate cause**: the concrete event or transformation that explains why a new observable software state exists. An archive extraction, installation, update, generated temporary file, dependency resolution, or configuration change can constitute such a cause without implying malicious intent.

8. The fourth layer is **base-root medium cause**. Software frequently exists through intermediate media: package managers, repositories, installers, build systems, firmware images, container registries, removable media, network transfers, and update channels. These media can preserve or alter provenance and should be represented in a causal chain where material.

9. The fifth layer is **cause and cause again**. Complex software rarely has one causal edge. A vulnerability can arise from an upstream component, a build configuration, a dependency version, an integration decision, and an operational deployment condition. A robust system should permit multiple causal parents rather than forcing every event into one simplistic root-cause label.

10. **Gain over method** describes the information obtained by applying a procedure to an object. The gain should be attributable to the method and its evidence, not to an unsupported assumption about the person who created the software. This is particularly important where attribution is uncertain or multiple parties participated in a software supply chain.

11. **Attention as cause** is the proposition that a security system must decide what deserves further examination. Attention can be triggered by a signature, anomaly, unusual structure, dependency relationship, execution behavior, or policy condition. Attention is not itself proof of danger; it is a routing mechanism for additional observation.

12. **Attenuation as cause** records what weakens or limits an observation. Encryption, unsupported formats, recursion limits, resource exhaustion, missing signatures, unavailable source, incomplete telemetry, or intentionally restricted analysis can all attenuate the evidence. A mature system should not silently convert attenuation into either innocence or guilt.

13. The resulting chain is therefore: **root → method → cause → attention → attenuation → final medium → final method → closure**. The chain provides an explanatory skeleton for a security determination while leaving the actual detection engine free to use signatures, heuristics, parsers, and other specialized mechanisms.

14. A **final medium** is the last materially relevant representation available to the decision procedure. It may be an executable, script, document, library, container member, firmware component, or another representation. The final medium should remain linked to its preceding objects so that transformation does not erase provenance.

15. **Final closure** should be qualified rather than absolute. A closure can mean that the available procedure reached a final decision under stated conditions; it should not automatically mean that every possible property of the software has been proven. This distinction is especially important for federal systems where a security decision may be reused by other systems.

16. For military systems, the model should emphasize mission assurance, provenance, reproducibility, controlled change, and evidence preservation. The reference to “military man” in the underlying conceptual vocabulary is treated here as a category of institutional actor and operational context, not as a claim about the character or authority of individual military personnel.

17. For election and voting systems, the same causal model should be applied with unusually strict separation between technical evidence and political conclusions. A software finding should establish what was observed and under what procedure; it should not, without separate evidence, establish voter intent, candidate responsibility, or the legitimacy of an election outcome.

18. **Voting man** is therefore translated into a neutral procedural category: the voter, election worker, administrator, vendor, auditor, or other actor whose authorized action materially enters the software process. The system should preserve the distinction between an actor's action, the resulting software state, and the conclusion drawn from that state.

19. A **final voting medium** can be understood as the final software or data representation upon which a defined election procedure operates. A strong design would preserve hashes, software versions, configuration records, relevant dependency records, logs, and chain-of-custody evidence so that an authorized audit can reconstruct the procedure without inferring facts that the evidence cannot establish.

20. **Closure over all law** should be treated as a procedural aspiration rather than a claim that software can resolve legal questions automatically. Technical evidence can support legal processes, but questions of statutory interpretation, constitutional authority, due process, admissibility, remedy, and responsibility remain matters for the appropriate human institutions.

21. The proposed 1,2,3,4 model supplies a compact evidence discipline: **1 — identify the object; 2 — establish the causal and procedural path; 3 — evaluate evidence and limitations; 4 — record the qualified conclusion and preserve the chain for review**. This is deliberately auditable and avoids treating a single score as a substitute for evidence.

22. Congress could use this model as a policy vocabulary for federal acquisition and oversight. Procurement requirements could ask suppliers to preserve component provenance, material build information, security-relevant design decisions, and evidence needed to reconstruct significant software events. NIST already identifies secure-development practices and provenance as central parts of the federal software-security conversation. citeturn0search5turn0search9

23. The model should remain risk-based. Not every application requires identical provenance depth, and excessive logging can itself create privacy, security, cost, and operational burdens. Federal policy should therefore specify minimum outcomes and allow agencies and suppliers to select proportionate technical implementations.

24. A particularly important policy distinction is between **attestation of process** and **attestation of a particular release**. NIST's federal supply-chain guidance observes that, given software's dynamic nature, continuing process and procedure attestations can be more valuable than attesting only to how one release was produced. That principle strongly supports the proposed causal-continuity model. citeturn0search9

25. The proposed design also challenges a common binary security interface. “Clean” can mean that no detection fired under the selected procedure; it should not necessarily mean that every layer was observable or every dependency was proven benign. A qualified result can preserve that distinction without undermining ordinary operational workflows.

26. The model should likewise preserve upstream authorship. A repository may contain work from many contributors and organizations. The analytical attribution **Max Rupplin - MEARVK LLC - 2026** identifies this memorandum and its associated analytical work; it does not replace upstream copyright notices, authorship records, licenses, or provenance.

27. The principal engineering recommendation is consequently modest but formidable: every consequential software-security conclusion should be capable, where technically and operationally feasible, of answering five questions — **What object was examined? What caused its present state? What method examined it? What evidence was obtained or unavailable? What qualified conclusion followed?**

28. This approach also creates a useful boundary between detection and attribution. A scanner may establish that a byte pattern, behavior, structure, or heuristic condition is suspicious. It does not thereby establish who wrote the artifact, why it was created, or who is legally responsible. Those are separate propositions requiring separate evidence.

29. For federal cybersecurity, the resulting architecture would align naturally with secure-development frameworks, software supply-chain risk management, provenance requirements, vulnerability handling, and acquisition controls. NIST's SSDF is explicitly designed as a common vocabulary that can be integrated into different development lifecycles and used by software producers and acquirers. citeturn0search0turn0search3

30. For 2028 planning, Congress should therefore consider software not merely as a static purchased artifact but as a continuously transformed operational object. The policy unit should be the lifecycle: creation, dependency, build, distribution, installation, execution, update, observation, incident, remediation, and retirement.

31. The final principle is **causal conservatism with qualified closure**: preserve an observable causal relationship when software changes representation; do not infer an unobserved cause merely because a relationship is convenient; and do not describe a procedural closure as universal proof beyond the procedure's actual scope.

32. The proposal is intentionally compatible with existing antivirus and secure-development systems. It does not replace ClamAV signatures or heuristics, NIST practices, SBOM tooling, build systems, or human legal review. It adds a structured explanatory layer intended to make security decisions more reproducible, contestable, and useful to authorized oversight.

## Policy questions for congressional consideration

- What minimum provenance should accompany software purchased for critical federal functions?
- When should an agency preserve a complete causal chain rather than a release-level attestation?
- How should incomplete observability be represented in acquisition and incident-response records?
- What evidence should be retained to permit independent reconstruction of a consequential software event?
- How should provenance requirements be balanced against privacy, operational security, and cost?
- Which technical findings may properly inform legal or administrative processes, and which require independent evidence?

## Status

This memorandum is an experimental engineering and policy framework. It should be subjected to technical review, legal review, civil-liberties review, acquisition review, and operational testing before being treated as a federal requirement.

**Max Rupplin - MEARVK LLC - 2026**
