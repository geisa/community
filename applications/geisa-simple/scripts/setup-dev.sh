#!/bin/sh
# File: scripts/setup-dev.sh
# Project: geisa-simple
# Purpose: Set up the pinned Specification schemas, nanopb checkout, and Python
# environment.
# Usage: ./scripts/setup-dev.sh; GEISA_SPECIFICATION_REPO,
# GEISA_SPECIFICATION_REF, and GEISA_SPECIFICATION_DIR override the schema input.
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
SPECIFICATION_REPO=${GEISA_SPECIFICATION_REPO:-https://github.com/geisa/specification.git}
SPECIFICATION_REF=${GEISA_SPECIFICATION_REF:-schemas-v0.9.0}
SPECIFICATION_DIR=${GEISA_SPECIFICATION_DIR:-$DEPS_DIR/geisa-specification}
NANOPB_DIR=$DEPS_DIR/nanopb
NANOPB_REF=${NANOPB_VERSION:-0.4.9.1}
VENV_DIR=${VENV_DIR:-$ROOT_DIR/.venv}

mkdir -p "$DEPS_DIR"

if [ -z "${GEISA_SPECIFICATION_DIR:-}" ]; then
    if [ ! -d "$SPECIFICATION_DIR/.git" ]; then
        git clone "$SPECIFICATION_REPO" "$SPECIFICATION_DIR"
    fi
    git -C "$SPECIFICATION_DIR" fetch --tags origin
    if ! git -C "$SPECIFICATION_DIR" cat-file -e \
            "$SPECIFICATION_REF^{commit}" 2>/dev/null; then
        git -C "$SPECIFICATION_DIR" fetch origin "$SPECIFICATION_REF"
    fi
    git -C "$SPECIFICATION_DIR" checkout --quiet "$SPECIFICATION_REF"
else
    test -d "$SPECIFICATION_DIR" || {
        echo "GEISA_SPECIFICATION_DIR does not exist: $SPECIFICATION_DIR" >&2
        exit 1
    }
fi

test -f "$SPECIFICATION_DIR/geisa-status.proto" || {
    echo "Selected Specification ref does not expose schemas at checkout root: $SPECIFICATION_REF" >&2
    exit 1
}
test -d "$SPECIFICATION_DIR/nanopb_options" || {
    echo "Selected Specification ref is missing nanopb_options/: $SPECIFICATION_REF" >&2
    exit 1
}

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

SPECIFICATION_COMMIT=$(git -C "$SPECIFICATION_DIR" rev-parse --short HEAD \
    2>/dev/null || echo local)
echo "GEISA Specification: $SPECIFICATION_DIR ($SPECIFICATION_COMMIT)"
echo "nanopb: $NANOPB_DIR ($(git -C "$NANOPB_DIR" rev-parse --short HEAD))"
echo "Next: make"
