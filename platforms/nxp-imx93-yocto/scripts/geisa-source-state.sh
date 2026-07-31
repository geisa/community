#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 PragSol Consulting, LLC
# Website: https://www.pragsolconsulting.com/
#
set -euo pipefail
out="${1:?output file required}"
repo="$(cd "$(dirname "$0")/.." && pwd -P)"
if git -C "$repo" rev-parse --is-inside-work-tree >/dev/null 2>&1; then
    revision="$(git -C "$repo" rev-parse HEAD)"
    dirty=false
    if [ -n "$(git -C "$repo" status --porcelain=v1 --untracked-files=all)" ]; then
        dirty=true
        digest="$( {
            cd "$repo"
            git -C "$repo" diff --no-ext-diff --binary HEAD
            git -C "$repo" diff --cached --no-ext-diff --binary HEAD
            git -C "$repo" ls-files --others --exclude-standard -z | LC_ALL=C sort -z | xargs -0 -r sha256sum
            git -C "$repo" submodule status --recursive
        } | sha256sum | cut -d " " -f1)"
    else
        digest=clean
    fi
else
    revision=staging-uncommitted
    dirty=true
    digest="$(find "$repo/sources/meta-geisa-community" -type f -print0 | LC_ALL=C sort -z | xargs -0 sha256sum | sha256sum | cut -d " " -f1)"
fi
mkdir -p "$(dirname "$out")"
printf 'GEISA_SOURCE_REVISION = "%s"\nGEISA_SOURCE_DIRTY = "%s"\nGEISA_SOURCE_DIRTY_DIGEST = "%s"\n' "$revision" "$dirty" "$digest" > "$out"
