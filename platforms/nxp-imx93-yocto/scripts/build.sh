#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 PragSol Consulting, LLC
# Website: https://www.pragsolconsulting.com/
#
set -euo pipefail
[ "${1:-}" = development ] || {
    echo "usage: ACCEPT_FSL_EULA=1 $0 development [--release RELEASE] [-- bitbake-command ...]" >&2
    exit 2
}
[ "${ACCEPT_FSL_EULA:-}" = 1 ] || {
    echo "ACCEPT_FSL_EULA=1 is required before building; review sources/meta-freescale/EULA and sources/meta-imx/LICENSE.txt" >&2
    exit 2
}
shift
root="$(cd "$(dirname "$0")/.." && pwd -P)"
export GEISA_PLATFORM_ROOT="$root"
# shellcheck source=release-profile.sh
. "$root/scripts/release-profile.sh"

release=
while [ "$#" -gt 0 ]; do
    case "$1" in
        --release)
            [ "$#" -ge 2 ] || {
                echo "--release requires a release identifier" >&2
                exit 2
            }
            release="$2"
            shift 2
            ;;
        --)
            shift
            break
            ;;
        *)
            echo "unknown build option: $1" >&2
            exit 2
            ;;
    esac
done

release_profile_select "$release"
release_profile_check_complete
build_config="$root/build-configuration/releases/$RELEASE_ID.conf"
[ -r "$build_config" ] || {
    echo "missing build configuration for $RELEASE_ID: $build_config" >&2
    exit 1
}
for setting in RELEASE_ID BUILD_DIR MACHINE DISTRO IMAGE; do
    profile_value="${!setting}"
    grep -Fqx "$setting=\"$profile_value\"" "$build_config" || {
        echo "build configuration disagrees with release profile: $setting" >&2
        exit 1
    }
done
release_profile_check_sources --check

build="$root/$BUILD_DIR"
mkdir -p "$build/conf"
sed "s|@PLATFORM_ROOT@|$root|g" "$root/build-configuration/templates/bblayers.conf" > "$build/conf/bblayers.conf"
cp "$root/build-configuration/templates/local.conf" "$build/conf/local.conf"
state_file="$build/conf/geisa-source-state.conf"
rm -f "$state_file"
"$root/scripts/geisa-source-state.sh" --release "$RELEASE_ID" "$state_file"
cat > "$build/conf/auto.conf" <<EOF
ACCEPT_FSL_EULA = "1"
DL_DIR = "${DL_DIR:-$HOME/yocto-cache/downloads}"
SSTATE_DIR = "${SSTATE_DIR:-$HOME/yocto-cache/sstate}"
MACHINE = "$MACHINE"
DISTRO = "$DISTRO"
EOF
set +u
. "$root/sources/poky/oe-init-build-env" "$build" >/dev/null
set -u
[ "$#" -gt 0 ] || set -- bitbake "$IMAGE"
"$@"
