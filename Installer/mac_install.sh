#!/usr/bin/env bash
# =============================================================================
# ArpSequencer VST3/AU Plugin – macOS Installer Script
# =============================================================================
# Usage:
#   chmod +x mac_install.sh
#   ./mac_install.sh [--uninstall] [--standalone] [--au] [--vst3] [--all]
#
# Requirements:
#   - macOS 10.13 (High Sierra) or later
#   - CMake build must be complete first:
#       cmake -B build -DCMAKE_BUILD_TYPE=Release
#       cmake --build build --config Release
# =============================================================================

set -euo pipefail

# ─── Config ──────────────────────────────────────────────────────────────────
APP_NAME="ArpSequencer"
APP_VERSION="1.0.0"
PUBLISHER="MannyZeneke"
BUNDLE_ID="com.mannyzeneke.arpsequencer"

# Build artefact locations (relative to repo root, i.e. one level up from Installer/)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$REPO_ROOT/build"

VST3_ARTEFACT="$BUILD_DIR/${APP_NAME}_artefacts/Release/VST3/${APP_NAME}.vst3"
AU_ARTEFACT="$BUILD_DIR/${APP_NAME}_artefacts/Release/AU/${APP_NAME}.component"
SA_ARTEFACT="$BUILD_DIR/${APP_NAME}_artefacts/Release/Standalone/${APP_NAME}.app"

# Install destinations (system-wide)
VST3_DEST="/Library/Audio/Plug-Ins/VST3"
AU_DEST="/Library/Audio/Plug-Ins/Components"
APP_DEST="/Applications"

# User preset directory
PRESET_DIR="$HOME/Library/Application Support/${APP_NAME}/Presets"
LOG_DIR="$HOME/Library/Logs/${APP_NAME}"

# ─── Colour helpers ───────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'
CYAN='\033[0;36m'; NC='\033[0m'; BOLD='\033[1m'

info()    { echo -e "${CYAN}[INFO]${NC}  $*"; }
success() { echo -e "${GREEN}[OK]${NC}    $*"; }
warn()    { echo -e "${YELLOW}[WARN]${NC}  $*"; }
error()   { echo -e "${RED}[ERROR]${NC} $*"; }
header()  { echo -e "\n${BOLD}${CYAN}===== $* =====${NC}\n"; }

# ─── Parse arguments ─────────────────────────────────────────────────────────
INSTALL_VST3=false
INSTALL_AU=false
INSTALL_SA=false
DO_UNINSTALL=false

if [[ $# -eq 0 ]]; then
    # Default: install VST3 + AU
    INSTALL_VST3=true
    INSTALL_AU=true
fi

for arg in "$@"; do
    case $arg in
        --vst3)       INSTALL_VST3=true ;;
        --au)         INSTALL_AU=true   ;;
        --standalone) INSTALL_SA=true   ;;
        --all)        INSTALL_VST3=true; INSTALL_AU=true; INSTALL_SA=true ;;
        --uninstall)  DO_UNINSTALL=true ;;
        --help|-h)
            echo "Usage: $0 [--vst3] [--au] [--standalone] [--all] [--uninstall]"
            exit 0 ;;
        *) warn "Unknown option: $arg" ;;
    esac
done

# ─── System checks ────────────────────────────────────────────────────────────
check_macos_version() {
    local major minor
    major=$(sw_vers -productVersion | cut -d. -f1)
    minor=$(sw_vers -productVersion | cut -d. -f2)
    if (( major < 10 )) || (( major == 10 && minor < 13 )); then
        error "macOS 10.13 (High Sierra) or later is required."
        error "Detected: $(sw_vers -productVersion)"
        exit 1
    fi
    info "macOS version: $(sw_vers -productVersion) ✓"
}

check_build_exists() {
    local path="$1" label="$2"
    if [[ ! -e "$path" ]]; then
        error "$label not found at: $path"
        error "Please build the plugin first:"
        error "  cmake -B build -DCMAKE_BUILD_TYPE=Release"
        error "  cmake --build build --config Release"
        exit 1
    fi
}

require_sudo() {
    if [[ $EUID -ne 0 ]]; then
        info "Some install steps require administrator privileges."
        sudo -v || { error "sudo failed"; exit 1; }
    fi
}

