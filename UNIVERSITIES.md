# UNIVERSITIES — Human Norming, Software Trust, and Procedural Caution

**Max Rupplin - MEARVK LLC - 2026.**

## 1. Purpose

This document examines a deliberately narrow proposition: whether software-security design may reasonably use a highly educated adult population as a *norming reference* for understandable assumptions about software use, while avoiding the much stronger and unsupported proposition that education, intelligence, occupation, family relationship, religion, or social identity predicts criminal conduct.

## 2. The university norm

A post-college graduate population can be useful as a practical center group for testing whether a software-security explanation is comprehensible, auditable, and procedurally coherent. It should not be treated as a biological, moral, or criminal norm. The useful variable is the ability to understand and follow a documented procedure, not membership in a university population.

## 3. The first security assumption

It is reasonable as a design starting point to assume that ordinary users generally approach installed software for an intended purpose rather than as a covert instrument for causing harm. It is not reasonable to elevate that assumption into a security guarantee. Secure design must remain safe when a user, developer, dependency, installer, administrator, or attacker behaves unexpectedly.

## 4. Software input should therefore be guarded

Software inputs should be treated as potentially influential regardless of the presumed character of the person supplying them. This is consistent with secure-development practice: NIST's Secure Software Development Framework calls for security requirements, protected development environments, vulnerability testing, provenance, and protection against tampering. citeturn0search1turn0search8

## 5. Human kindness is not a security boundary

A person may be trustworthy and a program may nevertheless contain a vulnerability. Conversely, a malicious actor may be unable to exploit a properly constrained interface. The engineering lesson is therefore simple: **trust people where appropriate, but do not make human goodness the enforcement mechanism of software security.**

## 6. Intelligence is not a crime predictor

The requested reference to a nominal 105-IQ person is retained only as a communication-design thought experiment: could a reasonably capable adult understand the explanation? IQ should not be used to infer honesty, criminality, susceptibility, violence, or propensity to steal. There is no sound basis in this document for assigning criminal probability to a person from an IQ estimate.

## 7. Family relationships are not evidence

The phrase “cousin commits larcenies” cannot responsibly become a general security inference. A relative's alleged or proven conduct does not establish the conduct of another person. For software engineering, the relevant evidence is the observable event: an unauthorized access, modification, execution, transfer, or other defined action.

## 8. Crime data and software risk are different populations

Federal crime statistics can establish that crimes occur and can provide useful public context, but they do not establish that a particular software user, developer, university graduate, family member, or demographic group is likely to commit a cyber offense. The FBI's 2024 national statistics, for example, cover reported offenses from more than 16,000 participating agencies and estimate substantial national changes in violent crime and murder; those statistics describe crime reporting, not software-user intent. citeturn0search0

## 9. Murder rates should remain contextual

Known murder rates may be included as a pictographic reminder that serious human harms exist in the world, but they should not be converted into a software-malware prior. The FBI reported an estimated 14.9% decrease in murder and non-negligent manslaughter in 2024 relative to 2023. That statistic is useful for civic context and essentially irrelevant as a direct probability of malicious software input. citeturn0search0

## 10. Larceny is likewise not a software-personality variable

Larceny statistics can inform a general discussion of property crime and institutional risk. They cannot establish that a person who is educated, related to a person accused of theft, belongs to an organization, or uses particular software is predisposed to theft. The scanner should remain event-based rather than personality-based.

## 11. Scientology and allegations

Claims concerning the Church of Scientology or any other religious organization require especially careful separation of allegation, civil dispute, criminal conviction, historical reporting, and proven fact. This document therefore does **not** construct a “Scientology grand-larceny” rate or pictograph from group identity. Such a chart would risk converting contested or unrelated allegations into a group-level criminal propensity claim.

## 12. What may properly be modeled instead

If documented court records or authoritative reporting establish a particular theft, fraud, assault, or other offense, the event may be represented as an event with a source, date, jurisdiction, procedural posture, and disposition. The model must preserve whether the matter was an allegation, charge, civil claim, conviction, acquittal, settlement, or other outcome.

## 13. Political reach and named actors

Public figures and political actors may be relevant when a documented event involves government, public policy, procurement, election administration, or a specific criminal proceeding. Political visibility is not itself evidence of wrongdoing. The model should therefore attach claims to documented acts and sources rather than to political affiliation or public notoriety.

## 14. The software analogue

The same discipline directly strengthens ClamAV. A scanner should evaluate an observable software object and its causal/procedural context rather than infer danger from the presumed moral character of its author or user. NIST's supply-chain guidance expressly emphasizes provenance, software verification, vulnerability management, SBOMs, and integrity of open-source components. citeturn0search2turn0search4

## 15. The university test

