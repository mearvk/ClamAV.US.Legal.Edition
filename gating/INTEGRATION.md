# Build integration

**Max Rupplin - MEARVK LLC - 2026.**

The eight-level layer is opt-in. The repository already uses CMake and exposes a module path, so the intended integration is:

```cmake
include(ProceduralGating)
```

Then configure with:

```sh
cmake -S . -B build -D ENABLE_PROCEDURAL_GATING=ON
cmake --build build
```

This keeps the experimental layer out of ordinary ClamAV builds until explicitly enabled. That is intentional: the upstream scanner remains the security engine and the procedural layer is an additional caution/explanation mechanism. ClamAV's documented development flow uses CMake, `cmake --build`, and CTest. citeturn0search0turn0search1
