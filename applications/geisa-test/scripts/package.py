#!/usr/bin/env python3
# File: scripts/package.py
# Project: geisa-test
# Purpose: Build an unsigned appoverlay archive and its pre-signing manifest.
# Copyright 2026 PragSol Consulting LLC.
# SPDX-License-Identifier: Apache-2.0

"""Create the one-file runtime appoverlay expected by geisa-test's manifest."""

import argparse
import gzip
import json
import os
import shutil
import stat
import subprocess
import sys
import tarfile
import tempfile
from pathlib import Path


def create_tarball(source, tar_path, member_name):
    source_stat = source.lstat()
    if not stat.S_ISREG(source_stat.st_mode) or not os.access(source, os.X_OK):
        raise ValueError(f"runtime executable is not an executable regular file: {source}")
    if not member_name or Path(member_name).name != member_name:
        raise ValueError("archive member name must be a single root-level filename")

    entry = tarfile.TarInfo(member_name)
    entry.size = source_stat.st_size
    entry.mode = 0o755
    entry.uid = 0
    entry.gid = 0
    entry.uname = ""
    entry.gname = ""
    entry.mtime = 0
    with tarfile.open(tar_path, "w", format=tarfile.USTAR_FORMAT) as archive:
        with source.open("rb") as executable:
            archive.addfile(entry, executable)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", required=True, type=Path)
    parser.add_argument("--member-name", required=True)
    parser.add_argument("--artifact", required=True, type=Path)
    parser.add_argument("--manifest-template", required=True, type=Path)
    parser.add_argument("--manifest-output", required=True, type=Path)
    parser.add_argument("--manifest-generator", required=True, type=Path)
    args = parser.parse_args()

    temporary_paths = []
    try:
        args.artifact.parent.mkdir(parents=True, exist_ok=True)
        args.manifest_output.parent.mkdir(parents=True, exist_ok=True)

        tar_fd, tar_name = tempfile.mkstemp(
            prefix=".package-", suffix=".tar", dir=args.artifact.parent
        )
        os.close(tar_fd)
        tar_path = Path(tar_name)
        temporary_paths.append(tar_path)

        artifact_fd, artifact_name = tempfile.mkstemp(
            prefix=".package-", suffix=".tgz", dir=args.artifact.parent
        )
        os.close(artifact_fd)
        temporary_artifact = Path(artifact_name)
        temporary_paths.append(temporary_artifact)

        manifest_fd, manifest_name = tempfile.mkstemp(
            prefix=".manifest-", suffix=".json", dir=args.manifest_output.parent
        )
        os.close(manifest_fd)
        temporary_manifest = Path(manifest_name)
        temporary_paths.append(temporary_manifest)

        create_tarball(args.executable, tar_path, args.member_name)
        with tar_path.open("rb") as tar_input, temporary_artifact.open("wb") as output:
            # Fixed gzip metadata keeps identical package inputs byte-for-byte stable.
            with gzip.GzipFile(filename="", fileobj=output, mode="wb", mtime=0) as zipped:
                shutil.copyfileobj(tar_input, zipped)

        env = os.environ.copy()
        env["IMAGE_NAME"] = args.artifact.name
        env["IMAGE_SIZE"] = str(temporary_artifact.stat().st_size)
        # The decompressed .tgz stream is the USTAR image measured by the schema.
        env["UNCOMPRESSED_SIZE"] = str(tar_path.stat().st_size)
        result = subprocess.run(
            [
                sys.executable,
                str(args.manifest_generator),
                str(args.manifest_template),
                str(temporary_manifest),
            ],
            env=env,
            check=False,
        )
        if result.returncode:
            return result.returncode

        # Do not publish a package unless its paired manifest is valid JSON.
        json.loads(temporary_manifest.read_text(encoding="utf-8"))
        os.replace(temporary_artifact, args.artifact)
        os.replace(temporary_manifest, args.manifest_output)
        print(f"Unsigned appoverlay: {args.artifact}")
        print(f"Pre-signing manifest: {args.manifest_output}")
        return 0
    except (OSError, ValueError, tarfile.TarError, json.JSONDecodeError) as error:
        print(f"package: error: {error}", file=sys.stderr)
        return 2
    finally:
        for path in temporary_paths:
            try:
                path.unlink()
            except FileNotFoundError:
                pass


if __name__ == "__main__":
    sys.exit(main())
