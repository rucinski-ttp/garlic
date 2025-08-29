#!/usr/bin/env bash
# Wrapper to capture RTT logs with J-Link
# Usage:
#   ./scripts/rtt_capture.sh [--tool auto|jlink-logger|jlink] [--outfile <path>] [--channel N]

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Activate venv if available (non-fatal)
source "${SCRIPT_DIR}/source.sh" >/dev/null 2>&1 || true

OUTFILE=""
TOOL="${GARLIC_RTT_TOOL:-auto}"
CHANNEL="${GARLIC_RTT_CHANNEL:-0}"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --tool)
      TOOL="$2"; shift 2;;
    --outfile)
      OUTFILE="$2"; shift 2;;
    --channel)
      CHANNEL="$2"; shift 2;;
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

echo "Capturing RTT (tool=$TOOL, channel=$CHANNEL) -> $OUTFILE"
exec python3 "${SCRIPT_DIR}/rtt_capture.py" --tool "$TOOL" --channel "$CHANNEL" --outfile "$OUTFILE"
