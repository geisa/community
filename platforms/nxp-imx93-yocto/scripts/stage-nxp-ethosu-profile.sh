#!/usr/bin/env bash
set -euo pipefail

# Stage a locally obtained, non-redistributed profile input for a private build.
platform_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
destination="${platform_root}/sources/meta-geisa-community/recipes-core/geisa/files"
archive="${1:-}"
member_list="$(mktemp "${TMPDIR:-/tmp}/geisa-profile-members.XXXXXX")"
normalized_list="$(mktemp "${TMPDIR:-/tmp}/geisa-profile-normalized.XXXXXX")"
trap 'rm -f "${member_list}" "${normalized_list}"' EXIT

usage() {
    echo "usage: $0 ARCHIVE --origin DESCRIPTION" >&2
    exit 2
}

[ -n "${archive}" ] && [ -f "${archive}" ] || usage
shift
[ "${1:-}" = "--origin" ] && [ -n "${2:-}" ] || usage
origin="$2"
[ "${origin#*$'\n'}" = "${origin}" ] || {
    echo "origin must be a single line" >&2
    exit 1
}

# Validate names before copying. The source archive is never extracted here.
tar -tzf "${archive}" > "${member_list}" || {
    echo "cannot list archive members" >&2
    exit 1
}

while IFS= read -r member || [ -n "${member}" ]; do
    path="${member%/}"
    while [ "${path#./}" != "${path}" ]; do path="${path#./}"; done
    [ -n "${path}" ] && [ "${path}" != "." ] || continue
    case "${path}" in
        /*|.|..|../*|*/../*|*/..|*'//'*)
            echo "unsafe archive member: ${member}" >&2
            exit 1
            ;;
    esac
    printf '%s\n' "${path}" >> "${normalized_list}"
done < "${member_list}"

if grep -Fxq 'nxp-ethosu-tflite/profile.json' "${normalized_list}"; then
    layout="directory-prefixed"
    if grep -Evqx 'nxp-ethosu-tflite(/.*)?' "${normalized_list}"; then
        echo "archive contains members outside nxp-ethosu-tflite/" >&2
        exit 1
    fi
elif grep -Fxq 'profile.json' "${normalized_list}"; then
    layout="root-level"
    if grep -Eq '^nxp-ethosu-tflite(/|$)' "${normalized_list}"; then
        echo "archive mixes root-level and nxp-ethosu-tflite/ members" >&2
        exit 1
    fi
else
    echo "archive is missing profile.json" >&2
    exit 1
fi

install -d -m 0755 "${destination}"
install -m 0644 "${archive}" "${destination}/nxp-ethosu-tflite.tar.gz"
{
    printf 'origin: %s\n' "${origin}"
    printf 'sha256: %s\n' "$(sha256sum "${destination}/nxp-ethosu-tflite.tar.gz" | awk '{print $1}')"
    printf 'layout: %s\n' "${layout}"
} > "${destination}/nxp-ethosu-tflite.local-origin.txt"

echo "Staged local profile input; both output files are ignored by Git."
