#!/usr/bin/env bash
set -euo pipefail

platform_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
stage_dir="${1:-}"
if [ -z "${stage_dir}" ]; then stage_dir="$(find "${platform_root}/release-staging" -mindepth 1 -maxdepth 1 -type d -printf "%T@ %p\n" | sort -nr | head -1 | cut -d" " -f2-)"; fi
[ -d "${stage_dir}" ] || { echo "Release staging directory not found." >&2; exit 1; }
for pattern in '*.wic.zst' '*.wic.bmap' '*.manifest' '*.image-manifest.json' '*.spdx.tar.zst'; do compgen -G "${stage_dir}/${pattern}" >/dev/null || { echo "Missing ${pattern}" >&2; exit 1; }; done
(cd "${stage_dir}" && sha256sum -c SHA256SUMS)
artifact="$(find "${stage_dir}" -maxdepth 1 -type f -name '*.wic.zst' -printf '%p\n' | sort | head -1)"
[ -n "${artifact}" ] || { echo "Missing WIC artifact for filesystem validation" >&2; exit 1; }
"${platform_root}/scripts/inspect-wic.sh" "${artifact}"
