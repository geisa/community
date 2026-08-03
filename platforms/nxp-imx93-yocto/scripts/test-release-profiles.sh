#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 PragSol Consulting, LLC
# Website: https://www.pragsolconsulting.com/
#
set -euo pipefail

platform_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
setup="$platform_root/scripts/setup-sources.sh"
state="$platform_root/scripts/geisa-source-state.sh"
export GEISA_PLATFORM_ROOT="$platform_root"
. "$platform_root/scripts/release-profile.sh"
tmp_parent="${TMPDIR:-/tmp}"
tmp_root="$(mktemp -d "$tmp_parent/geisa-release-profile-tests.XXXXXX")" || {
    echo "unable to create release-profile test temporary directory under $tmp_parent" >&2
    exit 1
}
trap 'rm -rf "$tmp_root"' EXIT

assert_fails() {
    if "$@" >"$tmp_root/stdout" 2>"$tmp_root/stderr"; then
        echo "expected failure did not occur: $*" >&2
        exit 1
    fi
}

"$setup" --check >"$tmp_root/default-check"
grep -Fq 'nxp-6.6.52-2.2.2' "$tmp_root/default-check"
"$setup" --check --release nxp-6.6.52-2.2.2 >"$tmp_root/supported-check"
grep -Fq 'nxp-6.6.52-2.2.2' "$tmp_root/supported-check"
assert_fails "$setup" --check --release no-such-release
(
    release_profile_select nxp-6.12.34-2.1.0
    release_profile_check_complete
    [[ " ${SOURCE_IDS} " == *" meta-clang "* && " ${SOURCE_IDS} " == *" meta-security "* ]]
)
(
    release_profile_select nxp-6.6.52-2.2.2
    [[ " ${SOURCE_IDS} " != *" meta-clang "* && " ${SOURCE_IDS} " != *" meta-security "* ]]
)

mkdir -p "$tmp_root/releases"
cp "$platform_root/releases/nxp-6.6.52-2.2.2.conf" "$tmp_root/releases/malformed.conf"
printf 'BAD="value"\n' >> "$tmp_root/releases/malformed.conf"
(
    export GEISA_PLATFORM_ROOT="$tmp_root"
    . "$platform_root/scripts/release-profile.sh"
    assert_fails release_profile_select malformed
)

fake="$tmp_root/fake"
mkdir -p "$fake/releases" "$fake/sources"
declare -A fake_revs
# shellcheck disable=SC2154
for source_id in "${release_profile_known_source_ids[@]}"; do
    path="$fake/$(case "$source_id" in
        poky) echo sources/poky;; meta-arm) echo sources/meta-arm;;
        meta-freescale) echo sources/meta-freescale;; meta-imx) echo sources/meta-imx;;
        meta-imx-frdm) echo sources/meta-imx-frdm;;
        meta-openembedded) echo sources/meta-openembedded;;
        meta-virtualization) echo sources/meta-virtualization;;
        meta-clang) echo sources/meta-clang;; meta-security) echo sources/meta-security;; esac)"
    mkdir -p "$path"
    git -C "$path" init -q
    git -C "$path" config user.name test
    git -C "$path" config user.email test@example.invalid
    printf '%s\n' "$source_id" > "$path/marker"
    git -C "$path" add marker
    git -C "$path" commit -qm initial
    fake_revs["$source_id"]="$(git -C "$path" rev-parse HEAD)"
done
{
    printf '%s\n' 'RELEASE_ID="fake"'
    printf '%s\n' 'STATUS="supported"'
    printf '%s\n' 'NXP_BSP_VERSION="fake"'
    printf '%s\n' 'YOCTO_SERIES="fake"'
    printf '%s\n' 'BUILD_DIR="fake-build"'
    printf '%s\n' 'MACHINE="fake"'
    printf '%s\n' 'DISTRO="fake"'
    printf '%s\n' 'IMAGE="fake"'
    printf '%s\n' 'SOURCE_IDS="poky meta-arm meta-freescale meta-imx meta-imx-frdm meta-openembedded meta-virtualization"'
    printf '%s\n' 'LAYER_PATHS="sources/meta-imx/meta-imx-ml"'
    printf '%s\n' 'GEISA_NXP_MACHINE_INCLUDE="conf/machine/imx93frdm.conf"'
    printf '%s\n' 'GEISA_MACHINE_INCLUDE="conf/machine/geisa-imx93-6.6.inc"'
    printf '%s\n' 'KERNEL_VERSION="6.6.52"'
    printf '%s\n' 'KERNEL_SRCREV="0123456789abcdef0123456789abcdef01234567"'
    printf '%s\n' 'UBOOT_VERSION="2024.04"'
    printf '%s\n' 'UBOOT_SRCREV="fedcba9876543210fedcba9876543210fedcba98"'
# shellcheck disable=SC2154
    for source_id in poky meta-arm meta-freescale meta-imx meta-imx-frdm meta-openembedded meta-virtualization; do
        var="$(release_profile_source_var "$source_id")"
        printf '%s="%s"\n' "$var" "${fake_revs[$source_id]}"
    done
} > "$fake/releases/fake.conf"
(
    export GEISA_PLATFORM_ROOT="$fake"
    . "$platform_root/scripts/release-profile.sh"
    release_profile_select fake
    release_profile_check_sources
    path="$fake/sources/meta-arm"
    initial="$(git -C "$path" rev-parse HEAD)"
    printf '%s\n' second >> "$path/marker"
    git -C "$path" add marker
    git -C "$path" commit -qm second
    second="$(git -C "$path" rev-parse HEAD)"
    git -C "$path" checkout --detach -q "$initial"
    printf '%s\n' untracked > "$path/untracked"
    sed -i "s/^SOURCE_META_ARM=.*/SOURCE_META_ARM=\"$second\"/" "$fake/releases/fake.conf"
    release_profile_select fake
    release_profile_source_dirty meta-arm ||
        fail "expected meta-arm to be reported dirty"
    assert_fails release_profile_setup_sources
)

"$state" --release nxp-6.6.52-2.2.2 "$tmp_root/state-a"
"$state" --release nxp-6.6.52-2.2.2 "$tmp_root/state-b"
cmp "$tmp_root/state-a" "$tmp_root/state-b"
grep -Fqx 'GEISA_RELEASE_ID = "nxp-6.6.52-2.2.2"' "$tmp_root/state-a"
grep -Fqx 'GEISA_SOURCE_RELEASE_MATCH = "true"' "$tmp_root/state-a"
grep -Fqx 'GEISA_EXPECTED_KERNEL_VERSION = "6.6.52"' "$tmp_root/state-a"
grep -Fqx 'GEISA_EXPECTED_UBOOT_VERSION = "2024.04"' "$tmp_root/state-a"

echo "release profile tests passed"
