#!/bin/sh
# Max Rupplin - MEARVK LLC - 2026.
#
# clamav-agent.sh — front-end wrapper for the ClamAV system agent.
#
# The agent sits co-concerned with the ClamAV services (clamd, freshclam,
# clamonacc). It OBSERVES their state and PROPOSES restorative actions; it
# never clears a ClamAV detection and never weakens clamd. Mutating service
# commands are dry-run unless --execute is passed AND the backend is installed.
#
# This wrapper locates the built agent binary and forwards all arguments to it.
# Build the binary first:
#
#   cmake -S . -B build -D ENABLE_PROCEDURAL_GATING=ON
#   cmake --build build --target clamav_agent
#
# or standalone:
#
#   g++ -std=c++17 agent/clamav_agent_main.cpp -o build/clamav_agent
#
# Usage:
#   sh tools/clamav-agent.sh status
#   sh tools/clamav-agent.sh observe --distro debian
#   sh tools/clamav-agent.sh plan
#   sh tools/clamav-agent.sh supervise            # dry-run
#   sh tools/clamav-agent.sh supervise --execute  # actually run the plan
set -eu

ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)

# Search common build output locations for the agent binary.
BIN=""
for candidate in \
    "$ROOT/build/clamav_agent" \
    "$ROOT/build/agent/clamav_agent" \
    "$ROOT/agent/clamav_agent" \
    "$ROOT/clamav_agent"
do
    if [ -x "$candidate" ]; then
        BIN="$candidate"
        break
    fi
done

if [ -z "$BIN" ]; then
    echo "clamav-agent: binary not found; build it first:" >&2
    echo "  cmake -S . -B build -D ENABLE_PROCEDURAL_GATING=ON" >&2
    echo "  cmake --build build --target clamav_agent" >&2
    echo "or: g++ -std=c++17 agent/clamav_agent_main.cpp -o build/clamav_agent" >&2
    exit 4
fi

exec "$BIN" "$@"
