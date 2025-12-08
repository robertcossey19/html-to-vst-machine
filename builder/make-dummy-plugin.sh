#!/usr/bin/env bash
set -e

# Simple placeholder “build” script for Stage 2.
# Later this will run JUCE / iPlug2 / CMake and actually build a VST.
# For now it just creates a fake .vst3 file so you can prove
# that cloud builds and artifact downloads work.

echo "Stage 2: running dummy plugin build..."

# Create output directory
mkdir -p build

# Create a fake VST file
echo "This is a dummy VST file built by the cloud builder." > build/MyPlugin.vst3

echo "Dummy plugin built at build/MyPlugin.vst3"
