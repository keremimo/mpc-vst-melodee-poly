#!/usr/bin/env bash
# Build Melodee Poly for MPC OS (Gen1 armv7, Gen2 aarch64) through mpc-vst-plugins' tools/build_port.sh: the skin from
# vst/layout.conf, params.h, the .so files and the plug-in list entry, in vst/build/. Needs Docker.
#   MPC_VST=/path/to/mpc-vst-plugins vst/build.sh [all|armv7|aarch64]
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$HERE/.." && pwd)"
MPC_VST="${MPC_VST:-$REPO/../mpc-vst-plugins}"
[ -x "$MPC_VST/tools/build_port.sh" ] || { echo "mpc-vst-plugins not found at $MPC_VST (set MPC_VST)" >&2; exit 1; }
"$MPC_VST/tools/build_port.sh" "$HERE/vst.json" "${1:-all}"
