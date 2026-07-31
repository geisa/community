# Known Issues

## Host-side ext4 inspection compatibility

The validated FRDM-i.MX93 first-boot `geisa-platform-fsck.service` selected the
correct sibling platform partition, returned status 0, and reported it clean.
Some Ubuntu 22.04 and other older build environments provide e2fsprogs 1.46.5
or another version that does not recognize the ext4 `FEATURE_C12` feature used
by this image. Such a tool may exit with status 12; that result is not proof of
filesystem corruption. Offline validation requires a newer compatible e2fsprogs
environment. Target-side e2fsck may be used when appropriate.

Earlier observations also involved an ambiguous `LABEL=platform` fstab entry
when the same WIC was present on both SD and eMMC. The image now derives the
platform partition from the active MMC root device, so this cross-mount cause
is addressed independently of the host e2fsprogs compatibility limitation.

`scripts/inspect-wic.sh` now extracts the root and platform partitions and runs
`e2fsck -fn` without modifying them. A candidate must be checked using an
e2fsprogs version that supports all enabled filesystem features before release.

## NXP Ethos-U/TFLite profile redistribution

The archived runtime profile used for validation is not committed because its
origin, complete component inventory, and redistribution authorization have not
been established. It remains a local build input only. No public image
containing it may be released until that review is complete.

## IW612 Bluetooth firmware and controller validation

The development image includes NXP's pinned IW612 SDIO firmware package and
the expected `uartspi_n61x_v1.bin.se` path. FRDM-i.MX93 validation confirmed
firmware download and controller discovery. The package is marked
`Proprietary` by the NXP BSP recipe; public binary redistribution authorization
still requires review against the NXP EULA Component Register and section 2.3.

## Wireless regulatory database

The kernel requires signed `regulatory.db` firmware. The image now selects
`wireless-regdb-static`, which supplies `regulatory.db` and `regulatory.db.p7s`
under `/usr/lib/firmware`, and the image build asserts both files. The corrected
image still needs one hardware boot check confirming the previous kernel load
warning is absent.

## Production image

There is no validated production image, production policy, or i.MX95 support in
this initial contribution.
