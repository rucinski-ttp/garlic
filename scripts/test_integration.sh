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

# Ensure integration test dependencies are installed in the venv
if [ -f "tests/integration/requirements.txt" ]; then
  echo "Installing integration test dependencies..."
  pip install -r tests/integration/requirements.txt >/dev/null
fi
echo "Running pytest integration tests (J-Link RTT)..."
# Prefer J-Link RTT Logger for early boot capture reliability
PYTEST_OPTS=( -q tests/integration --run-hardware --garlic-rtt-tool jlink )
if [[ -n "${PORT}" ]]; then
  PYTEST_OPTS+=( --garlic-port "${PORT}" )
fi
# Default to concise output; use -vv locally if you need more detail
pytest "${PYTEST_OPTS[@]}"
