# UNIVERSITIES — Human Norming, Software Trust, and Procedural Caution

**Max Rupplin - MEARVK LLC - 2026.**

## 1. Purpose

This document examines whether software-security design may use a highly educated adult population as a norming reference for understandable assumptions about software use, while avoiding the unsupported proposition that education, intelligence, family relationship, religion, or social identity predicts criminal conduct.

## 2. The university norm

A post-college graduate population can be useful as a practical center group for testing whether a security explanation is comprehensible, auditable, and procedurally coherent. It should not be treated as a biological, moral, or criminal norm. The useful variable is the ability to understand and follow a documented procedure.

## 3. The first security assumption

It is reasonable as a design starting point to assume that ordinary users generally approach installed software for an intended purpose rather than as a covert instrument for causing harm. It is not reasonable to elevate that assumption into a security guarantee. Secure design must remain safe when a user, developer, dependency, installer, administrator, or attacker behaves unexpectedly.

## 4. Software input should therefore be guarded

Software inputs should be treated as potentially influential regardless of the presumed character of the person supplying them. NIST's Secure Software Development Framework calls for security requirements, protected development environments, vulnerability testing, provenance, and protection against tampering.

## 5. Human kindness is not a security boundary

A person may be trustworthy and a program may nevertheless contain a vulnerability. Conversely, a malicious actor may be unable to exploit a properly constrained interface. Trust people where appropriate, but do not make human goodness the enforcement mechanism of software security.

## 6. Intelligence is not a crime predictor

A nominal 105-IQ person can be used only as a communication-design thought experiment: could a reasonably capable adult understand the explanation? IQ should not be used to infer honesty, criminality, susceptibility, violence, or propensity to steal.

## 7. Family relationships are not evidence

A relative's alleged or proven conduct does not establish the conduct of another person. For software engineering, the relevant evidence is the observable event: unauthorized access, modification, execution, transfer, or another defined action.

## 8. Crime data and software risk are different populations

Federal crime statistics can establish that crimes occur and provide public context, but they do not establish that a particular software user, developer, university graduate, family member, or demographic group is likely to commit a cyber offense. Crime statistics should therefore remain contextual rather than becoming a software-user prior.

## 9. Murder rates should remain contextual

Known murder rates may be included as a pictographic reminder that serious human harms exist in the world, but they should not be converted into a software-malware prior. A violent-crime rate is not evidence about the security disposition of a software input.

## 10. Larceny is likewise not a software-personality variable

Larceny statistics can inform a general discussion of property crime and institutional risk. They cannot establish that a person who is educated, related to a person accused of theft, belongs to an organization, or uses particular software is predisposed to theft. The scanner should remain event-based rather than personality-based.

## 11. Scientology: the relevant historical record

There is, however, a documented historical record concerning specific Scientology officials and the organization's Guardian's Office that is relevant as an **institutional case study of provenance, authorization, and misuse of information systems**. It must not be converted into a claim that Scientologists as a population are criminal.

In the 1970s, Operation Snow White involved efforts by Scientology operatives to obtain government records and information. A National Archives-hosted Clinton Presidential Library record summarizes that an FBI raid in July 1977 uncovered evidence of a conspiracy involving infiltration, burglary, and electronic surveillance of IRS and Justice Department offices; it states that eleven Scientologists, including Mary Sue Hubbard, ultimately went to prison. citeturn0search38

A federal appellate decision in *In re Search Warrant Dated July 4, 1977* describes seized Snow White materials and records referring to obtaining non-FOIA government information through covert means, including burglary or theft. The court's discussion is useful because it concerns evidence and search procedure rather than group-level moral characterization. citeturn0search8

Contemporary historical reporting likewise describes the Snow White operation as expanding from an effort to remove allegedly false records into a criminal conspiracy involving infiltration and burglary, with eleven defendants receiving prison sentences. citeturn0search4

The documented criminal conduct included theft and burglary of government material, but the available authoritative record does **not** justify a simple statistical category called “Scientology grand larceny.” The historically documented offenses should instead be recorded by defendant, act, jurisdiction, charge, conviction or plea, and disposition. That is a materially stronger data model.

## 12. What may properly be modeled instead

A transparent institutional case-study record can use fields such as:

| Field | Example treatment |
|---|---|
| Organization | Church of Scientology / relevant organizational unit |
| Operation | Operation Snow White |
| Period | 1970s |
| Conduct | infiltration, burglary, theft of government documents, related obstruction |
| Evidence | seized records, investigative records, court findings |
| Defendants | identified individuals, not the religious population as a whole |
| Disposition | plea/conviction/sentence as established by the record |
| Relevance | information-security, provenance, authorization, and procedural-control case study |

This preserves the root facts without manufacturing a population crime rate.

## 13. Why the distinction matters for software

