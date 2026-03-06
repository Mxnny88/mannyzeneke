#!/usr/bin/env bash
# build_mac.sh — Build ArpSequencer VST3 & AU for macOS
# Usage: ./build_mac.sh [--install]
#   --install  Copy built plugins to ~/Library/Audio/Plug-Ins/

set -euo pipefail

INSTALL=false
for arg in "$@"; do
  [[ "$arg" == "--install" ]] && INSTALL=true
done

# ── Prerequisites check ────────────────────────────────────────────────────
check_cmd() {
  if ! command -v "$1" &>/dev/null; then
    echo "ERROR: '$1' not found. $2"
    exit 1
  fi
}
check_cmd cmake  "Install via: brew install cmake"
check_cmd git    "Install Xcode Command Line Tools: xcode-select --install"

# Xcode CLT check
if ! xcode-select -p &>/dev/null; then
  echo "ERROR: Xcode Command Line Tools not installed."
  echo "Run: xcode-select --install"
  exit 1
fi

# ── Detect architecture ────────────────────────────────────────────────────
ARCH=$(uname -m)
if [[ "$ARCH" == "arm64" ]]; then
  CMAKE_ARCH="arm64"
else
  CMAKE_ARCH="x86_64"
fi
echo "Building for architecture: $CMAKE_ARCH"

# ── Build ──────────────────────────────────────────────────────────────────
BUILD_DIR="build-mac"
echo ""
echo "Configuring..."
cmake -B "$BUILD_DIR" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES="$CMAKE_ARCH" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET="11.0" \
  -DBUILD_TESTS=OFF

echo ""
echo "Building (this will take a few minutes on first run while JUCE downloads)..."
cmake --build "$BUILD_DIR" --config Release --parallel "$(sysctl -n hw.logicalcpu)"

# ── Locate artefacts ───────────────────────────────────────────────────────
VST3_SRC="$BUILD_DIR/ArpSequencer_artefacts/Release/VST3/ArpSequencer.vst3"
AU_SRC="$BUILD_DIR/ArpSequencer_artefacts/Release/AU/ArpSequencer.component"

echo ""
echo "Build complete!"
[[ -d "$VST3_SRC" ]] && echo "  VST3: $VST3_SRC"
[[ -d "$AU_SRC"   ]] && echo "  AU:   $AU_SRC"

# ── Install ────────────────────────────────────────────────────────────────
if [[ "$INSTALL" == true ]]; then
  VST3_DEST="$HOME/Library/Audio/Plug-Ins/VST3"
  AU_DEST="$HOME/Library/Audio/Plug-Ins/Components"

  mkdir -p "$VST3_DEST" "$AU_DEST"

  if [[ -d "$VST3_SRC" ]]; then
    rm -rf "$VST3_DEST/ArpSequencer.vst3"
    cp -R "$VST3_SRC" "$VST3_DEST/"
    echo "Installed VST3 → $VST3_DEST/ArpSequencer.vst3"
  fi

  if [[ -d "$AU_SRC" ]]; then
    rm -rf "$AU_DEST/ArpSequencer.component"
    cp -R "$AU_SRC" "$AU_DEST/"
    echo "Installed AU  → $AU_DEST/ArpSequencer.component"
    # Refresh AU cache
    killall -9 AudioComponentRegistrar 2>/dev/null || true
    auval -a 2>/dev/null | grep -i arp || true
  fi

  echo ""
  echo "Done! Restart your DAW and scan for new plugins."
else
  echo ""
  echo "To install, re-run with: ./build_mac.sh --install"
fi
