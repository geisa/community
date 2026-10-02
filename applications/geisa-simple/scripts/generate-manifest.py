#!/usr/bin/env python3
# File: scripts/generate-manifest.py
# Project: geisa-simple
# Purpose: Render the pre-signing GEISA manifest from its source template.
# Copyright 2026 PragSol Consulting LLC.
# SPDX-License-Identifier: Apache-2.0

"""Render the geisa-simple pre-signing manifest from its source template."""

import json
import os
import re
import sys
from pathlib import Path


VERSION_RE = re.compile(r"[0-9]+\.[0-9]+\.[0-9]+", re.ASCII)
INTEGER_RE = re.compile(r"[0-9]+", re.ASCII)
PLACEHOLDER_RE = re.compile(r"__[A-Z0-9_]+__", re.ASCII)
INPUTS = (
    "TOOLCHAIN_ID",
    "TOOLCHAIN_VERSION",
    "IMAGE_NAME",
    "IMAGE_SIZE",
    "UNCOMPRESSED_SIZE",
)
SIGNING_PLACEHOLDERS = frozenset(
    {
        "__ARTIFACT_SIGNATURE_LENGTH__",
        "__ARTIFACT_SIGNATURE_HEX__",
        "__ARTIFACT_SIGNATURE_REFERENCE__",
        "__MANIFEST_SIGNATURE_LENGTH__",
        "__MANIFEST_SIGNATURE_HEX__",
        "__MANIFEST_SIGNATURE_REFERENCE__",
    }
)


def fail(message):
    print(f"manifest: error: {message}", file=sys.stderr)
    return 2


def find_placeholders(value):
    if isinstance(value, dict):
        for key, item in value.items():
            yield from find_placeholders(key)
            yield from find_placeholders(item)
    elif isinstance(value, list):
        for item in value:
            yield from find_placeholders(item)
    elif isinstance(value, str):
        yield from PLACEHOLDER_RE.findall(value)


def main():
    if len(sys.argv) != 3:
        return fail("usage: generate-manifest.py TEMPLATE OUTPUT")

    values = {name: os.environ.get(name, "") for name in INPUTS}
    missing = [name for name, value in values.items() if not value.strip()]
    if missing:
        for name in missing:
            print(f"manifest: error: {name} is required", file=sys.stderr)
        return 2

    errors = []
    for name in ("TOOLCHAIN_ID", "IMAGE_NAME"):
        if not 4 <= len(values[name]) <= 256:
            errors.append(f"{name} must be 4-256 characters (v0.9.0 schema)")
    if VERSION_RE.fullmatch(values["TOOLCHAIN_VERSION"]) is None:
        errors.append("TOOLCHAIN_VERSION must match X.Y.Z")

    sizes = {}
    for name in ("IMAGE_SIZE", "UNCOMPRESSED_SIZE"):
        value = values[name]
        if INTEGER_RE.fullmatch(value) is None:
            errors.append(f"{name} must be a positive integer")
            continue
        try:
            sizes[name] = int(value)
        except ValueError:
            errors.append(f"{name} must be a positive integer")
            continue
        if sizes[name] <= 0:
            errors.append(f"{name} must be a positive integer")

    if errors:
        for message in errors:
            print(f"manifest: error: {message}", file=sys.stderr)
        return 2

    template_path = Path(sys.argv[1])
    output_path = Path(sys.argv[2])
    try:
        document = json.loads(template_path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return fail(f"cannot read JSON template {template_path}: {error}")

    try:
        manifest = document["geisa-application-manifest"]["manifest"]
        artifacts = manifest["artifacts"]
        if len(artifacts) != 1:
            return fail("v0.9.0 template must contain exactly one artifact")
        compatibility = manifest["compatibility"]
        artifact = artifacts[0]
    except (KeyError, IndexError, TypeError):
        return fail("template does not match the GEISA manifest structure")

    compatibility["toolchain-id"] = values["TOOLCHAIN_ID"]
    compatibility["toolchain-version"] = values["TOOLCHAIN_VERSION"]
    artifact["image-name"] = values["IMAGE_NAME"]
    artifact["image-size"] = sizes["IMAGE_SIZE"]
    artifact["uncompressed-size"] = sizes["UNCOMPRESSED_SIZE"]

    unresolved = sorted(set(find_placeholders(document)) - SIGNING_PLACEHOLDERS)
    if unresolved:
        return fail(
            "unresolved non-signing placeholders: " + ", ".join(unresolved)
        )

    try:
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_text(
            json.dumps(document, indent=2) + "\n", encoding="utf-8"
        )
    except OSError as error:
        return fail(f"cannot write generated manifest {output_path}: {error}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
