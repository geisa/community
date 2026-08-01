#!/bin/sh

set -eu

usage()
{
    cat <<'USAGE'
Usage: scripts/setup-sources.sh [--dry-run]

Initializes the external source repositories declared by this project-local
.gitmodules file and checks out the exact revisions recorded by the Community
repository.

Options:
  --dry-run   Show each dependency, URL, and recorded revision without cloning.
USAGE
}

dry_run=false

case "${1:-}" in
    "")
        ;;
    --dry-run)
        dry_run=true
        ;;
    -h|--help)
        usage
        exit 0
        ;;
    *)
        usage >&2
        exit 2
        ;;
esac

script_dir=$(
    CDPATH= cd -- "$(dirname -- "$0")" &&
    pwd
)

project_dir=$(
    CDPATH= cd -- "$script_dir/.." &&
    pwd
)

repo_root=$(git -C "$project_dir" rev-parse --show-toplevel)
project_rel=$(realpath --relative-to="$repo_root" "$project_dir")
modules_file="$project_dir/.gitmodules"

if [ ! -f "$modules_file" ]; then
    echo "Missing project-local .gitmodules: $modules_file" >&2
    exit 1
fi

git config -f "$modules_file" \
    --get-regexp '^submodule\..*\.path$' |
while read -r key relative_path; do
    name=${key#submodule.}
    name=${name%.path}

    url=$(git config -f "$modules_file" --get "submodule.$name.url")
    recorded_revision=$(
        git -C "$repo_root" rev-parse \
            "HEAD:$project_rel/$relative_path"
    )

    target="$project_dir/$relative_path"

    printf '%s\n' \
        "Dependency: $relative_path" \
        "  URL:      $url" \
        "  Revision: $recorded_revision"

    if $dry_run; then
        continue
    fi

    if [ -d "$target/.git" ] || [ -f "$target/.git" ]; then
        git -C "$target" remote set-url origin "$url"
    else
        if [ -e "$target" ]; then
            if [ -n "$(find "$target" -mindepth 1 -maxdepth 1 -print -quit)" ]; then
                echo "Refusing to replace non-empty path: $target" >&2
                exit 1
            fi
            rmdir "$target"
        fi

        git clone --no-checkout "$url" "$target"
    fi

    git -C "$target" fetch --tags --prune origin

    if ! git -C "$target" cat-file -e "$recorded_revision^{commit}" 2>/dev/null; then
        git -C "$target" fetch origin "$recorded_revision"
    fi

    git -C "$target" checkout --detach "$recorded_revision"
    git -C "$target" submodule update --init --recursive
done
