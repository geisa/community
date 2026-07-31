# Release Process

This initial subtree supports development-image work only. Before a public
artifact is released, record a source revision and submodule revisions, run a
normal build, and collect one unambiguous WIC plus its manifests, SPDX archive,
and checksums.

Run `./scripts/validate-release.sh RELEASE_DIRECTORY`. It verifies staged
checksums and performs read-only ext4 checks on the WIC. The release must stop
if e2fsck cannot validate the selected ext4 feature set or reports errors.

Offline acceptance should  also verify that the root filesystem contains the
platform-mount generator, the selected IW612 firmware path
`/usr/lib/firmware/nxp/uartspi_n61x_v1.bin.se`, and the kernel regulatory
database paths `/usr/lib/firmware/regulatory.db` and `regulatory.db.p7s`.
It also should verify that the kernel image embeds the regulatory database and
signature needed before the root filesystem is available. Hardware acceptance
should verify that root and `/platform` come from partitions 2 and 3 of the
same MMC device, that early platform fsck is clean, and that Bluetooth
controller discovery succeeds without a regulatory-database warning.

The final candidate must also confirm the development-only field tools and
kernel capabilities selected for this image: `can-utils`, `iw`, `rfkill`,
`nft`, `perf`, `bpftool`, checkpoint/restore prerequisites, socket diagnostics,
and trace/probe support. This release stays on the reviewed Scarthgap/NXP 6.6
baseline; BSP or kernel migration requires a separate acceptance cycle.

Development builds may contain source-dirty provenance while work is in
progress. Release artifacts should be built from the reviewed clean source state.
