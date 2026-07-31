#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 PragSol Consulting, LLC
# Website: https://www.pragsolconsulting.com/
#
set -euo pipefail

platform_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
deploy_dir="${platform_root}/build-development/tmp/deploy/images/geisa-imx93"
artifact="${1:-$(find "${deploy_dir}" -maxdepth 1 -type f -name "geisa-dev-image-geisa-imx93.rootfs-*.wic.zst" -printf "%T@ %p\n" | sort -nr | head -1 | cut -d" " -f2-)}"
[ -f "${artifact}" ] || { echo "WIC artifact not found: ${artifact}" >&2; exit 1; }

version_is_compatible() {
    local version="$1" major minor
    major="${version%%.*}"
    minor="${version#*.}"
    minor="${minor%%.*}"
    [[ "${major}" =~ ^[0-9]+$ && "${minor}" =~ ^[0-9]+$ ]] || return 1
    (( major > 1 || (major == 1 && minor >= 47) ))
}

select_e2fsprogs() {
    local candidate version

    if [ -n "${GEISA_E2FSPROGS_BIN_DIR:-}" ]; then
        candidate="${GEISA_E2FSPROGS_BIN_DIR}"
    else
        candidate="$(find "${platform_root}/build-development/tmp/sysroots-components" \
            -path '*/e2fsprogs-native/sbin/e2fsck' -printf '%h\n' 2>/dev/null | \
            sort -u | tail -1)"
    fi

    if [ -z "${candidate}" ]; then
        candidate="$(dirname "$(command -v e2fsck)")"
    fi

    E2FSCK="${candidate}/e2fsck"
    DUMPE2FS="${candidate}/dumpe2fs"
    TUNE2FS="${candidate}/tune2fs"
    for tool in "${E2FSCK}" "${DUMPE2FS}" "${TUNE2FS}"; do
        [ -x "${tool}" ] || {
            echo "Compatible e2fsprogs tools are incomplete in ${candidate}" >&2
            exit 2
        }
    done

    version="$(${E2FSCK} -V 2>&1 | sed -n 's/^e2fsck \([^ ]*\).*/\1/p' | head -1)"
    version_is_compatible "${version}" || {
        echo "Unsupported e2fsprogs ${version:-unknown} at ${candidate}; " \
            "need e2fsprogs 1.47 or later for ext4 orphan_file." >&2
        exit 2
    }
    echo "Using e2fsprogs ${version} from ${candidate}"
}

select_e2fsprogs
sha256sum "${artifact}"
zstd -l "${artifact}"
temporary="$(mktemp "${TMPDIR:-/tmp}/geisa-wic.XXXXXX")"
workspace="$(mktemp -d "${TMPDIR:-/tmp}/geisa-wic-check.XXXXXX")"
trap 'rm -f "${temporary}"; rm -rf "${workspace}"' EXIT
zstd -dc "${artifact}" > "${temporary}"
parted -ms "${temporary}" unit B print
fdisk -l "${temporary}"

check_ext4_partition() {
    local number="$1" label="$2" record start end size extracted status
    record="$(parted -ms "${temporary}" unit B print | awk -F: -v n="${number}" '$1 == n { print $2 ":" $3; exit }')"
    [ -n "${record}" ] || { echo "Missing partition ${number} (${label})" >&2; return 1; }
    start="${record%%:*}"
    end="${record##*:}"
    start="${start%B}"
    end="${end%B}"
    size=$((end - start + 1))
    extracted="${workspace}/${label}.ext4"

    # Extract one partition at a time so e2fsck can inspect it without writes.
    dd if="${temporary}" of="${extracted}" iflag=skip_bytes,count_bytes \
        skip="${start}" count="${size}" status=none
    echo "${label} ext4 superblock:"
    "${DUMPE2FS}" -h "${extracted}" 2>/dev/null | \
        grep -E '^Filesystem (features|state):'
    echo "${label} ext4 summary:"
    "${TUNE2FS}" -l "${extracted}" 2>/dev/null | \
        grep -E '^Filesystem (features|state):'
    echo "Checking ${label} with e2fsck -fn"
    if "${E2FSCK}" -fn "${extracted}"; then
        return 0
    else
        status=$?
        if [ "${status}" -eq 12 ]; then
            echo "${label}: selected e2fsprogs cannot inspect this filesystem" >&2
            return 2
        fi
        echo "${label}: e2fsck reported filesystem errors (status ${status})" >&2
        return "${status}"
    fi
}

check_ext4_partition 2 rootfs
check_ext4_partition 3 platform
