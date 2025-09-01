#!/usr/bin/env bash
# Wrapper to capture RTT logs with J-Link
# Usage:
#   ./scripts/rtt_capture.sh [--tool auto|jlink-logger|jlink] [--outfile <path>] [--channel N] [--timeout SEC]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Activate venv if available (non-fatal)
source "${SCRIPT_DIR}/source.sh" >/dev/null 2>&1 || true

OUTFILE=""
TOOL="${GARLIC_RTT_TOOL:-auto}"
CHANNEL="${GARLIC_RTT_CHANNEL:-0}"
TIMEOUT=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --tool)
      TOOL="$2"; shift 2;;
    --outfile)
      OUTFILE="$2"; shift 2;;
    --channel)
      CHANNEL="$2"; shift 2;;
    --timeout)
      TIMEOUT="$2"; shift 2;;
    -h|--help)
      echo "Usage: $0 [--tool auto|jlink-logger|jlink] [--outfile <path>] [--channel N]"; exit 0;;
    *)
      echo "Unknown arg: $1"; exit 1;;
  esac
done

if [[ -z "$OUTFILE" ]]; then
  mkdir -p "${SCRIPT_DIR}/../logs"
  OUTFILE="${SCRIPT_DIR}/../logs/rtt_$(date +%s).log"
fi

if [[ -z "$TIMEOUT" ]]; then
  echo "[rtt_capture.sh] No timeout provided; defaulting to 10 seconds"
  TIMEOUT=10
fi

echo "Capturing RTT (tool=$TOOL, channel=$CHANNEL, timeout=${TIMEOUT}s) -> $OUTFILE"

# Always enforce a bash-level timeout as a safety net when TIMEOUT > 0
if [[ "$TIMEOUT" != "0" ]]; then
  # Use SIGTERM first, then SIGKILL after 3s grace
  timeout --signal=TERM --kill-after=3s "${TIMEOUT}s" \
    python3 "${SCRIPT_DIR}/rtt_capture.py" --tool "$TOOL" --channel "$CHANNEL" --outfile "$OUTFILE" --timeout "$TIMEOUT" || true
  # Best-effort cleanup of potential leftover tools
  pkill -f JLinkRTTLogger 2>/dev/null || true
  pkill -f JLinkGDBServer 2>/dev/null || true
  pkill -f JLinkRTTClient 2>/dev/null || true
  exit 0
else
  exec python3 "${SCRIPT_DIR}/rtt_capture.py" --tool "$TOOL" --channel "$CHANNEL" --outfile "$OUTFILE" --timeout "$TIMEOUT"
fi