# ─── Uninstall ────────────────────────────────────────────────────────────────
do_uninstall() {
    header "Uninstalling ${APP_NAME} ${APP_VERSION}"
    require_sudo

    local removed=0

    remove_if_exists() {
        local p="$1"
        if [[ -e "$p" ]]; then
            info "Removing: $p"
            sudo rm -rf "$p"
            ((removed++)) || true
        fi
    }

    remove_if_exists "${VST3_DEST}/${APP_NAME}.vst3"
    remove_if_exists "${AU_DEST}/${APP_NAME}.component"
    remove_if_exists "${APP_DEST}/${APP_NAME}.app"

    if (( removed > 0 )); then
        success "Removed $removed plugin item(s)."
        info "Resetting AU cache..."
        sudo killall -9 AudioComponentRegistrar 2>/dev/null || true
        info "Run 'auval -a' or relaunch your DAW to rescan plugins."
    else
        warn "No installed plugin files found."
    fi

    # Offer to remove presets
    echo ""
    read -r -p "Remove user presets at $PRESET_DIR? [y/N] " answer
    if [[ "${answer,,}" == "y" ]]; then
        rm -rf "$PRESET_DIR"
        success "Presets removed."
    else
        info "Presets kept at: $PRESET_DIR"
    fi

    success "Uninstall complete."
}

# ─── Install helpers ──────────────────────────────────────────────────────────
install_plugin() {
    local src="$1" dest_dir="$2" label="$3"

    info "Installing $label..."
    check_build_exists "$src" "$label"

    sudo mkdir -p "$dest_dir"

    # Remove previous version first
    local dest_path="${dest_dir}/$(basename "$src")"
    if [[ -e "$dest_path" ]]; then
        info "Removing previous version at $dest_path"
        sudo rm -rf "$dest_path"
    fi

    sudo cp -R "$src" "$dest_dir/"
    success "$label installed → $dest_dir"
}

codesign_plugin() {
    local path="$1"
    if command -v codesign &>/dev/null; then
        # Ad-hoc sign if no Developer ID is available
        info "Code-signing: $path (ad-hoc)"
        sudo codesign --force --deep --sign - "$path" 2>/dev/null && \
            success "Signed: $(basename "$path")" || \
            warn "Code-signing failed (plugin may still work; see macOS Gatekeeper docs)"
    fi
}

validate_au() {
    if command -v auval &>/dev/null; then
        info "Validating AU component with auval..."
        # auval -v aufx ArSq MnZk
        # (Use your 4-char codes from CMakeLists.txt)
        auval -v aufx ArSq MnZk 2>&1 | tail -5 || \
            warn "AU validation reported issues (may be normal for new installs)"
    else
        warn "auval not found – skipping AU validation"
    fi
}

create_preset_dir() {
    mkdir -p "$PRESET_DIR"
    mkdir -p "$LOG_DIR"
    success "Preset directory ready: $PRESET_DIR"
}

print_summary() {
    header "Installation Summary"
    echo -e "  ${APP_NAME} ${APP_VERSION} by ${PUBLISHER}"
    echo ""
    [[ "$INSTALL_VST3" == true ]] && echo -e "  ${GREEN}✓${NC}  VST3: ${VST3_DEST}/${APP_NAME}.vst3"
    [[ "$INSTALL_AU"   == true ]] && echo -e "  ${GREEN}✓${NC}  AU:   ${AU_DEST}/${APP_NAME}.component"
    [[ "$INSTALL_SA"   == true ]] && echo -e "  ${GREEN}✓${NC}  App:  ${APP_DEST}/${APP_NAME}.app"
    echo ""
    echo -e "  Presets: $PRESET_DIR"
    echo ""
    echo -e "  ${YELLOW}Next steps:${NC}"
    echo -e "    1. Open your DAW (Ableton, Reaper, Logic, etc.)"
    echo -e "    2. Rescan VST3/AU plugins"
    echo -e "    3. Search for '${APP_NAME}'"
    echo ""
}

# ─── Main ────────────────────────────────────────────────────────────────────
header "${APP_NAME} ${APP_VERSION} Installer"

if [[ "$DO_UNINSTALL" == true ]]; then
    do_uninstall
    exit 0
fi

check_macos_version
require_sudo
create_preset_dir

if [[ "$INSTALL_VST3" == true ]]; then
    install_plugin "$VST3_ARTEFACT" "$VST3_DEST" "VST3"
    codesign_plugin "${VST3_DEST}/${APP_NAME}.vst3"
fi

if [[ "$INSTALL_AU" == true ]]; then
    install_plugin "$AU_ARTEFACT" "$AU_DEST" "AU Component"
    codesign_plugin "${AU_DEST}/${APP_NAME}.component"

    # Restart AudioComponentRegistrar so hosts pick up new AU immediately
    info "Refreshing Audio Unit cache..."
    sudo killall -9 AudioComponentRegistrar 2>/dev/null || true
    sleep 1

    validate_au
fi

if [[ "$INSTALL_SA" == true ]]; then
    install_plugin "$SA_ARTEFACT" "$APP_DEST" "Standalone App"
    codesign_plugin "${APP_DEST}/${APP_NAME}.app"

    # Remove quarantine attribute (avoids Gatekeeper warning on first launch)
    sudo xattr -cr "${APP_DEST}/${APP_NAME}.app" 2>/dev/null || true
fi

print_summary
success "Installation complete!"
