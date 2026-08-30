# ClamAV System Agent

**Max Rupplin - MEARVK LLC - 2026.**

The gate (`gating/gating.cpp`) and herald (`herald/herald.cpp`) are in-process
reference engines. This document describes the **system agent** that lets them
sit *linkedly relevant* to the actual ClamAV services a running host operates,
through `systemctl` (or a `clamavctl` shim).

The agent is **additive and advisory**, exactly like the rest of this layer.
It observes; it proposes; it never overrides ClamAV.

## What it observes

| Service | systemd unit (Debian / RedHat / generic) | Role |
|---|---|---|
| `clamd` | `clamav-daemon` / `clamd@scan` / `clamd` | resident scanning daemon |
| `freshclam` | `clamav-freshclam` / `freshclam` / `freshclam` | signature-database updater |
| `clamonacc` | `clamav-clamonacc` / `clamonacc` / `clamonacc` | on-access scanning (needs clamd) |
| `clamav-milter` | `clamav-milter` | optional mail filter |

For each, the agent reads `ActiveState`, `SubState`, `UnitFileState`, `MainPID`
and the activation timestamp via `systemctl show`. It also observes the clamd
**socket liveness** and the **signature-database freshness** (age of
`main.{cvd,cld}` / `daily.{cvd,cld}` against the operator's `freshclam` `Checks`
cadence).

## Operational posture

From those observations the agent derives one **posture**, ordered most-severe
first so the derivation can only become more cautious:

| Posture | Meaning | Gate stance | Herald |
|---|---|---|---|
| `failed` | a required unit is in a failed state | hold, fail-closed | critical |
| `inconsistent` | `clamonacc` up while `clamd` is **not** confirmably running — on-access protection is illusory | **quarantine-grade** | critical |
| `unprotected` | `clamd` not confirmably running, or socket silent | hold, **fail-closed** | critical |
| `degraded` | `clamd` live but database stale/absent, or updater not scheduled | proceed **attenuated** | warning |
| `protected` | `clamd` live, socket answers, database fresh | may proceed | notice |

### Co-concern rules

These are enforced in code (`agent/clamav_agent.hpp`, `agent/bridge.cpp`):

- **O1 — never suppress a detection.** The agent may keep services alive; it
  cannot make a dirty object look clean. The supervisor refuses any proposal
  that would `stop`/`disable` `clamd`.
- **O2 — unknown is fail-closed.** If `clamd` is not confirmably running, object
  safety is *unknown*, and unknown is never "clean". The bridge collapses gate
  confidence to `0.0`.
- **O3 — staleness attenuates.** A running `clamd` with a stale database is
  `degraded`, not `protected`; confidence is capped below `1.0`, never cleared.
- **O4 — inconsistency quarantines.** `clamonacc` without `clamd` is treated as
  a quarantine-grade condition, not ignored.
- **O5 — dry-run by default.** Every mutating control command is withheld unless
  `--execute` is passed **and** the backend binary is installed. `stop`/`disable`
  need their own `--allow-stop`/`--allow-disable` on top.
- **O6 — the more cautious reading wins.** Unparseable or missing state maps to
  `unknown`, never to a running state.

## CLI

The operator front end is a `clamavctl`-style dispatcher
(`agent/clamav_agent_main.cpp`):

```sh
# build (opt-in, alongside the gating scaffold)
cmake -S . -B build -D ENABLE_PROCEDURAL_GATING=ON
cmake --build build --target clamav_agent
# or standalone:
g++ -std=c++17 agent/clamav_agent_main.cpp -o build/clamav_agent

# observe posture (read-only)
sh tools/clamav-agent.sh observe --distro debian

# print the corrective plan without running it
sh tools/clamav-agent.sh plan

# supervise: dry-run (default) then, only if you mean it, execute
sh tools/clamav-agent.sh supervise
sh tools/clamav-agent.sh supervise --execute
```

Subcommands: `status`, `observe`, `plan`, `supervise`, `selftest`.
Flags: `--execute`, `--allow-stop`, `--allow-disable`, `--distro`, `--backend`,
`--db-dir`, `--socket`, `--freshclam-conf`.

All output is JSON so it composes with other tooling. `supervise` exits non-zero
when the system is not in the `protected` posture, so shell callers and systemd
can react.

## systemd integration

Advisory units live under `agent/systemd/`:

- `clamav-agent.service` — a one-shot **dry-run** supervisor, ordered `After=` the
  ClamAV services and hardened (`ProtectSystem=strict`, `NoNewPrivileges=yes`).
- `clamav-agent.timer` — runs the dry-run check every fifteen minutes.
- `clamav-agent-execute.conf` — a **drop-in** that adds `--execute` so the agent
  may restore (start/restart/reload/enable) the ClamAV services. Installing this
  is an explicit operator opt-in; it still never stops/disables `clamd`.

```sh
systemctl enable --now clamav-agent.timer          # periodic dry-run reporting
# opt in to auto-restore:
install -Dm644 agent/systemd/clamav-agent-execute.conf \
    /etc/systemd/system/clamav-agent.service.d/execute.conf
systemctl daemon-reload
```

## How it links to the gate and herald

`agent/bridge.cpp` is the single place all three engines meet. It maps the
posture into:

- **Gate conditioning** — an *environmental precondition* on top of the gate's
  own policy: an unprotected environment forces the gate closed and caps the
  confidence a "clean" scan can earn at `0.0`; a degraded environment caps it
  below `1.0`. The bridge is proven (by self-test) to only ever make the gate
  *more* cautious — never less.
- **Herald conditioning** — the severity at which the environment is announced,
  and whether a clean completion may be narrated at all. Under `unprotected` /
  `inconsistent` / `failed`, the herald may narrate only uncertainty.

The upstream ClamAV engine remains the detection authority. The agent, gate, and
herald form a caution/explanation layer around it — nothing more.
