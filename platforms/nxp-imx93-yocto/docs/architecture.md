# Architecture

This initial contribution supports the NXP FRDM-i.MX93 development image only.
The Yocto metadata is in `sources/meta-geisa-community`; upstream Yocto layers
are pinned as submodules.

The image uses a 256 MB boot partition, an 8 GB writable root filesystem, and
a 6 GB `/platform` filesystem. Host configuration and application data live
under `/etc/geisa`, `/opt/geisa`, `/var/lib/geisa`, and `/var/lib/lxc`.
`/platform` holds the application base, execution profiles, and input data.

Managed applications use LXC and cgroup v2. Mosquitto is included for local
messaging. The `geisa` and `ethosu` identities, plus the `/dev/ethosu*` udev
policy, enable authorized access to the NXP Ethos-U device.

The NXP Ethos-U/TFLite profile archive is a local build input rather than a
tracked source artifact. See the profile-input README before building it into
an image.
