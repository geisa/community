# Release Process

This platform currently supports development-image releases for the
NXP FRDM-i.MX93 only.

Use the checklist below when preparing a release candidate.

## 1. Start from a clean source state

- Build from the reviewed source revision intended for release.
- Confirm the main repository and submodules are at the expected revisions.
- Confirm the source tree is clean before the release build.
- Record the source revision and submodule revisions alongside the release
  artifacts.

## 2. Run a normal build

- Run a normal development-image build.
- Identify the exact candidate WIC.
- Collect the matching checksums, manifests, SPDX archive, and image metadata.
- Confirm the generated `nxp-ethosu-tflite` profile records the declared Yocto
  inputs and the pinned PyAV wheel.

## 3. Run release validation

Run:

```sh
./scripts/validate-release.sh RELEASE_DIRECTORY
```

This checks the staged artifact set and performs read-only ext4 validation on
its WIC contents.

Do not release the image if the selected e2fsck cannot validate the filesystem
feature set or if it reports filesystem errors.

## 4. Complete offline acceptance checks

Confirm that the candidate root filesystem contains:

- the platform-mount generator
- the selected IW612 firmware path
  `/usr/lib/firmware/nxp/uartspi_n61x_v1.bin.se`
- runtime copies of:
  - `/usr/lib/firmware/regulatory.db`
  - `/usr/lib/firmware/regulatory.db.p7s`

Also confirm that the kernel image embeds the signed regulatory database and
signature required before the root filesystem becomes available.

## 5. Complete hardware acceptance checks

Validate the candidate on FRDM-i.MX93 hardware.

Confirm at minimum:

- root and `/platform` come from partitions 2 and 3 of the same MMC device
- early platform fsck is clean
- the boot completes without a regulatory-database warning or console messages
- Bluetooth controller discovery succeeds
- the expected development-tool set is present
- the selected kernel capabilities for this image are present, including:
  - `can-utils`
  - `iw`
  - `rfkill`
  - `nft`
  - `perf`
  - `bpftool`
  - checkpoint/restore prerequisites
  - socket diagnostics
  - trace and probe support

## 6. Keep release scope narrow

This release is intended to stay on the reviewed Scarthgap and NXP 6.6
baseline. A BSP or kernel migration requires its own review and acceptance
cycle rather than being mixed into a release of this build.
