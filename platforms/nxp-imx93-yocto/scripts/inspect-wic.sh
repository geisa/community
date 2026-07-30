#!/usr/bin/env bash
set -euo pipefail

platform_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
deploy_dir="${platform_root}/build-development/tmp/deploy/images/geisa-imx93"
artifact="${1:-$(find "${deploy_dir}" -maxdepth 1 -type f -name "geisa-dev-image-geisa-imx93.rootfs-*.wic.zst" -printf "%T@ %p\n" | sort -nr | head -1 | cut -d" " -f2-)}"
[ -f "${artifact}" ] || { echo "WIC artifact not found: ${artifact}" >&2; exit 1; }
sha256sum "${artifact}"
zstd -l "${artifact}"
temporary="$(mktemp "${TMPDIR:-/tmp}/geisa-wic.XXXXXX")"
workspace="$(mktemp -d "${TMPDIR:-/tmp}/geisa-wic-check.XXXXXX")"
trap 'rm -f "${temporary}"; rm -rf "${workspace}"' EXIT
zstd -dc "${artifact}" > "${temporary}"
parted -ms "${temporary}" unit B print
fdisk -l "${temporary}"

check_ext4_partition() {
    local number="$1" label="$2" record start end size extracted
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
    echo "Checking ${label} with e2fsck -fn"
    e2fsck -fn "${extracted}"
}

check_ext4_partition 2 rootfs
check_ext4_partition 3 platform
