#!/usr/bin/env bash
set -euo pipefail

# Stage a locally obtained, non-redistributed profile input for a private build.
platform_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
destination="${platform_root}/sources/meta-geisa-community/recipes-core/geisa/files"
archive="${1:-}"

usage() {
    echo "usage: $0 ARCHIVE --origin DESCRIPTION" >&2
    exit 2
}

[ -n "${archive}" ] && [ -f "${archive}" ] || usage
shift
[ "${1:-}" = "--origin" ] && [ -n "${2:-}" ] || usage
origin="$2"

# The recipe expects one top-level directory and a profile descriptor.
tar -tzf "${archive}" | grep -qx 'nxp-ethosu-tflite/' || {
    echo "missing nxp-ethosu-tflite/ top-level directory" >&2
    exit 1
}
tar -tzf "${archive}" | grep -qx 'nxp-ethosu-tflite/profile.json' || {
    echo "missing nxp-ethosu-tflite/profile.json" >&2
    exit 1
}
if tar -tzf "${archive}" | grep -qv '^nxp-ethosu-tflite/'; then
    echo "archive contains paths outside nxp-ethosu-tflite/" >&2
    exit 1
fi

install -d -m 0755 "${destination}"
install -m 0644 "${archive}" "${destination}/nxp-ethosu-tflite.tar.gz"
{
    printf 'origin: %s\n' "${origin}"
    printf 'sha256: %s\n' "$(sha256sum "${destination}/nxp-ethosu-tflite.tar.gz" | awk '{print $1}')"
} > "${destination}/nxp-ethosu-tflite.local-origin.txt"

echo "Staged local profile input; both output files are ignored by Git."
