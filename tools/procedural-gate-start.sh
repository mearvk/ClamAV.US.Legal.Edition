#!/bin/sh
# Max Rupplin - MEARVK LLC - 2026.
# Start one advisory gate. It does not replace clamscan/clamd and never clears a ClamAV detection.
set -eu
LEVEL=${1:-}
OBJECT=${2:-}
case "$LEVEL" in
  1|2|3|4|5|6|7|8) ;;
  *) echo "usage: $0 [1-8] OBJECT" >&2; exit 2 ;;
esac
[ -n "$OBJECT" ] || { echo "OBJECT is required" >&2; exit 2; }
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
case "$LEVEL" in
  1) DIR="$ROOT/gating/1"; BIN="$DIR/gate1";;
  2) DIR="$ROOT/herald/2"; BIN="$DIR/herald2";;
  3) DIR="$ROOT/gating/3"; BIN="$DIR/gate3";;
  4) DIR="$ROOT/herald/4"; BIN="$DIR/herald4";;
  5) DIR="$ROOT/gating/5"; BIN="$DIR/gate5";;
  6) DIR="$ROOT/herald/6"; BIN="$DIR/herald6";;
  7) DIR="$ROOT/gating/7"; BIN="$DIR/gate7";;
  8) DIR="$ROOT/herald/8"; BIN="$DIR/herald8";;
esac
CONFIG="$DIR/config.json"
[ -f "$CONFIG" ] || { echo "missing config: $CONFIG" >&2; exit 3; }
[ -x "$BIN" ] || { echo "gate is not built: $BIN; run the CMake/Make build first" >&2; exit 4; }
exec "$BIN" "$OBJECT" "$CONFIG"