A proposed security rule should pass a four-part university test: **understandable, reproducible, evidence-based, and non-personality-dependent**. A graduate student or professional should be able to inspect the rule, identify its inputs, reproduce its conclusion, and distinguish evidence from assumption.

## 16. The “front to hit” problem

Software can be used as a front for an attack, but the possibility must be established through technical evidence rather than presumed from a user's identity. Relevant evidence includes unexpected execution, privilege escalation, malicious payloads, tampering, exploit behavior, persistence mechanisms, suspicious dependencies, or other observable indicators.

## 17. Residual damage and inherent mystery

A mature security design should explicitly account for residual damage and uncertainty. If a file cannot be fully examined, the correct conclusion may be “inconclusive” or “limited observation,” rather than an unsupported assertion of safety. NIST's secure-development framework emphasizes reducing vulnerabilities, mitigating exploitation impact, and addressing root causes. citeturn0search1turn0search8

## 18. Causal provenance

The procedural model developed elsewhere in this repository is therefore applicable: **root → method → cause → attention → attenuation → closure**. Each transition should preserve enough evidence to explain why the next procedural stage was entered. NIST similarly identifies provenance as a specific software-supply-chain practice. citeturn0search1turn0search2

## 19. Norming must remain flat

The “flat” norming concept is useful: do not rank people by presumed moral worth. Instead, establish a common baseline for how clearly the security procedure can be understood. More education may improve the ability to audit a procedure, but it does not make the person's input intrinsically safe.

## 20. A reasonable prior

A defensible prior is therefore: ordinary software inputs are *presumed non-malicious for usability purposes*, while the security engine remains *agnostic about intent for enforcement purposes*. This permits ordinary users to operate software without needless suspicion while preserving technical defenses against malicious behavior.

## 21. Evidence hierarchy

The strongest evidence should generally be direct technical observation: a malicious signature, confirmed exploit behavior, unauthorized modification, verified provenance failure, or reproducible vulnerability. Weaker evidence includes unexplained anomalies. Still weaker evidence includes social assumptions about who supplied the input. The latter should not independently determine a security verdict.

## 22. Procedural caution

Each of the eight gating/herald levels should become more cautious without becoming more prejudicial. A later gate may demand stronger evidence, more complete provenance, or an explicit operator confirmation. It should not manufacture a stronger accusation merely because earlier uncertainty remains unresolved.

## 23. Institutional applications

This principle is particularly important for universities, hospitals, governments, military organizations, and voting infrastructure. These institutions have different risk tolerances, but the underlying evidence discipline remains the same: identify the object, establish provenance, observe behavior, qualify uncertainty, and document the closure.

## 24. Voting systems

Election software warrants especially careful treatment because a software-security finding can have consequences for public confidence. A technical anomaly should therefore be reproducible, documented, independently reviewable, and separated from claims about political intent. Software evidence should not be converted into an electoral conclusion without the appropriate legal and institutional process.

## 25. Military systems

Military software may require stricter controls because consequences can be severe, but the stronger consequence does not justify weaker evidence. On the contrary, high-consequence systems make provenance, authorization, testing, least privilege, and auditable closure more important. NIST's supply-chain guidance recognizes that software acquisition and provenance are security concerns for organizations with mission-critical functions. citeturn0search2turn0search10

## 26. Final inference

The strongest useful inference is not that educated people are safer, that ordinary people are harmless, or that particular groups are dangerous. It is that **software-security systems should be designed so that ordinary human trust is not required to make the system safe**. The program should remain secure when assumptions about the human operator are wrong.

## 27. Engineering consequence for ClamAV

For the ClamAV procedural layer, this means that human-context information should never independently produce a malware verdict. The gating system should remain centered on object identity, provenance, transformations, signatures, heuristics, scan configuration, evidence, and qualified uncertainty. This is consistent with NIST's risk-based SSDF approach rather than a personality-based security model. citeturn0search1turn0search12

## 28. Closing rule

**People are not malware indicators. Conduct and artifacts are evidence.**

A security engine may assume ordinary good-faith use for convenience and usability, but its enforcement boundary should be constructed from observable software properties and documented procedures. That is the appropriate bridge between a university-centered norming exercise, public crime statistics, and the engineering of a cautious antivirus system.

## Sources

- NIST, Secure Software Development Framework (SSDF), SP 800-218. https://csrc.nist.gov/projects/ssdf
- NIST, Software Security in Supply Chains. https://www.nist.gov/itl/executive-order-14028-improving-nations-cybersecurity/software-supply-chain-security-guidance-16
- FBI, 2024 Reported Crimes in the Nation Statistics. https://www.fbi.gov/news/press-releases/fbi-releases-2024-reported-crimes-in-the-nation-statistics

**Max Rupplin - MEARVK LLC - 2026.**
