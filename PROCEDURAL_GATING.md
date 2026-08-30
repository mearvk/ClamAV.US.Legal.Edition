# Procedural Gating and Herald Scaffold

**Max Rupplin - MEARVK LLC - 2026.**

This is an opt-in experimental layer around ClamAV. It is deliberately additive: it must never downgrade, erase, or reinterpret an upstream ClamAV malware detection as safe.

## Eight sequitur levels

| Level | Directory | Function |
|---|---|---|
| 1 | `gating/1` | baseline provenance |
| 2 | `herald/2` | second-look confirmation |
| 3 | `gating/3` | parent/child tracing |
| 4 | `herald/4` | evidence qualification |
| 5 | `gating/5` | causal attachment |
| 6 | `herald/6` | attenuation and uncertainty |
| 7 | `gating/7` | explicit qualified closure |
| 8 | `herald/8` | final operator herald and closure |

Each level inherits the concerns of the preceding level and adds another gate. The progression is intentionally more cautious, not more permissive.

## Start command

Use the supplied dispatcher with a level and an object:

`sh tools/procedural-gate-start.sh 1 /path/to/object`

Valid levels are `1` through `8`. The corresponding JSON configuration is selected automatically.

The executable is an advisory procedural check. The normal ClamAV engine remains the detection authority.

## CMake

`gating/CMakeLists.txt` provides opt-in targets for all eight levels. ClamAV's normal CMake development workflow uses CMake/Ninja and CTest; this scaffold follows that model. citeturn0search0turn0search1

To incorporate the directory into a consuming build, use CMake's `add_subdirectory(gating)` mechanism. CMake processes a subdirectory's `CMakeLists.txt` as part of the parent build. citeturn2search0turn2search6

## Safety rules

1. Never treat absence of a gate finding as proof of safety.
2. Never replace a ClamAV detection with a gate result.
3. Preserve uncertainty when content cannot be completely observed.
4. Preserve provenance where available.
5. Require explicit closure at level 8.
6. Keep authorship, provenance, detection, and legal responsibility distinct.

The scaffold is intentionally small and reviewable before any deeper coupling to `libclamav`. This is consistent with a cautious secure-development approach in which provenance and security decisions remain explicit and reviewable. NIST's SSDF emphasizes addressing vulnerability root causes and collecting provenance data for software components. citeturn0search2turn0search3
