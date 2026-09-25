#!/bin/sh
#
# setup-dev.sh
#
# Fetch the pinned GEISA schemas and nanopb sources, then install the matching
# nanopb generator into the application's local virtual environment.
#
# Copyright 2026 PragSol Consulting LLC.
# SPDX-License-Identifier: Apache-2.0

set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
DEPS_DIR=${DEPS_DIR:-$ROOT_DIR/.deps}
SCHEMAS_REPO=${GEISA_SCHEMAS_REPO:-https://github.com/geisa/schemas.git}
SCHEMAS_REF=${GEISA_SCHEMAS_REF:-v0.9.0}
SCHEMAS_DIR=${GEISA_SCHEMAS_DIR:-$DEPS_DIR/geisa-schemas}
NANOPB_DIR=$DEPS_DIR/nanopb
NANOPB_REF=${NANOPB_VERSION:-0.4.9.1}
VENV_DIR=${VENV_DIR:-$ROOT_DIR/.venv}

mkdir -p "$DEPS_DIR"
if [ -z "${GEISA_SCHEMAS_DIR:-}" ]; then
    if [ ! -d "$SCHEMAS_DIR/.git" ]; then
        git clone "$SCHEMAS_REPO" "$SCHEMAS_DIR"
    fi
    git -C "$SCHEMAS_DIR" fetch --tags origin
    git -C "$SCHEMAS_DIR" checkout --quiet "$SCHEMAS_REF"
else
    test -d "$SCHEMAS_DIR" || {
        echo "GEISA_SCHEMAS_DIR does not exist: $SCHEMAS_DIR" >&2
        exit 2
    }
fi

for schema in geisa-status.proto conn-status.proto app-message.proto manifest.proto discovery.proto sensor.proto waveform.proto; do
    test -f "$SCHEMAS_DIR/$schema" || {
        echo "Missing GEISA schema: $SCHEMAS_DIR/$schema" >&2
        exit 2
    }
done

test -d "$SCHEMAS_DIR/nanopb_options" || {
    echo "Missing nanopb_options/: $SCHEMAS_DIR" >&2
    exit 2
}

if [ ! -d "$NANOPB_DIR/.git" ]; then
    git clone https://github.com/nanopb/nanopb.git "$NANOPB_DIR"
fi

git -C "$NANOPB_DIR" fetch --tags origin
git -C "$NANOPB_DIR" checkout --quiet "$NANOPB_REF"

if [ ! -x "$VENV_DIR/bin/python" ]; then
    python3 -m venv "$VENV_DIR"
fi

"$VENV_DIR/bin/python" -m pip install "nanopb==$NANOPB_REF"
printf 'GEISA schemas: %s (%s)\nnanopb: %s\nNext: make test\n' \
    "$SCHEMAS_DIR" "$SCHEMAS_REF" "$NANOPB_REF"
