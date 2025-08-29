#!/bin/bash

# Source environment variables for Zephyr development
# Usage: source scripts/source.sh

echo "Setting up Zephyr environment..."

# Determine script directory robustly when sourced
_SRC_PATH="${BASH_SOURCE[0]:-$0}"
SCRIPT_DIR="$(cd "$(dirname "$_SRC_PATH")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."

# Activate Python virtual environment (prefer repo-local venv)
if [ -f "${ROOT_DIR}/venv/bin/activate" ]; then
  # shellcheck disable=SC1090
  source "${ROOT_DIR}/venv/bin/activate"
fi

# Set Zephyr SDK path
# Allow override; otherwise keep existing if set
export ZEPHYR_SDK_INSTALL_DIR="${ZEPHYR_SDK_INSTALL_DIR:-/home/aaronrucinski/tools/zephyr-global/zephyr-sdk-0.16.8}"

# Set Zephyr base (updated by west)
export ZEPHYR_BASE="${ROOT_DIR}/zephyr"

echo "Environment ready!"
echo "  Python venv: $(which python3)"
echo "  Zephyr SDK: $ZEPHYR_SDK_INSTALL_DIR"
echo "  Zephyr base: $ZEPHYR_BASE"
