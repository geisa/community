# Architecture

This initial contribution supports the NXP FRDM-i.MX93 development image only.
The Yocto metadata is in `sources/meta-geisa-community`; upstream Yocto layers
are pinned as submodules.

The supported default is NXP 6.6.52-2.2.2 on Scarthgap with Linux 6.6.52 and
the pinned FRDM-i.MX93 layer set. A newer BSP or kernel migration is separate
future work.

Release selection is explicit. `releases/default` selects
`nxp-6.6.52-2.2.2`; `scripts/setup-sources.sh --check` reports expected and
actual source commits without changing them. The `nxp-6.12.34-2.1.0` profile
is an incomplete experimental placeholder: it has no verified source commits
or recipe compatibility and must fail source setup and builds until those are
established.

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
discovery. Built-in cfg80211 requests signed regulatory firmware before
the root filesystem is available. The development build embeds
`regulatory.db` and `regulatory.db.p7s` in the kernel image. It retains
runtime copies below `/usr/lib/firmware`.

The NXP Ethos-U/TFLite profile is generated during the image build from
declared Yocto runtime outputs and a checksum-pinned PyAV wheel. See
`docs/nxp-ethosu-tflite-profile.md` for its reproducible source inputs and
redistribution boundary.
