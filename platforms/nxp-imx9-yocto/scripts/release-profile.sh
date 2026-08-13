#!/usr/bin/env bash
set -euo pipefail

GEISA_PLATFORM_ROOT="${GEISA_PLATFORM_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)}"
release_profile_error() { printf 'release profile error: %s\n' "$*" >&2; return 1; }

release_profile_known_source_ids=(base bitbake openembedded-core meta-yocto meta-arm meta-freescale meta-freescale-distro meta-freescale-ml meta-imx meta-openembedded meta-virtualization meta-clang)
release_profile_source_path() {
    case "$1" in
        base) echo sources/base;; bitbake) echo sources/bitbake;; openembedded-core) echo sources/openembedded-core;; meta-yocto) echo sources/meta-yocto;;
        meta-arm) echo sources/meta-arm;; meta-freescale) echo sources/meta-freescale;; meta-freescale-distro) echo sources/meta-freescale-distro;;
        meta-freescale-ml) echo sources/meta-freescale-ml;; meta-imx) echo sources/meta-imx;; meta-openembedded) echo sources/meta-openembedded;;
        meta-virtualization) echo sources/meta-virtualization;; meta-clang) echo sources/meta-clang;;
        *) release_profile_error "unknown source id: $1";;
    esac
}
release_profile_source_var() {
    case "$1" in
        base) echo SOURCE_BASE;; bitbake) echo SOURCE_BITBAKE;; openembedded-core) echo SOURCE_OPENEMBEDDED_CORE;; meta-yocto) echo SOURCE_META_YOCTO;;
        meta-arm) echo SOURCE_META_ARM;; meta-freescale) echo SOURCE_META_FREESCALE;; meta-freescale-distro) echo SOURCE_META_FREESCALE_DISTRO;;
        meta-freescale-ml) echo SOURCE_META_FREESCALE_ML;; meta-imx) echo SOURCE_META_IMX;; meta-openembedded) echo SOURCE_META_OPENEMBEDDED;;
        meta-virtualization) echo SOURCE_META_VIRTUALIZATION;; meta-clang) echo SOURCE_META_CLANG;;
        *) release_profile_error "unknown source id: $1";;
    esac
}
release_profile_validate_inventory() {
    local id var value
    local -A seen=()
    release_profile_source_ids=()
    [ -n "${SOURCE_IDS:-}" ] || { release_profile_error "SOURCE_IDS is empty"; return 1; }
    for id in $SOURCE_IDS; do
        [ -z "${seen[$id]+x}" ] || { release_profile_error "duplicate source ID: $id"; return 1; }
        release_profile_source_path "$id" >/dev/null || return 1
        seen[$id]=1; release_profile_source_ids+=("$id")
    done
    for id in "${release_profile_known_source_ids[@]}"; do
        var="$(release_profile_source_var "$id")"; value="${!var:-}"
        if [ -n "${seen[$id]+x}" ]; then
            [[ "$value" =~ ^[0-9a-fA-F]{40}$ ]] || { release_profile_error "$var must be a full commit"; return 1; }
        elif [ -n "$value" ]; then
            release_profile_error "$var is set for an unselected source $id"; return 1
        fi
    done
}
release_profile_validate_layer_paths() {
    local p; local -A seen=()
    [ -n "${LAYER_PATHS:-}" ] || { release_profile_error "LAYER_PATHS is empty"; return 1; }
    for p in $LAYER_PATHS; do
        [ -z "${seen[$p]+x}" ] || { release_profile_error "duplicate layer path: $p"; return 1; }
        [[ "$p" != /* && "$p" != *..* ]] || { release_profile_error "unsafe layer path: $p"; return 1; }
        seen[$p]=1
    done
}
release_profile_default_id() {
    sed -n 's/^DEFAULT_RELEASE="\([A-Za-z0-9._+-]*\)"$/\1/p' "$GEISA_PLATFORM_ROOT/releases/default" | head -1
}
release_profile_load() {
    local file="$1" line key value
    [ -r "$file" ] || { release_profile_error "missing profile: $file"; return 1; }
    unset RELEASE_ID STATUS NXP_BSP_VERSION YOCTO_SERIES BUILD_DIR IMAGE DISTRO SOURCE_IDS LAYER_PATHS BBMASK_PATHS \
        MACHINE_IMX93 MACHINE_IMX95 GEISA_NXP_MACHINE_INCLUDE_IMX93 GEISA_NXP_MACHINE_INCLUDE_IMX95 \
        GEISA_MACHINE_INCLUDE_IMX93 GEISA_MACHINE_INCLUDE_IMX95 KERNEL_VERSION KERNEL_SRCREV UBOOT_VERSION UBOOT_SRCREV \
        SOURCE_BASE SOURCE_BITBAKE SOURCE_OPENEMBEDDED_CORE SOURCE_META_YOCTO SOURCE_META_ARM SOURCE_META_FREESCALE \
        SOURCE_META_FREESCALE_DISTRO SOURCE_META_FREESCALE_ML SOURCE_META_IMX SOURCE_META_OPENEMBEDDED \
        SOURCE_META_VIRTUALIZATION SOURCE_META_CLANG
    while IFS= read -r line || [ -n "$line" ]; do
        [ -z "$line" ] || [[ "$line" == \#* ]] && continue
        [[ "$line" =~ ^([A-Z][A-Z0-9_]*)=\"([^\"]*)\"$ ]] || { release_profile_error "invalid profile line: $line"; return 1; }
        key="${BASH_REMATCH[1]}"; value="${BASH_REMATCH[2]}"
        case "$key" in
            RELEASE_ID|STATUS|NXP_BSP_VERSION|YOCTO_SERIES|BUILD_DIR|IMAGE|DISTRO|SOURCE_IDS|LAYER_PATHS|BBMASK_PATHS|MACHINE_IMX93|MACHINE_IMX95|GEISA_NXP_MACHINE_INCLUDE_IMX93|GEISA_NXP_MACHINE_INCLUDE_IMX95|GEISA_MACHINE_INCLUDE_IMX93|GEISA_MACHINE_INCLUDE_IMX95|KERNEL_VERSION|KERNEL_SRCREV|UBOOT_VERSION|UBOOT_SRCREV|SOURCE_BASE|SOURCE_BITBAKE|SOURCE_OPENEMBEDDED_CORE|SOURCE_META_YOCTO|SOURCE_META_ARM|SOURCE_META_FREESCALE|SOURCE_META_FREESCALE_DISTRO|SOURCE_META_FREESCALE_ML|SOURCE_META_IMX|SOURCE_META_OPENEMBEDDED|SOURCE_META_VIRTUALIZATION|SOURCE_META_CLANG) ;;
            *) release_profile_error "unknown key $key"; return 1;;
        esac
        printf -v "$key" '%s' "$value"
    done < "$file"
    [ "$RELEASE_ID" = "$(basename "$file" .conf)" ] || { release_profile_error "profile ID mismatch"; return 1; }
    [[ "$STATUS" = experimental ]] || { release_profile_error "invalid status"; return 1; }
    for key in RELEASE_ID NXP_BSP_VERSION YOCTO_SERIES BUILD_DIR IMAGE DISTRO SOURCE_IDS LAYER_PATHS MACHINE_IMX93 MACHINE_IMX95 GEISA_NXP_MACHINE_INCLUDE_IMX93 GEISA_NXP_MACHINE_INCLUDE_IMX95 GEISA_MACHINE_INCLUDE_IMX93 GEISA_MACHINE_INCLUDE_IMX95 KERNEL_VERSION KERNEL_SRCREV UBOOT_VERSION UBOOT_SRCREV; do
        [ -n "${!key:-}" ] || { release_profile_error "missing $key"; return 1; }
    done
    release_profile_validate_inventory
    release_profile_validate_layer_paths
    RELEASE_PROFILE_FILE="$file"
    RELEASE_PROFILE_HASH="$(sha256sum "$file" | awk '{print $1}')"
    RELEASE_PROFILE_STATUS="$STATUS"
}
release_profile_select() {
    local id="${1:-$(release_profile_default_id)}"
    [[ "$id" =~ ^[A-Za-z0-9._+-]+$ ]] || { release_profile_error "invalid release ID"; return 1; }
    release_profile_load "$GEISA_PLATFORM_ROOT/releases/$id.conf"
}
release_profile_check_complete() {
    local id var
    for id in "${release_profile_source_ids[@]}"; do
        var="$(release_profile_source_var "$id")"; [[ "${!var:-}" =~ ^[0-9a-fA-F]{40}$ ]] || { release_profile_error "missing $var"; return 1; }
    done
}
release_profile_source_actual() { git -C "$GEISA_PLATFORM_ROOT/$(release_profile_source_path "$1")" rev-parse HEAD 2>/dev/null; }
release_profile_source_dirty() { [ -n "$(git -C "$GEISA_PLATFORM_ROOT/$(release_profile_source_path "$1")" status --porcelain=v1 --untracked-files=normal 2>/dev/null || true)" ]; }
release_profile_check_sources() {
    local id var expected actual dirty mismatch=0
    release_profile_check_complete
    for id in "${release_profile_source_ids[@]}"; do
        var="$(release_profile_source_var "$id")"; expected="${!var}"
        if ! actual="$(release_profile_source_actual "$id")"; then echo "source $id is not initialized" >&2; mismatch=1; continue; fi
        dirty=false; release_profile_source_dirty "$id" && dirty=true
        printf 'source %-22s expected=%s actual=%s dirty=%s\n' "$id" "$expected" "$actual" "$dirty"
        [ "$actual" = "$expected" ] || mismatch=1
    done
    [ "$mismatch" -eq 0 ] || { release_profile_error "source checkout does not match $RELEASE_ID"; return 1; }
}
release_profile_setup_sources() {
    local id path var expected actual dirty
    release_profile_check_complete
    for id in "${release_profile_source_ids[@]}"; do
        path="$GEISA_PLATFORM_ROOT/$(release_profile_source_path "$id")"
        if ! git -C "$path" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
            git -C "$GEISA_PLATFORM_ROOT" submodule update --init --no-fetch -- "$(release_profile_source_path "$id")" || { release_profile_error "source $id unavailable without fetch"; return 1; }
        fi
        var="$(release_profile_source_var "$id")"; expected="${!var}"
        git -C "$path" cat-file -e "$expected^{commit}" 2>/dev/null || { release_profile_error "expected commit unavailable for $id"; return 1; }
        actual="$(release_profile_source_actual "$id")"; dirty=false; release_profile_source_dirty "$id" && dirty=true
        [ "$dirty" = false ] || [ "$actual" = "$expected" ] || { release_profile_error "refusing to switch dirty source $id"; return 1; }
    done
    for id in "${release_profile_source_ids[@]}"; do
        path="$GEISA_PLATFORM_ROOT/$(release_profile_source_path "$id")"; var="$(release_profile_source_var "$id")"; expected="${!var}"; actual="$(release_profile_source_actual "$id")"
        [ "$actual" = "$expected" ] || git -C "$path" checkout --detach "$expected"
    done
    release_profile_check_sources
}
