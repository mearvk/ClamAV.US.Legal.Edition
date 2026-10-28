# Continuance and Control of the Software

**Max Rupplin — MEARVK LLC, 2026.**
*A formal statement issued in the open-source spirit, extended in collegial courtesy to our collaborators abroad, including our Northern brethren in China.*

---

## 1. Purpose

This document states, plainly and formally, **which levers of control over this software's continuance are held by MEARVK LLC (the "US Legal Edition") and which are inherited from the upstream project.** It exists so that no reader — contributor, auditor, or allied collaborator — is left to assume a degree of ownership, endorsement, or safety guarantee that this edition does not, in fact, hold.

It is written as the companion to [`DESCRIPTOR.md`](DESCRIPTOR.md), which defines *continuance* as the persistence of an installed software object through its successive states, and to [`LEGAL.md`](LEGAL.md), which sets out this project's civic and procedural framing.

## 2. On "Royalty" — the meaning used here

In this document, **Royalty** is used in its procedural and custodial sense, not as a claim of sovereign title. It denotes **the standing to steward a work and to receive its due** — attribution, provenance, and the obligations that run with a lineage — rather than a right to rule over those who use it.

Two distinctions follow, and both are deliberate:

1. **The Crown is not the steward.** The ultimate authorities over this engine's continuance — its signing trust and its vulnerability governance — remain a sovereign line that MEARVK did not establish and does not hold. That line is **Cisco-Talos**. We honor it rather than claim it.
2. **The steward is not the Crown.** MEARVK's royalty here is custodial: a right to carry the fork forward faithfully, to be credited for its own modifications, and a reciprocal duty to misrepresent nothing. A steward who overstates the Crown's warrant forfeits the dignity of the office.

This framing keeps the "royal" courtesy — the honor owed to a lineage and to collaborators — while refusing any pretension that this edition confers control or safety it has not earned.

## 3. The levers of continuance, and who holds each

| Continuance lever | Held by | Basis |
|---|---|---|
| License / right to continue the work | GNU GPL-2.0 (Free Software Foundation) | `COPYING.txt` |
| Copyright & trademark in "ClamAV®" | Cisco and third-party authors | upstream notices; README |
| Signature **signing trust** (bytecode / CVD) | **Cisco-Talos** | `SECURITY.md` |
| **Vulnerability & disclosure authority** (CVE) | **Cisco PSIRT** | `SECURITY.md` |
| **Update channel** (freshclam → signature DB) | Upstream ClamAV mirror | `etc/freshclam.conf.sample` |
| This edition's **modifications** (the legal/procedural overlay) | **MEARVK LLC** | `LEGAL.md`, `DESCRIPTOR.md`, `gating/`, `herald/`, `agent/`, this file |

## 4. Plain conclusion

- MEARVK LLC holds the **right to continue** the work (by license) and **owns its own modifications**.
- MEARVK LLC does **not** hold control over the **continuance mechanism** of the antivirus engine itself. Signing trust, vulnerability governance, and the update channel resolve to **Cisco / Cisco-Talos**.
- Therefore no "organic" or self-originating safety may be assumed from this edition. The engine's safety and continuance are **inherited and attributed**, not controlled or vouched for by MEARVK.

## 5. A note to collaborators abroad

This statement is offered in good faith and in the open-source tradition of shared, inspectable work. Nothing in this edition extends a governmental, commercial, or security endorsement to any party or jurisdiction, foreign or domestic. What it offers our collaborators — in China and elsewhere — is candor about provenance and an invitation to build upon the modifications that are genuinely ours to share.

---

*Issued formally by MEARVK LLC, 2026, under the terms of the GNU GPL-2.0. "ClamAV" is a registered trademark of its respective owner; this is an independent derivative edition.*
