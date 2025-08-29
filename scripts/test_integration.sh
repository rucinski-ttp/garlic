#!/bin/bash

# Build, flash, and run integration (hardware) tests
# Usage: ./scripts/test_integration.sh [--port /dev/ttyACM0]

set -euo pipefail

PORT=""
while [[ $# -gt 0 ]]; do
  case $1 in
    --port|-p)
      PORT="$2"; shift 2;;
    *)
      echo "Unknown arg: $1"; exit 1;;
  esac
done

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# Build & flash
"${SCRIPT_DIR}/build.sh"
"${SCRIPT_DIR}/flash.sh"

# Activate venv for pytest
source "${SCRIPT_DIR}/source.sh" >/dev/null 2>&1 || true

cd "${SCRIPT_DIR}/.."
echo "Running pytest integration tests (J-Link RTT)..."
if [[ -n "${PORT}" ]]; then
  pytest -q tests/integration --run-hardware --garlic-port "${PORT}" --garlic-rtt-tool jlink -s
else
  pytest -q tests/integration --run-hardware --garlic-rtt-tool jlink -s
fi
