#!/usr/bin/env python3
"""Assemble and verify the reproducible NXP Ethos-U/TFLite profile."""

from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import shutil
import stat
import zipfile

PROFILE = "nxp-ethosu-tflite"
PROFILE_VERSION = "2"
PYAV_WHEEL = "av-18.0.0-cp311-abi3-manylinux_2_28_aarch64.whl"
PYAV_SHA256 = "4d683b7747a0ba9222b8a5f81e41db5f796e7f64473454ec4fe2548e083c2fa0"
PYAV_TAG = "cp311-abi3-manylinux_2_28_aarch64"
PYAV_URL = (
    "https://files.pythonhosted.org/packages/84/74/"
    "6732f17b96dc23fd23b876b2805435855abdc8a3b397142be4e581165de8/"
    "av-18.0.0-cp311-abi3-manylinux_2_28_aarch64.whl"
)

NATIVE_SOURCES = {
    "lib/libethosu_delegate.so": "usr/lib/libethosu_delegate.so",
    "lib/libethosu.so.1.0.0": "usr/lib/libethosu.so.1.0.0",
}
NUMPY_EXCLUDES = {
    "_pyinstaller",
    "distutils",
    "doc",
    "f2py",
    "include",
    "test",
    "tests",
    "__pycache__",
}
PROHIBITED_PARTS = {
    "build",
    "doc",
    "docs",
    "examples",
    "headers",
    "include",
    "pip",
    "sboms",
    "setuptools",
    "test",
    "tests",
    "vela",
    "wheel",
}
PROHIBITED_SUFFIXES = (".a", ".c", ".h", ".pxd", ".pyx", ".whl")


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def tensorflow_lite_library(sysroot: Path) -> tuple[str, str]:
    libraries = sorted(
        path
        for path in (sysroot / "usr/lib").glob("libtensorflow-lite.so.*")
        if path.is_file()
    )
    if len(libraries) != 1:
        raise ValueError(f"expected one TensorFlow Lite runtime library, found: {libraries}")
    name = libraries[0].name
    return name, f"usr/lib/{name}"


def python_site(sysroot: Path) -> Path:
    sites = sorted(
        path
        for path in (sysroot / "usr/lib").glob("python*/site-packages")
        if path.is_dir() and (path / "tflite_runtime").is_dir()
    )
    if len(sites) != 1:
        raise ValueError(f"expected one Python runtime site-packages tree, found: {sites}")
    return sites[0].relative_to(sysroot)


def ensure_safe_relative(name: str) -> Path:
    path = Path(name)
    if not name or path.is_absolute() or ".." in path.parts:
        raise ValueError(f"unsafe path: {name}")
    return path


def copy_file(source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)


def copy_tree(source: Path, destination: Path, excluded_parts: set[str]) -> None:
    if not source.is_dir():
        raise ValueError(f"missing source tree: {source}")
    for path in sorted(source.rglob("*")):
        relative = path.relative_to(source)
        if excluded_parts.intersection(part.lower() for part in relative.parts):
            continue
        if path.is_symlink():
            target = path.readlink()
            if target.is_absolute() or ".." in target.parts:
                raise ValueError(f"unsafe source symlink: {path}")
            output = destination / relative
            output.parent.mkdir(parents=True, exist_ok=True)
            output.symlink_to(target)
        elif path.is_file():
            if path.name == "setup.py" or path.suffix in PROHIBITED_SUFFIXES:
                continue
            copy_file(path, destination / relative)


def records(root: Path) -> list[dict[str, object]]:
    result: list[dict[str, object]] = []
    for path in sorted(root.rglob("*")):
        relative = str(path.relative_to(root))
        mode = stat.S_IMODE(path.lstat().st_mode)
        if path.is_symlink():
            result.append(
                {
                    "mode": mode,
                    "path": relative,
                    "target": path.readlink().as_posix(),
                    "type": "symlink",
                }
            )
        elif path.is_file():
            result.append(
                {
                    "mode": mode,
                    "path": relative,
                    "sha256": sha256(path),
                    "size-bytes": path.stat().st_size,
                    "type": "file",
                }
            )
    return result


def tree_sha256(items: list[dict[str, object]]) -> str:
    encoded = json.dumps(items, sort_keys=True, separators=(",", ":")).encode("utf-8")
    return hashlib.sha256(encoded).hexdigest()


def extract_pyav(wheel: Path, destination: Path) -> dict[str, object]:
    if wheel.name != PYAV_WHEEL or sha256(wheel) != PYAV_SHA256:
        raise ValueError("PyAV wheel identity mismatch")
    with zipfile.ZipFile(wheel) as archive:
        names = archive.namelist()
        if any(Path(name).is_absolute() or ".." in Path(name).parts for name in names):
            raise ValueError("unsafe PyAV wheel member")
        metadata_name = "av-18.0.0.dist-info/METADATA"
        wheel_name = "av-18.0.0.dist-info/WHEEL"
        if metadata_name not in names or wheel_name not in names:
            raise ValueError("PyAV wheel metadata is missing")
        metadata = archive.read(metadata_name).decode("utf-8", errors="strict")
        tags = [line.removeprefix("Tag: ") for line in archive.read(wheel_name).decode().splitlines() if line.startswith("Tag: ")]
        if "Name: av" not in metadata or "Version: 18.0.0" not in metadata or tags != [PYAV_TAG]:
            raise ValueError("PyAV wheel metadata drift")
        for member in archive.infolist():
            name = member.filename
            if member.is_dir():
                continue
            path = ensure_safe_relative(name)
            parts = path.parts
            allowed = (
                parts[0] in {"av", "av.libs"}
                or (parts[0] == "av-18.0.0.dist-info" and (name.endswith("/METADATA") or "/licenses/" in name))
            )
            if not allowed:
                continue
            if path.suffix in PROHIBITED_SUFFIXES or PROHIBITED_PARTS.intersection(part.lower() for part in parts):
                continue
            output = destination / path
            output.parent.mkdir(parents=True, exist_ok=True)
            with archive.open(member) as source, output.open("wb") as target:
                shutil.copyfileobj(source, target)
    return {"filename": PYAV_WHEEL, "sha256": PYAV_SHA256, "url": PYAV_URL, "tag": PYAV_TAG}