The Snow White history is especially relevant to software design because it demonstrates a real-world failure mode in which access to information, institutional credentials, documents, and governmental processes was allegedly or demonstrably used beyond authorized purposes, with criminal convictions following. The lesson for software is not that a religious group is dangerous; it is that **authorization and provenance cannot be inferred from identity alone**.

## 14. Political reach and named actors

Public figures and political actors may be relevant when a documented event involves government, public policy, procurement, election administration, or a specific criminal proceeding. Political visibility is not itself evidence of wrongdoing. Claims should attach to documented acts and sources rather than political affiliation or public notoriety.

## 15. The software analogue

The same discipline strengthens ClamAV. A scanner should evaluate an observable software object and its causal/procedural context rather than infer danger from the presumed moral character of its author or user. Provenance, software verification, vulnerability management, and integrity of open-source components are appropriate technical controls.

## 16. The university test

A proposed security rule should pass a four-part university test: **understandable, reproducible, evidence-based, and non-personality-dependent**. A graduate student or professional should be able to inspect the rule, identify its inputs, reproduce its conclusion, and distinguish evidence from assumption.

## 17. The “front to hit” problem

Software can be used as a front for an attack, but the possibility must be established through technical evidence rather than presumed from a user's identity. Relevant evidence includes unexpected execution, privilege escalation, malicious payloads, tampering, exploit behavior, persistence mechanisms, suspicious dependencies, or other observable indicators.

## 18. Residual damage and inherent mystery

A mature security design should explicitly account for residual damage and uncertainty. If a file cannot be fully examined, the correct conclusion may be “inconclusive” or “limited observation,” rather than an unsupported assertion of safety.

## 19. Causal provenance

The procedural model developed elsewhere in this repository is applicable: **root → method → cause → attention → attenuation → closure**. Each transition should preserve enough evidence to explain why the next procedural stage was entered.

## 20. Norming must remain flat

The “flat” norming concept is useful: do not rank people by presumed moral worth. Establish a common baseline for how clearly the security procedure can be understood. More education may improve the ability to audit a procedure, but it does not make a person's input intrinsically safe.

## 21. A reasonable prior

A defensible prior is therefore: ordinary software inputs are presumed non-malicious for usability purposes, while the security engine remains agnostic about intent for enforcement purposes. This permits ordinary users to operate software without needless suspicion while preserving technical defenses against malicious behavior.

## 22. Evidence hierarchy

The strongest evidence should generally be direct technical observation: a malicious signature, confirmed exploit behavior, unauthorized modification, verified provenance failure, or reproducible vulnerability. Weaker evidence includes unexplained anomalies. Social assumptions about who supplied the input should not independently determine a security verdict.

## 23. Procedural caution

Each of the eight gating/herald levels should become more cautious without becoming more prejudicial. A later gate may demand stronger evidence, more complete provenance, or explicit operator confirmation. It should not manufacture a stronger accusation merely because earlier uncertainty remains unresolved.

## 24. Institutional applications

This principle is particularly important for universities, hospitals, governments, military organizations, and voting infrastructure. These institutions have different risk tolerances, but the underlying evidence discipline remains the same: identify the object, establish provenance, observe behavior, qualify uncertainty, and document the closure.

## 25. Voting systems

Election software warrants especially careful treatment because a software-security finding can have consequences for public confidence. A technical anomaly should therefore be reproducible, documented, independently reviewable, and separated from claims about political intent.

## 26. Military systems

Military software may require stricter controls because consequences can be severe, but the stronger consequence does not justify weaker evidence. High-consequence systems make provenance, authorization, testing, least privilege, and auditable closure more important.

## 27. Final inference

The strongest useful inference is not that educated people are safer, that ordinary people are harmless, or that particular groups are dangerous. It is that **software-security systems should be designed so that ordinary human trust is not required to make the system safe**. The program should remain secure when assumptions about the human operator are wrong.

## 28. Engineering consequence for ClamAV

For the ClamAV procedural layer, human-context information should never independently produce a malware verdict. The gating system should remain centered on object identity, provenance, transformations, signatures, heuristics, scan configuration, evidence, and qualified uncertainty.

## 29. Closing rule

**People are not malware indicators. Conduct and artifacts are evidence.**

The Scientology case study therefore contributes a useful design lesson only at the level of documented conduct: unauthorized access, impersonation, burglary, theft of records, and misuse of institutional processes are observable categories that security systems can reason about. Religious affiliation is not.

## Sources

- U.S. National Archives, Clinton Presidential Library record concerning the Church of Scientology and Operation Snow White. citeturn0search38
- *In re Search Warrant Dated July 4, 1977*, 667 F.2d 117 (D.C. Cir. 1981). citeturn0search8
- Los Angeles Times historical reporting on Operation Snow White and the resulting convictions. citeturn0search4
- FBI, national crime statistics and methodology.
- NIST, Secure Software Development Framework (SSDF), SP 800-218.

**Max Rupplin - MEARVK LLC - 2026.**