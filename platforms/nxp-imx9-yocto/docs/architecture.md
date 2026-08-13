# Architecture

This tree contains a shared Yocto platform for the NXP FRDM-i.MX93 and
FRDM-i.MX95. Common image and runtime metadata lives in
`sources/meta-geisa-community`. The `geisa-imx93` and `geisa-imx95` machine
files are thin wrappers around the NXP board includes and shared i.MX9
settings.

## Shared Platform

The common image provides the GEISA runtime defaults, systemd integration,
Mosquitto, LXC, LXC networking, kernel modules, and development image support.
The `geisa-dev-image` recipe also builds a separate `geisa-application-base`
image through the `geisa-container` multiconfig. It copies that image's
SquashFS and tar outputs to `/platform/base`.

The image creates `/platform/base`, `/platform/profiles`, and `/platform/inputs`.
A root-relative systemd generator accepts an active root at
`/dev/mmcblkXp2` and derives `/dev/mmcblkXp3` for `/platform`. Unsupported root
sources fail closed. This keeps a cloned SD or eMMC image from selecting a
platform partition on another MMC device.

The image includes the kernel, userspace, and service support used by a
higher-level platform implementation to place applications in cgroup v2 and
apply resource limits. Application-specific placement and limits are outside
this image.

## System Selection

System configuration sets the accelerator package group, runtime, device
group, and profile settings. The selection is implemented through system
variables and package groups; there is no separate generic accelerator layer.

| System 		| Accelerator | System-specific content 						          |
| ------------- | ----------- | --------------------------------------------------------- |
| `geisa-imx93` | Ethos-U     | Ethos-U runtime, device policy, and TFLite profile        |
| `geisa-imx95` | Neutron     | Neutron runtime, device policy, and qualification fixture |

The i.MX93 provides a platform profile path for Ethos-U. The i.MX95 uses the
Neutron runtime without a platform profile path.
