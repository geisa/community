#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 PragSol Consulting, LLC
# Website: https://www.pragsolconsulting.com/
#

set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd -P)"
platform_root="$(cd "$script_dir/.." && pwd -P)"
export GEISA_PLATFORM_ROOT="$platform_root"
# shellcheck source=release-profile.sh
. "$script_dir/release-profile.sh"

release=
check=false
usage() {
    printf '%s\n' \
        "usage: $0 [--release RELEASE] [--check]" \
        "       $0 --help"
}

while [ "$#" -gt 0 ]; do
    case "$1" in
        --release)
            [ "$#" -ge 2 ] || {
                usage >&2
                exit 2
            }
            release="$2"
            shift 2
            ;;
        --check)
            check=true
            shift
            ;;
        --help|-h)
            usage
            exit 0
            ;;
        *)
            printf 'unknown option: %s\n' "$1" >&2
            usage >&2
            exit 2
            ;;
    esac
done

release_profile_select "$release"
printf 'selected release: %s (%s)\n' "$RELEASE_ID" "$RELEASE_PROFILE_STATUS"
printf 'profile: %s\n' "$RELEASE_PROFILE_FILE"

if [ "$check" = true ]; then
    release_profile_check_sources --check
else
    release_profile_setup_sources
fi
