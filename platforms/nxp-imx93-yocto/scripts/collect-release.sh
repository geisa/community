#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 PragSol Consulting, LLC
# Website: https://www.pragsolconsulting.com/
#
set -euo pipefail

platform_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
deploy_dir="${platform_root}/build-development/tmp/deploy/images/geisa-imx93"
build_id="${1:?usage: $0 <build-id>}"
case "${build_id}" in
    *[!0-9]*|'') echo "Build identifier must be a timestamp such as 20260729045937." >&2; exit 2 ;;
esac
artifact="${deploy_dir}/geisa-dev-image-geisa-imx93.rootfs-${build_id}.wic.zst"
[ -f "${artifact}" ] || { echo "No development WIC artifact found for ${build_id}." >&2; exit 1; }

base="${artifact%.wic.zst}"
release_id="$(basename "${base%.rootfs}")"
stage_dir="${platform_root}/release-staging/${release_id}"
[ ! -e "${stage_dir}" ] || { echo "Release destination already exists: ${stage_dir}" >&2; exit 1; }
install -d -m 0755 "${stage_dir}"

for suffix in wic.zst wic.bmap manifest image-manifest.json spdx.tar.zst; do
    source="${base}.${suffix}"
    [ -f "${source}" ] || { echo "Missing release input: ${source}" >&2; exit 1; }
    install -m 0644 "${source}" "${stage_dir}/"
done

(
    cd "${stage_dir}"
    sha256sum *.wic.zst *.wic.bmap *.manifest *.image-manifest.json *.spdx.tar.zst > SHA256SUMS
)
printf "%s\n" "${stage_dir}"
