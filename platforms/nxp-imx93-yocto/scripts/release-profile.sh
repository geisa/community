#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 PragSol Consulting, LLC
# Website: https://www.pragsolconsulting.com/
#
set -euo pipefail
# shellcheck disable=SC2034

# shellcheck disable=SC2034
GEISA_PLATFORM_ROOT="${GEISA_PLATFORM_ROOT:-$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)}"
release_profile_error() { printf 'release profile error: %s\n' "$*" >&2; return 1; }

release_profile_known_source_ids=(poky meta-arm meta-freescale meta-imx meta-imx-frdm meta-openembedded meta-virtualization meta-clang meta-security)
release_profile_source_ids=()
release_profile_source_path() {
    case "$1" in
        poky) echo sources/poky;; meta-arm) echo sources/meta-arm;;
        meta-freescale) echo sources/meta-freescale;; meta-imx) echo sources/meta-imx;;
        meta-imx-frdm) echo sources/meta-imx-frdm;;
        meta-openembedded) echo sources/meta-openembedded;;
        meta-virtualization) echo sources/meta-virtualization;;
        meta-clang) echo sources/meta-clang;; meta-security) echo sources/meta-security;;
        *) release_profile_error "unknown source id: $1";;
    esac
}
release_profile_source_var() {
    case "$1" in
        poky) echo SOURCE_POKY;; meta-arm) echo SOURCE_META_ARM;;
        meta-freescale) echo SOURCE_META_FREESCALE;; meta-imx) echo SOURCE_META_IMX;;
        meta-imx-frdm) echo SOURCE_META_IMX_FRDM;;
        meta-openembedded) echo SOURCE_META_OPENEMBEDDED;;
        meta-virtualization) echo SOURCE_META_VIRTUALIZATION;;
        meta-clang) echo SOURCE_META_CLANG;; meta-security) echo SOURCE_META_SECURITY;;
        *) release_profile_error "unknown source id: $1";;
    esac
}
release_profile_validate_inventory() {
    local source_id var value
    local -A seen=()
    release_profile_source_ids=()
    [ -n "${SOURCE_IDS:-}" ] || { release_profile_error "SOURCE_IDS is empty"; return 1; }
    for source_id in $SOURCE_IDS; do
        [ -z "${seen[$source_id]+x}" ] || {
            release_profile_error "duplicate source ID: $source_id"; return 1;
        }
        release_profile_source_path "$source_id" >/dev/null || return 1
        seen[$source_id]=1
        release_profile_source_ids+=("$source_id")
    done
    for source_id in "${release_profile_known_source_ids[@]}"; do
        var="$(release_profile_source_var "$source_id")"; value="${!var:-}"
        if [ -n "${seen[$source_id]+x}" ]; then
            [[ "$value" =~ ^[0-9a-fA-F]{40}$ ]] || {
                release_profile_error "$var must be a full commit for selected source $source_id"; return 1;
            }
        elif [ -n "$value" ]; then
            release_profile_error "$var is set for unselected source $source_id"; return 1
        fi
    done
}
release_profile_validate_layer_paths() {
    local layer_path
    local -A seen=()
    [ -n "${LAYER_PATHS:-}" ] || { release_profile_error "LAYER_PATHS is empty"; return 1; }
    for layer_path in $LAYER_PATHS; do
        [ -z "${seen[$layer_path]+x}" ] || {
            release_profile_error "duplicate layer path: $layer_path"; return 1;
        }
        seen[$layer_path]=1
        [[ "$layer_path" != /* && "$layer_path" != *..* ]] || {
            release_profile_error "unsafe layer path: $layer_path"; return 1;
        }
    done
}
release_profile_default_id() {
    local line file="$GEISA_PLATFORM_ROOT/releases/default"
    [ -r "$file" ] || { release_profile_error "missing default release: $file"; return 1; }
    while IFS= read -r line || [ -n "$line" ]; do
        [ -z "$line" ] || [[ "$line" == \#* ]] && continue
        [[ "$line" =~ ^DEFAULT_RELEASE=\"([A-Za-z0-9._+-]+)\"$ ]] || {
            release_profile_error "invalid default release file: $file"; return 1;
        }
        echo "${BASH_REMATCH[1]}"; return 0
    done < "$file"
    release_profile_error "default release is empty: $file"
}
release_profile_load() {
    local file="$1" line key value
    [ -r "$file" ] || { release_profile_error "missing profile: $file"; return 1; }
    unset RELEASE_ID STATUS NXP_BSP_VERSION YOCTO_SERIES BUILD_DIR MACHINE DISTRO IMAGE KERNEL_VERSION KERNEL_SRCREV UBOOT_VERSION UBOOT_SRCREV SOURCE_IDS LAYER_PATHS GEISA_NXP_MACHINE_INCLUDE GEISA_MACHINE_INCLUDE SOURCE_POKY SOURCE_META_ARM SOURCE_META_FREESCALE SOURCE_META_IMX SOURCE_META_IMX_FRDM SOURCE_META_OPENEMBEDDED SOURCE_META_VIRTUALIZATION SOURCE_META_CLANG SOURCE_META_SECURITY
    while IFS= read -r line || [ -n "$line" ]; do
        [ -z "$line" ] || [[ "$line" == \#* ]] && continue
        [[ "$line" =~ ^([A-Z][A-Z0-9_]*)=\"([^\"]*)\"$ ]] || {
            release_profile_error "invalid profile line in $file: $line"; return 1;
        }
        key="${BASH_REMATCH[1]}"; value="${BASH_REMATCH[2]}"
        case "$key" in
            SOURCE_IDS|LAYER_PATHS)
                [[ "$value" =~ ^[A-Za-z0-9._:/+\ -]+$ ]] || {
                    release_profile_error "unsafe value for $key in $file: $value"; return 1;
                }
                ;;
            *)
                [[ "$value" =~ ^[A-Za-z0-9._:/+-]+$ ]] || {
                    release_profile_error "unsafe value for $key in $file"; return 1;
                }
                ;;
        esac
        case "$key" in
            RELEASE_ID|STATUS|NXP_BSP_VERSION|YOCTO_SERIES|BUILD_DIR|MACHINE|DISTRO|IMAGE|KERNEL_VERSION|KERNEL_SRCREV|UBOOT_VERSION|UBOOT_SRCREV|SOURCE_IDS|LAYER_PATHS|GEISA_NXP_MACHINE_INCLUDE|GEISA_MACHINE_INCLUDE|SOURCE_POKY|SOURCE_META_ARM|SOURCE_META_FREESCALE|SOURCE_META_IMX|SOURCE_META_IMX_FRDM|SOURCE_META_OPENEMBEDDED|SOURCE_META_VIRTUALIZATION|SOURCE_META_CLANG|SOURCE_META_SECURITY) ;;
            *) release_profile_error "unknown key $key in $file"; return 1;;
        esac
        printf -v "$key" '%s' "$value"
    done < "$file"
    [ "${RELEASE_ID:-}" = "$(basename "$file" .conf)" ] || { release_profile_error "profile ID does not match filename: $file"; return 1; }
    [[ "${STATUS:-}" = supported || "${STATUS:-}" = experimental ]] || { release_profile_error "invalid STATUS in $file"; return 1; }
    for key in RELEASE_ID NXP_BSP_VERSION YOCTO_SERIES BUILD_DIR MACHINE DISTRO IMAGE; do
        [ -n "${!key:-}" ] || { release_profile_error "missing $key in $file"; return 1; }
    done
    for key in SOURCE_IDS LAYER_PATHS GEISA_NXP_MACHINE_INCLUDE GEISA_MACHINE_INCLUDE; do
        [ -n "${!key:-}" ] || { release_profile_error "missing $key in $file"; return 1; }
    done
    release_profile_validate_inventory
    release_profile_validate_layer_paths
    # shellcheck disable=SC2034
    RELEASE_PROFILE_FILE="$file"
    # shellcheck disable=SC2034
    RELEASE_PROFILE_HASH="$(sha256sum "$file" | awk '{print $1}')"
    # shellcheck disable=SC2034
    RELEASE_PROFILE_STATUS="$STATUS"
}
release_profile_select() {
    local id="${1:-}"
    [ -n "$id" ] || id="$(release_profile_default_id)"
    [[ "$id" =~ ^[A-Za-z0-9._+-]+$ ]] || { release_profile_error "invalid release ID: $id"; return 1; }
    release_profile_load "$GEISA_PLATFORM_ROOT/releases/$id.conf"
}
release_profile_check_complete() {
    local key value source_id
    local -a missing=()
    for key in KERNEL_VERSION KERNEL_SRCREV UBOOT_VERSION UBOOT_SRCREV; do
        value="${!key:-}"
        [ -n "$value" ] && [ "$value" != unresolved ] || missing+=("$key")
    done
    for source_id in "${release_profile_source_ids[@]}"; do
        key="$(release_profile_source_var "$source_id")"; value="${!key:-}"
        [[ "$value" =~ ^[0-9a-fA-F]{40}$ ]] || missing+=("$key")
    done
    [ "${#missing[@]}" -eq 0 ] || { release_profile_error "release $RELEASE_ID is incomplete; unresolved: ${missing[*]}"; return 1; }
    for key in KERNEL_SRCREV UBOOT_SRCREV; do
        [[ "${!key}" =~ ^[0-9a-fA-F]{40}$ ]] || { release_profile_error "$key is not a full commit"; return 1; }
    done
}
release_profile_source_actual() {
    git -C "$GEISA_PLATFORM_ROOT/$(release_profile_source_path "$1")" rev-parse HEAD 2>/dev/null
}
release_profile_source_dirty() {
    [ -n "$(git -C "$GEISA_PLATFORM_ROOT/$(release_profile_source_path "$1")" status --porcelain=v1 --untracked-files=all 2>/dev/null || true)" ]
}
release_profile_check_sources() {
    local source_id var expected actual dirty mismatch=0
    release_profile_check_complete
    for source_id in "${release_profile_source_ids[@]}"; do
        var="$(release_profile_source_var "$source_id")"; expected="${!var}"
        if ! actual="$(release_profile_source_actual "$source_id")"; then
            echo "source $source_id is not initialized" >&2; mismatch=1; continue
        fi
        dirty=false; release_profile_source_dirty "$source_id" && dirty=true
        printf 'source %-20s expected=%s actual=%s dirty=%s\n' "$source_id" "$expected" "$actual" "$dirty"
        [ "$actual" = "$expected" ] || mismatch=1
    done
    [ "$mismatch" -eq 0 ] || { release_profile_error "source checkout does not match $RELEASE_ID"; return 1; }
}
release_profile_setup_sources() {
    local source_id path var expected actual dirty
    release_profile_check_complete
    for source_id in "${release_profile_source_ids[@]}"; do
        path="$GEISA_PLATFORM_ROOT/$(release_profile_source_path "$source_id")"
        if ! git -C "$path" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
            git -C "$GEISA_PLATFORM_ROOT" submodule update --init --no-fetch -- "$(release_profile_source_path "$source_id")" || {
                release_profile_error "source $source_id is unavailable without an explicit fetch"; return 1;
            }
        fi
        var="$(release_profile_source_var "$source_id")"; expected="${!var}"
        git -C "$path" cat-file -e "$expected^{commit}" 2>/dev/null || {
            release_profile_error "expected commit unavailable for $source_id: $expected"; return 1;
        }
        actual="$(release_profile_source_actual "$source_id")"
        dirty=false; release_profile_source_dirty "$source_id" && dirty=true
        [ "$dirty" = false ] || [ "$actual" = "$expected" ] || {
            release_profile_error "refusing to switch dirty source $source_id"; return 1;
        }
    done
    for source_id in "${release_profile_source_ids[@]}"; do
        path="$GEISA_PLATFORM_ROOT/$(release_profile_source_path "$source_id")"
        var="$(release_profile_source_var "$source_id")"; expected="${!var}"
        actual="$(release_profile_source_actual "$source_id")"
        [ "$actual" = "$expected" ] || git -C "$path" checkout --detach "$expected"
    done
    release_profile_check_sources
}
