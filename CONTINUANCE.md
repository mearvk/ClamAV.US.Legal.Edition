# Continuance and Control of the Software

**Max Rupplin — MEARVK LLC, 2026.**
*A formal statement issued in the open-source spirit, extended in collegial courtesy to our collaborators abroad, including our Northern brethren in China.*

> **Languages** · English (this document, governing) · [中文 / Chinese](CONTINUANCE.zh-CN.md)

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

---

# 软件之延续与控制（中文）

**Max Rupplin — MEARVK LLC，2026 年。**
*本声明本着开源精神正式发布，并以同僚之谊致意我们海外的合作者，包括我们在中国的北方弟兄。*

> 译者说明：本中文部分为上文英文声明的忠实对照译本。若两种文本在含义上出现歧义，以英文原文为准。

## 1. 目的

本文件明确而正式地说明：**对本软件之"延续"（continuance），哪些控制权由 MEARVK LLC（即"美国法律版"）持有，哪些系承自上游项目。** 其目的在于，使任何读者——无论是贡献者、审计者，还是友好的合作者——都不致误以为本版本拥有其实际并不持有的所有权、背书或安全保证。

本文件与 [`DESCRIPTOR.md`](DESCRIPTOR.md)（将"延续"界定为已安装软件对象历经其各个状态而持续存在）及 [`LEGAL.md`](LEGAL.md)（阐明本项目的公民与程序性框架）互为补充。

## 2. 关于"王权"（Royalty）——本文所用之义

本文中的**王权**，取其程序性与托管性之义，而非主权名分之主张。它所指的，是**守护一件作品并受领其应得之礼的身份**——即署名、来源溯源，以及随血脉而行的义务——而非统治使用者之权。

由此引出两条有意为之的区分：

1. **王冠非守护者。** 对本引擎之延续握有终极权柄者——其签名信任与漏洞治理——仍属一条 MEARVK 既未开创、亦未持有的主权脉络。此脉络即 **Cisco-Talos**。我们对其致敬，而不僭称。
2. **守护者非王冠。** MEARVK 在此之王权是托管性的：忠实地将此分支承续向前之权、就其自身修改受领署名之权，以及一份不得有任何失实陈述的对等义务。若守护者夸大王冠之授权，便失却此职之尊严。

至于**剑（Sword）**：剑并非征服之具，而是守护者所持的纪律。它所斩除者,乃是夸张、冒称与失实——而非他人。王冠象征所承之血脉，守护者象征所尽之本分，而剑象征二者之间那条明亮的界线：凡非我们所应得者，绝不僭取。礼待在于克制，而非扩张。

此一框架既保全了"王者"之礼——即对血脉与合作者应有的敬意——又摒弃任何佯称，不假冒本版本并未赢得的控制权或安全保证。

## 3. 延续之各项权柄，及其归属

| 延续之权柄 | 持有者 | 依据 |
|---|---|---|
| 许可 / 承续此作品之权 | GNU GPL-2.0（自由软件基金会） | `COPYING.txt` |
| "ClamAV®" 之版权与商标 | Cisco 及第三方作者 | 上游声明；README |
| 特征码**签名信任**（bytecode / CVD） | **Cisco-Talos** | `SECURITY.md` |
| **漏洞与披露权柄**（CVE） | **Cisco PSIRT** | `SECURITY.md` |
| **更新通道**（freshclam → 特征码库） | 上游 ClamAV 镜像 | `etc/freshclam.conf.sample` |
| 本版本之**修改部分**（法律 / 程序性叠加层） | **MEARVK LLC** | `LEGAL.md`、`DESCRIPTOR.md`、`gating/`、`herald/`、`agent/`，及本文件 |

## 4. 明白无误之结论

- MEARVK LLC 持有承续此作品之**权利**（经由许可），并**拥有其自身的修改部分**。
- MEARVK LLC **并不**持有对杀毒引擎本身之**延续机制**的控制权。签名信任、漏洞治理与更新通道，均归于 **Cisco / Cisco-Talos**。
- 因此，不得就本版本臆断任何"内生的"或自源的安全性。本引擎之安全与延续系**承自上游并予以署名**，而非由 MEARVK 控制或担保。

## 5. 致海外合作者

本声明本着善意发布，秉持开源传统中共享、可查之作的精神。本版本不向任何一方或任何法域——无论境外或境内——延伸任何政府性、商业性或安全性的背书。它向我们的合作者——在中国及他处——所奉上者，是对来源的坦诚，以及一份邀请：在真正属于我们、可以分享的修改之上继续建造。

---

*由 MEARVK LLC 于 2026 年依 GNU GPL-2.0 条款正式发布。"ClamAV" 为其各自所有者之注册商标；本版本系独立的衍生版本。*


## 6. Release provenance checklist

For each maintained release, record the upstream source URL, exact upstream tag or commit SHA, date obtained, local import commit, and the method used to import or merge the source. If the exact baseline cannot be established, mark it **unknown** rather than inferring it from the repository name.

Also record the locally modified paths, a summary of fork-specific changes, reviewed license notices, relevant CI results, and (when generated) the SBOM format and artifact hashes. Preserve upstream copyright and trademark notices and the component-specific terms in `COPYING/`.

A Git commit hash identifies a Git object; by itself it does not prove authorship, approval, build reproducibility, or software safety. Distinguish observed evidence, inference, and unknowns. This checklist is a maintenance procedure, not a claim that every item has already been completed.


## 7. License and attribution review

The top-level license notice is not a substitute for reviewing the terms that apply to each bundled or copied component. Before a release or material dependency change:

- Preserve upstream copyright, license, and trademark notices.
- Check `COPYING.txt` and component notices under `COPYING/`.
- Record new dependencies, versions, source locations, and license identifiers.
- Keep fork-specific modifications distinguishable from upstream code.
- Do not imply that MEARVK owns upstream code or that project-authored statements supersede third-party license terms.

This is a review checklist, not a legal determination that the repository is fully compliant. Obtain qualified legal review when license compatibility or redistribution obligations are uncertain.
