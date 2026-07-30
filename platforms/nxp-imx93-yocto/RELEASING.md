# Releasing the GEISA NXP i.MX93 Community Image

## Scope

This guide is for Community maintainers and contributors preparing an image
release. It is not required for an ordinary local build. The image is a
community-contributed development and validation image. It is not an officially
supported GEISA deliverable, a conformance or certification image, or a
production image.

## Source and Revision State

Build from a recorded commit or release tag and initialize the pinned
submodules. Do not publish unreviewed local changes. If a deliberately dirty
build is retained, record and disclose its dirty digest and confirm that the
image manifest reports the intended source state.

Before publication, ensure the Community source contains no local staging
paths, linked local repositories, build caches, or automatic NXP EULA
acceptance.

## Build Preparation

Review the source state, the NXP license material, and available disk capacity
before building. Reuse download and sstate caches where appropriate; there is
no need to destroy a validated cache solely to start a new build. Run focused
parse and source checks before the full build.

## Build and Artifact Selection

Run one normal consolidated build and select one exact build identifier. Do not
publish an ambiguous or stale deploy artifact, and do not silently substitute a
later rebuild after hardware acceptance.

Record the compressed and raw WIC SHA-256 values and sizes. Confirm that the
raw WIC fits the target media, and record the partition sizes and alignment.

## Offline WIC Inspection

Mount a temporary decompressed WIC copy read-only and confirm the expected
installed documentation and metadata:

    /etc/geisa/image-manifest.json
    /etc/geisa-image-release
    /usr/share/doc/geisa-image/README.md
    /usr/share/doc/geisa-image/NOTICE.md
    /usr/share/doc/geisa-image/LICENSES/Apache-2.0.txt
    /usr/share/doc/geisa-image/packages.manifest

Verify the README, NOTICE, and Apache license against the corresponding files
in the release source.
Verify the installed package manifest against the same-build deploy manifest
and the image manifest against its external copy. Record the app-base and
execution-profile hashes, required paths and ownership, and the expected boot,
root, and platform partition layout.

Confirm that no personal accounts, credentials, active applications, fixtures,
application-management source, validation logs, legacy mock/conformance residue, or host-
specific build paths are packaged in the image metadata or filesystem.

## Hardware Validation

1. Flash the exact candidate to SD.
2. Validate SD boot identity.
3. Provision operator accounts, SSH keys, repository access, and other
   site-specific settings after boot rather than including them in the image.
4. Test network, DNS, time, sudo, MQTT, LXC, cgroups, and Ethos-U.
5. Validate a basic managed application.
6. Validate an Ethos-U workload when accelerator support is part of the release
   claims.
7. Verify resource controls and cleanup.
8. Run a soak and preserve its evidence.
9. Migrate the same artifact to eMMC only after SD acceptance.

A rebuild after SD acceptance requires another SD acceptance cycle.

## Release Artifact Collection

Collect one exact build identifier with no ambiguous matches. A release should
include:

    geisa-dev-image-geisa-imx93.rootfs-<release>.wic.zst
    geisa-dev-image-geisa-imx93.rootfs-<release>.wic.zst.sha256
    geisa-dev-image-geisa-imx93.rootfs-<release>.manifest
    geisa-dev-image-geisa-imx93.rootfs-<release>.spdx.tar.zst
    geisa-dev-image-geisa-imx93.rootfs-<release>.image-manifest.json
    README.md
    NOTICE.md
    Apache-2.0.txt

The release collector must verify required files and hashes, refuse an existing
staging destination, and never upload, tag, publish, or create a release
automatically.

## Licensing and Redistribution Review

`ACCEPT_FSL_EULA=1` allows EULA-gated build handling; it is not blanket
redistribution permission. Review the exact package manifest, SPDX archive, and
license material for the selected artifact. Inspect NXP firmware and proprietary
BSP components, and verify whether optional firmware is included.

Do not publish an image containing the local NXP Ethos-U/TFLite profile archive
until its origin, component inventory, and redistribution permission are
recorded and reviewed. The initial source contribution intentionally excludes
that archive.

Apache-2.0 applies only to Community-owned metadata, scripts, and documentation
explicitly released under it. Record unresolved redistribution questions before
publication; do not make legal conclusions not supported by the generated
artifacts.

## Publication Checklist

- [ ] Source revision or tag and submodule revisions recorded
- [ ] Clean or dirty source state disclosed
- [ ] No local source links, build paths, credentials, or automatic EULA
      acceptance remain
- [ ] Build identifier, WIC hashes, sizes, and media fit recorded
- [ ] Package manifest, image manifest, SPDX archive, and license material checked
- [ ] Offline `e2fsck -fn` checks passed with an e2fsprogs version that supports
      every enabled ext4 filesystem feature
- [ ] Source documentation byte matches verified
- [ ] SD validation completed, plus eMMC validation where claimed
- [ ] Known limitations and release notes prepared
- [ ] Release files uploaded and post-upload checksums verified

## Post-Publication Verification

Verify downloaded artifact hashes, release links, and filenames. Confirm that
issue links point to <https://github.com/geisa/community/issues>, README
rendering is correct, and the release tag and artifact identity are recorded.
Preserve the validation evidence.
