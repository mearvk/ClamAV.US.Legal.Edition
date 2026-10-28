# ClamAV.US.Legal.Edition

**An independent ClamAV derivative with a documented legal/procedural analysis layer.**

This repository combines the ClamAV antivirus engine source with MEARVK LLC's project-specific documentation and analytical materials concerning software provenance, state changes, and continuity. The analysis layer is documentary and procedural; it is not a replacement scanning engine, a certification, or a claim of official upstream endorsement.

> **Upstream identity and security boundary:** ClamAV is an upstream project associated with Cisco Talos. This repository is independently maintained and is not the official ClamAV project. The fork does not control upstream signing trust, signature-database infrastructure, or upstream vulnerability disclosure. See [CONTINUANCE.md](CONTINUANCE.md) and [SECURITY.md](SECURITY.md).

## Start here

- [Legal and procedural analysis](LEGAL.md) — project position and limitations.
- [Continuance and control](CONTINUANCE.md) — which responsibilities are upstream versus fork-specific.
- [Procedural descriptor](DESCRIPTOR.md) — conceptual state and retro-dependency model.
- [Security policy](SECURITY.md) — reporting and upstream boundaries.
- [License notices](COPYING.txt) and [third-party notices](COPYING/).

## Antivirus engine and build instructions

The antivirus engine and its build system originate from upstream ClamAV. For authoritative usage guidance, consult the [upstream ClamAV repository](https://github.com/Cisco-Talos/clamav) and [official documentation](https://docs.clamav.net/). This fork may differ from upstream; review the local [INSTALL.md](INSTALL.md), [CMakeLists.txt](CMakeLists.txt), and [CMake presets](CMakePresets.json) for the exact source revision in this repository.

Do not assume that an upstream installation guide, binary, badge, or security statement proves that this fork has been built, tested, signed, or audited.

## What this edition adds

The project-specific material proposes a way to record the history and context surrounding an installed scanner: artifact identity, dependencies, configuration, signature-database state, execution observations, and intervening events. The proposed record fields and limitations are documented in [DESCRIPTOR.md](DESCRIPTOR.md); release provenance and attribution guidance are in [CONTINUANCE.md](CONTINUANCE.md). The model is intended to support human review and future automation.

**Current limitation:** documentation of a model does not mean the model is automatically collected or enforced by the ClamAV engine. Only capabilities demonstrated by code and tests should be described as implemented.

## Verification and CI

The repository includes existing CMake build, formatting, and CodeQL workflows under [`.github/workflows/`](.github/workflows/). Their results should be checked on the current commit before making release or security claims. A successful build or static-analysis run is useful evidence, but is not proof of absence of vulnerabilities.

## Legal and attribution notes

- This is an independent derivative; upstream names, copyrights, trademarks, and notices remain with their respective holders.
- Follow the repository's [license text](COPYING.txt) and applicable third-party notices in [`COPYING/`](COPYING/). Check component-specific terms before redistributing.
- [LEGAL.md](LEGAL.md) describes project analysis and positions; it is **not** a substitute for the software license and does not independently create legal rights or establish compliance.
- The documentation is informational and is not legal advice.

## Contributing

Please include the reason for a change, the files or behaviors affected, how it was tested, and whether the change modifies upstream-derived code or the project-specific analysis layer. Preserve existing upstream notices and avoid claiming that a change has been verified unless the relevant checks were run.

For engine defects and upstream security issues, consult the official [ClamAV project](https://github.com/Cisco-Talos/clamav) and the reporting instructions in [SECURITY.md](SECURITY.md).
