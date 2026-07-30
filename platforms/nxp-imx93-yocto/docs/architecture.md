# Architecture

This initial contribution supports the NXP FRDM-i.MX93 development image only.
The Yocto metadata is in `sources/meta-geisa-community`; upstream Yocto layers
are pinned as submodules.

The supported baseline is Scarthgap with the pinned NXP `lf-6.6.y` source and
6.6.36-based kernel configuration. A newer BSP or kernel migration is separate
future work.

The image uses a 256 MB boot partition, an 8 GB writable root filesystem, and
a 6 GB `/platform` filesystem. Host configuration and application data live
under `/etc/geisa`, `/opt/geisa`, `/var/lib/geisa`, and `/var/lib/lxc`.
`/platform` holds the application base, execution profiles, and input data.

`/platform` is selected from the same MMC device that supplies `/`. A systemd
generator accepts `/dev/mmcblkXp2` as the active root and creates the matching
mount for `/dev/mmcblkXp3`. This avoids ambiguous filesystem labels when the
same WIC has been cloned to both SD and eMMC. A root source outside that form
causes the local-filesystem dependency to fail instead of mounting an unrelated
platform partition.

Managed applications use LXC and cgroup v2. Mosquitto is included for local
messaging. The `geisa` and `ethosu` identities, plus the `/dev/ethosu*` udev
policy, enable authorized access to the NXP Ethos-U device.

The development kernel includes checkpoint/restore prerequisites, socket
diagnostics, trace events, dynamic probes, virtual and common USB/serial CAN
paths, and CH341 USB serial support. The corresponding modules have no
steady-state cost while unused. CRIU userspace is intentionally not included.

The development image includes BlueZ and the pinned IW612 firmware package.
FRDM-i.MX93 hardware validation confirmed firmware download and controller
discovery. The selected kernel requires signed wireless regulatory firmware;
the image provides `regulatory.db` and `regulatory.db.p7s` below
`/usr/lib/firmware`.

The NXP Ethos-U/TFLite profile archive is a local build input rather than a
tracked source artifact. See the profile-input README before building it into
an image.