def assert_profile_safe(root: Path) -> None:
    for record in records(root):
        relative = str(record["path"])
        path = ensure_safe_relative(relative)
        if PROHIBITED_PARTS.intersection(part.lower() for part in path.parts):
            raise ValueError(f"prohibited profile content: {relative}")
        if relative.endswith(PROHIBITED_SUFFIXES) or relative.endswith((".mp4", ".tflite")):
            raise ValueError(f"prohibited profile content: {relative}")
        if record["type"] == "file" and int(record["mode"]) & (stat.S_ISUID | stat.S_ISGID | stat.S_IWOTH):
            raise ValueError(f"unsafe profile permissions: {relative}")
        if record["type"] == "symlink":
            target = Path(str(record["target"]))
            if target.is_absolute() or ".." in target.parts:
                raise ValueError(f"unsafe profile symlink: {relative}")


def assemble(sysroot: Path, wheel: Path, output: Path, source_revision: str) -> None:
    if output.exists():
        raise ValueError(f"refusing to replace profile output: {output}")
    output.mkdir(parents=True)
    tensorflow_library, tensorflow_source = tensorflow_lite_library(sysroot)
    native_sources = {
        **NATIVE_SOURCES,
        f"lib/{tensorflow_library}": tensorflow_source,
    }
    links = {
        "lib/libethosu.so": "libethosu.so.1.0.0",
        "lib/libethosu.so.1": "libethosu.so.1.0.0",
        "lib/libtensorflow-lite.so": tensorflow_library,
    }
    site = python_site(sysroot)
    for destination, source in native_sources.items():
        candidate = sysroot / source
        if not candidate.is_file():
            raise ValueError(f"missing declared Yocto runtime input: {candidate}")
        copy_file(candidate, output / destination)
    copy_tree(sysroot / site / "tflite_runtime", output / "python" / "tflite_runtime", {"test", "tests", "__pycache__"})
    copy_tree(sysroot / site / "numpy", output / "python" / "numpy", NUMPY_EXCLUDES)
    for destination, target in links.items():
        link = output / destination
        link.parent.mkdir(parents=True, exist_ok=True)
        link.symlink_to(target)
    pyav = extract_pyav(wheel, output / "python")
    assert_profile_safe(output)
    file_records = records(output)
    document = {
        "schema-version": "geisa.nxp-ethosu-tflite-profile.v1",
        "profile": PROFILE,
        "profile-version": PROFILE_VERSION,
        "architecture": "aarch64",
        "python-soabi": "cpython-312-aarch64-linux-gnu",
        "loader": {"pythonpath": "python", "library-path": ["lib", "python/av.libs"]},
        "provenance": {
            "generated-by": "meta-geisa-community",
            "source-revision": source_revision,
            "yocto-inputs": [
                "tensorflow-lite",
                "tensorflow-lite-ethosu-delegate",
                "ethos-u-driver-stack",
                "python3-numpy",
            ],
            "pyav-wheel": pyav,
        },
        "files": file_records,
        "tree-sha256": tree_sha256(file_records),
    }
    (output / "profile.json").write_text(json.dumps(document, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    for path in sorted(output.rglob("*"), reverse=True):
        if not path.is_symlink():
            path.chmod(path.stat().st_mode & ~(stat.S_IWGRP | stat.S_IWOTH))


def verify(root: Path) -> None:
    document = json.loads((root / "profile.json").read_text(encoding="utf-8"))
    if document.get("profile") != PROFILE or str(document.get("profile-version")) != PROFILE_VERSION:
        raise ValueError("profile identity mismatch")
    if document.get("architecture") != "aarch64" or document.get("python-soabi") != "cpython-312-aarch64-linux-gnu":
        raise ValueError("profile architecture or Python ABI mismatch")
    if document.get("loader") != {"pythonpath": "python", "library-path": ["lib", "python/av.libs"]}:
        raise ValueError("profile loader configuration mismatch")
    expected = [item for item in document.get("files", []) if item.get("path") != "profile.json"]
    actual = [item for item in records(root) if item.get("path") != "profile.json"]
    if expected != actual or document.get("tree-sha256") != tree_sha256(actual):
        raise ValueError("profile inventory mismatch")
    assert_profile_safe(root)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("command", choices=("assemble", "verify"))
    parser.add_argument("--sysroot", type=Path)
    parser.add_argument("--wheel", type=Path)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--source-revision", default="unknown")
    args = parser.parse_args()
    if args.command == "assemble":
        if not args.sysroot or not args.wheel:
            raise SystemExit("assemble requires --sysroot and --wheel")
        assemble(args.sysroot, args.wheel, args.root, args.source_revision)
    else:
        verify(args.root)


if __name__ == "__main__":
    main()
