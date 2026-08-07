#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 PragSol Consulting, LLC
# Website: https://www.pragsolconsulting.com/
#
set -euo pipefail

out=
release=
while [ "$#" -gt 0 ]; do
    case "$1" in
        --release)
            [ "$#" -ge 2 ] || { echo "--release requires an identifier" >&2; exit 2; }
            release="$2"
            shift 2
            ;;
        --)
            shift
            break
            ;;
        *)
            if [ -z "$out" ]; then
                out="$1"
                shift
            else
                echo "unexpected argument: $1" >&2
                exit 2
            fi
            ;;
    esac
done
[ -n "$out" ] || { echo "usage: $0 [--release RELEASE] OUTPUT" >&2; exit 2; }

platform_root="$(cd "$(dirname "$0")/.." && pwd -P)"
export GEISA_PLATFORM_ROOT="$platform_root"
# shellcheck source=release-profile.sh
. "$platform_root/scripts/release-profile.sh"
release_profile_select "$release"
release_profile_check_complete
release_profile_check_sources --check >/dev/null

git_root="$(git -C "$platform_root" rev-parse --show-toplevel)"
prefix="$(git -C "$platform_root" rev-parse --show-prefix)"
tmp="$out.tmp.$$"
rm -f "$out" "$tmp"
cleanup() { rm -f "$tmp"; }
trap cleanup EXIT HUP INT TERM

source_state_path() {
    printf '%s/%s' "$platform_root" "$(release_profile_source_path "$1")"
}

outer_status_filtered() {
    local line path
    while IFS= read -r line || [ -n "$line" ]; do
        [ -n "$line" ] || continue
        path="${line:3}"
        case "$path" in
            "${prefix}"sources/poky|"${prefix}"sources/meta-arm|"${prefix}"sources/meta-freescale|"${prefix}"sources/meta-imx|"${prefix}"sources/meta-imx-frdm|"${prefix}"sources/meta-openembedded|"${prefix}"sources/meta-virtualization|"${prefix}"sources/meta-clang|"${prefix}"sources/meta-security)
                ;;
            *) printf '%s\n' "$line" ;;
        esac
    done < <(git -C "$git_root" status --porcelain=v1 --untracked-files=all)
}

outer_excludes=(
    ":(exclude)${prefix}sources/poky"
    ":(exclude)${prefix}sources/meta-arm"
    ":(exclude)${prefix}sources/meta-freescale"
    ":(exclude)${prefix}sources/meta-imx"
    ":(exclude)${prefix}sources/meta-imx-frdm"
    ":(exclude)${prefix}sources/meta-openembedded"
    ":(exclude)${prefix}sources/meta-virtualization"
    ":(exclude)${prefix}sources/meta-clang"
    ":(exclude)${prefix}sources/meta-security"
)

source_dirty=false
# shellcheck disable=SC2154
source_state_lines=
# shellcheck disable=SC2154
for source_id in "${release_profile_source_ids[@]}"; do
    source_var="$(release_profile_source_var "$source_id")"
    expected="${!source_var}"
    actual="$(git -C "$(source_state_path "$source_id")" rev-parse HEAD)"
    dirty=false
    release_profile_source_dirty "$source_id" && dirty=true
    [ "$dirty" = true ] && source_dirty=true
    key="${source_id^^}"
    key="${key//-/_}"
    source_state_lines+="GEISA_SOURCE_EXPECTED_$key=\"$expected\"\n"
    source_state_lines+="GEISA_SOURCE_ACTUAL_$key=\"$actual\"\n"
    source_state_lines+="GEISA_SOURCE_DIRTY_$key=\"$dirty\"\n"
done

outer_status="$(outer_status_filtered)"
outer_dirty=false
[ -n "$outer_status" ] && outer_dirty=true
dirty=false
[ "$outer_dirty" = true ] && dirty=true
[ "$source_dirty" = true ] && dirty=true

if [ "$dirty" = true ]; then
    digest="$(
        {
            git -C "$git_root" diff --no-ext-diff --binary HEAD -- . "${outer_excludes[@]}"
            git -C "$git_root" diff --cached --no-ext-diff --binary HEAD -- . "${outer_excludes[@]}"
            printf '%s\n' "$outer_status"
# shellcheck disable=SC2154
            for source_id in "${release_profile_source_ids[@]}"; do
                source_path="$(source_state_path "$source_id")"
                printf 'SOURCE %s\n' "$source_id"
                git -C "$source_path" diff --no-ext-diff --binary HEAD
                git -C "$source_path" diff --cached --no-ext-diff --binary HEAD
                git -C "$source_path" status --porcelain=v1 --untracked-files=all
                git -C "$source_path" ls-files --stage
                git -C "$source_path" ls-files --others --exclude-standard -z |
                    LC_ALL=C sort -z | xargs -0 -r sha256sum
            done
        } | sha256sum | awk '{print $1}'
    )"
else
    digest=clean
fi

{
    printf 'GEISA_RELEASE_ID = "%s"\n' "$RELEASE_ID"
    printf 'GEISA_RELEASE_STATUS = "%s"\n' "$RELEASE_PROFILE_STATUS"
    printf 'GEISA_NXP_BSP_VERSION = "%s"\n' "$NXP_BSP_VERSION"
    printf 'GEISA_YOCTO_SERIES = "%s"\n' "$YOCTO_SERIES"
    printf 'GEISA_RELEASE_PROFILE_SHA256 = "%s"\n' "$RELEASE_PROFILE_HASH"
    printf 'GEISA_SOURCE_REVISION = "%s"\n' "$(git -C "$git_root" rev-parse HEAD)"
    printf 'GEISA_SOURCE_DIRTY = "%s"\n' "$dirty"
    printf 'GEISA_SOURCE_DIRTY_DIGEST = "%s"\n' "$digest"
    printf 'GEISA_SOURCE_RELEASE_MATCH = "true"\n'
    printf 'GEISA_EXPECTED_KERNEL_VERSION = "%s"\n' "$KERNEL_VERSION"
    printf 'GEISA_EXPECTED_KERNEL_SRCREV = "%s"\n' "$KERNEL_SRCREV"
    printf 'GEISA_EXPECTED_UBOOT_VERSION = "%s"\n' "$UBOOT_VERSION"
    printf 'GEISA_EXPECTED_UBOOT_SRCREV = "%s"\n' "$UBOOT_SRCREV"
    printf '%b' "$source_state_lines"
} > "$tmp"
mv -f "$tmp" "$out"
trap - EXIT HUP INT TERM
