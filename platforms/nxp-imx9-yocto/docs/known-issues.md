# Known Issues

## Older e2fsprogs cannot inspect generated filesystems

The WIC root and `/platform` filesystems use the ext4 `orphan_file` feature.
e2fsprogs versions older than 1.47 may report this as `FEATURE_C12` and
exit with status 12 during offline inspection. That result does *not* imply
that the filesystem is corrupt.

Use e2fsprogs 1.47 or newer for offline checks. `scripts/inspect-wic.sh`
selects the build's compatible native tools when available, or accepts an
explicit tool directory through `GEISA_E2FSPROGS_BIN_DIR`.
