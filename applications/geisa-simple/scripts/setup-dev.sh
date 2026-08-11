#!/bin/sh
# File: scripts/setup-dev.sh
# Project: geisa-simple
# Purpose: Set up the pinned schemas, nanopb checkout, and Python environment.
# Usage: ./scripts/setup-dev.sh; GEISA_SCHEMAS_REF and GEISA_SCHEMAS_DIR
# override the schema input.
#
# Copyright 2026 PragSol Consulting LLC.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

set -eu

ROOT_DIR=$(cd -- "$(dirname "$0")/.." && pwd)
DEPS_DIR=${DEPS_DIR:-$ROOT_DIR/.deps}
SCHEMAS_DIR=${GEISA_SCHEMAS_DIR:-$DEPS_DIR/geisa-schemas}
SCHEMAS_REF=${GEISA_SCHEMAS_REF:-v0.9.0}
NANOPB_DIR=$DEPS_DIR/nanopb
NANOPB_REF=${NANOPB_VERSION:-0.4.9.1}
VENV_DIR=${VENV_DIR:-$ROOT_DIR/.venv}

mkdir -p "$DEPS_DIR"

if [ -z "${GEISA_SCHEMAS_DIR:-}" ]; then
    if [ ! -d "$SCHEMAS_DIR/.git" ]; then
        git clone https://github.com/geisa/schemas.git "$SCHEMAS_DIR"
    fi
    git -C "$SCHEMAS_DIR" fetch --tags origin
    if ! git -C "$SCHEMAS_DIR" cat-file -e \
            "$SCHEMAS_REF^{commit}" 2>/dev/null; then
        git -C "$SCHEMAS_DIR" fetch origin "$SCHEMAS_REF"
    fi
    git -C "$SCHEMAS_DIR" checkout --quiet "$SCHEMAS_REF"
else
    test -d "$SCHEMAS_DIR" || {
        echo "GEISA_SCHEMAS_DIR does not exist: $SCHEMAS_DIR" >&2
        exit 1
    }
fi

if [ ! -d "$NANOPB_DIR/.git" ]; then
    git clone https://github.com/nanopb/nanopb.git "$NANOPB_DIR"
fi
git -C "$NANOPB_DIR" fetch --tags origin
git -C "$NANOPB_DIR" checkout --quiet "$NANOPB_REF"

if [ ! -x "$VENV_DIR/bin/python" ]; then
    python3 -m venv "$VENV_DIR"
fi
"$VENV_DIR/bin/python" -m pip install --upgrade pip
"$VENV_DIR/bin/python" -m pip install "nanopb==$NANOPB_REF"

SCHEMAS_COMMIT=$(git -C "$SCHEMAS_DIR" rev-parse --short HEAD \
    2>/dev/null || echo local)
echo "GEISA schemas: $SCHEMAS_DIR ($SCHEMAS_COMMIT)"
echo "nanopb: $NANOPB_DIR ($(git -C "$NANOPB_DIR" rev-parse --short HEAD))"
echo "Next: make"
