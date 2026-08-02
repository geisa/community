#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 PragSol Consulting, LLC
# Website: https://www.pragsolconsulting.com/
#
set -euo pipefail
[ "${1:-}" = development ] || { echo "usage: ACCEPT_FSL_EULA=1 $0 development [-- bitbake-command ...]" >&2; exit 2; }
[ "${ACCEPT_FSL_EULA:-}" = 1 ] || { echo "ACCEPT_FSL_EULA=1 is required before building; review sources/meta-freescale/EULA and sources/meta-imx/LICENSE.txt" >&2; exit 2; }
shift
[ "${1:-}" != -- ] || shift
root="$(cd "$(dirname "$0")/.." && pwd -P)"
build="$root/build-development"
mkdir -p "$build/conf"
sed "s|@PLATFORM_ROOT@|$root|g" "$root/build-configuration/templates/bblayers.conf" > "$build/conf/bblayers.conf"
cp "$root/build-configuration/templates/local.conf" "$build/conf/local.conf"
state_file="$build/conf/geisa-source-state.conf"
rm -f "$state_file"
"$root/scripts/geisa-source-state.sh" "$state_file"
cat > "$build/conf/auto.conf" <<EOF
ACCEPT_FSL_EULA = "1"
DL_DIR = "${DL_DIR:-$HOME/yocto-cache/downloads}"
SSTATE_DIR = "${SSTATE_DIR:-$HOME/yocto-cache/sstate}"
MACHINE = "geisa-imx93"
DISTRO = "geisa-dev"
EOF
set +u
. "$root/sources/poky/oe-init-build-env" "$build" >/dev/null
set -u
[ "$#" -gt 0 ] || set -- bitbake geisa-dev-image
"$@"
