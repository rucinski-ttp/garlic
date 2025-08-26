#!/bin/bash

# Source environment variables for Zephyr development
# Usage: source scripts/source.sh

echo "Setting up Zephyr environment..."

# Activate Python virtual environment
source ~/zephyr-venv/bin/activate

# Set Zephyr SDK path
export ZEPHYR_SDK_INSTALL_DIR=~/tools/zephyr-global/zephyr-sdk-0.16.8

# Set Zephyr base (updated by west)
export ZEPHYR_BASE=/projects/garlic/zephyr

echo "Environment ready!"
echo "  Python venv: $(which python3)"
echo "  Zephyr SDK: $ZEPHYR_SDK_INSTALL_DIR"
echo "  Zephyr base: $ZEPHYR_BASE"