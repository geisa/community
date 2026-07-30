# GEISA Development Image for NXP i.MX93

Prepared by [PragSol Consulting LLC](https://www.pragsolconsulting.com). This
community development image is based on the GEISA specification. For more
information, see <https://lfenergy.org/projects/geisa/>.

## Overview

This community-contributed development and validation image targets the NXP
FRDM-i.MX93. It is not an official GEISA-supported deliverable, a conformance
image or intended for production use. Future maintenance belongs to community
maintainers, not the GEISA organization. Its source is available at:

<https://github.com/geisa/community/tree/main/platforms/nxp-imx93-yocto>
Community questions and issue reports belong in the
[GEISA Community issue tracker](https://github.com/geisa/community/issues),
not the specification or conformance repositories.

## Platform and Image Contents

The image is a Yocto/OpenEmbedded `geisa-dev-image` for `geisa-imx93` (AArch64).
It provides LXC, Mosquitto, GEISA runtime defaults, a managed container root,
and Ethos-U support packages, along with development tools and utilities. It
does not include GEISA applications or a conformance framework. The separate
NXP Ethos-U/TFLite profile input is locally staged and is not redistributed in
this source tree until its provenance and redistribution terms are verified.

## Specification Requirements Versus Image Details

| Classification            | Meaning                                                       |
| ------------------------- | ------------------------------------------------------------- |
| Specification requirement | LEE `/etc/geisa/mqtt.conf` is available in each app container |
| Specification requirement | Platform controls CPU, RAM, and persistent/nonpersistent data |
| Specification requirement | `/tmp` is bounded; `/home/geisa` is persistent and bounded    |
| Implementation convention | Host `/etc/geisa`, `/opt/geisa`, `/var/lib/geisa`, and LXC    |
| Image-specific convention | `/platform` holds large and operator/validation artifacts     |
| Validation recommendation | Record requested, rendered, effective, and observed values    |

Note that the GEISA specification uses LXC and `/platform` as examples; the
specification does not mandate the host paths, use of LXC, cgroup v2,
execution-profile locations, or Ethos-U group/udev policy, while the intent of
this image is to provide a GEISA-aligned starting point with reasonable
paths where not specified in the specification, along with basic enabling tools
and utilities.

## Filesystem Layout

The development SD image has a raw i.MX boot area followed by the partitions
below. Sizes describe the current development image and may differ in a future
release image.

| Disk area     | Mount       | Format | Size        | Purpose                                                                                                  |
| ------------- | ----------- | ------ | ----------- | -------------------------------------------------------------------------------------------------------- |
| Raw boot area | Not mounted | Raw    | BSP-defined | i.MX boot image and U-Boot                                                                               |
| Partition 1   | `/boot`     | FAT    | 256 MB      | Boot files and device tree                                                                               |
| Partition 2   | `/`         | ext4   | 8 GB        | Operating system, development tools, GEISA runtime, installed applications, and mutable application data |
| Partition 3   | `/platform` | ext4   | 6 GB        | Large image-provided container artifacts, execution profiles, and operator-supplied input data           |

This image is intended to support commonly available SD cards while allowing
additional space for installed applications and platform artifacts.

Managed application packages, installed applications, configuration, state,
persistent storage, and event records are stored below `/var/lib/geisa` on the
root filesystem. The larger root partition provides some headroom for those
runtime paths and for the development toolchain.

The following locations are part of the image layout. The GEISA-required MQTT
file is mapped inside each application container; the remaining host locations
are implementation conventions used for this image.

| Location                  | Purpose                                         | GEISA status              |
| ------------------------- | ----------------------------------------------- | ------------------------- |
| `/etc/geisa/mqtt.conf`    | Per-application MQTT credentials in a container | Specification requirement |
| `/etc/geisa`              | Host defaults and execution-profile catalog     | Image convention          |
| `/opt/geisa`              | Installed host runtime code and helpers         | Image convention          |
| `/var/lib/geisa`          | GEISA application and platform data             | Implementation convention |
| `/var/lib/geisa/packages` | Downloaded and verified application packages    | Implementation convention |
| `/var/lib/geisa/apps`     | Installed managed-application payloads          | Implementation convention |
| `/var/lib/geisa/config`   | Host-managed application configuration          | Implementation convention |
| `/var/lib/geisa/state`    | Host-managed application runtime state          | Implementation convention |
| `/var/lib/geisa/persist`  | Per-application persistent storage              | Implementation convention |
| `/var/lib/geisa/eventlog` | Durable application and platform event records  | Implementation convention |
| `/var/lib/lxc`            | Managed LXC configuration and rootfs state      | Image convention          |
| `/platform/base`          | Reusable managed-application base artifacts     | Image convention          |
| `/platform/profiles`      | Approved execution profiles                     | Image convention          |
| `/platform/inputs`        | Operator-supplied input data                    | Image convention          |

## Execution Profiles, MQTT, and Resource Controls

GEISA expects the platform, rather than the application itself, to assign and
limit each application's CPU, memory, and storage. This image uses LXC to run
an application in its own container and Linux cgroup v2 to apply those limits.
GEISA does not require LXC or cgroup v2; they are this image's implementation
choice.

Application packages may declare CPU, memory, storage, and process needs in
their application manifest. The System Operator can approve or adjust those
values in a Deployment Manifest, which the platform then uses when configuring
the application. An EMA/application-management implementation can translate
the declarations it supports into LXC and cgroup v2 settings. Effective values
for a running application can be inspected in that application's directory
below `/sys/fs/cgroup`. Running a real or representative workload can help
confirm resource use or CPU throttling.

The NXP execution profile is at `/platform/profiles/nxp-ethosu-tflite`.
Packages should select that profile rather than hard-code host paths.
Per-application MQTT configuration is provided in the container at
`/etc/geisa/mqtt.conf`.

The development image build requires a locally acquired profile archive. See
[`README.nxp-ethosu-tflite-profile.md`](sources/meta-geisa-community/recipes-core/geisa/files/README.nxp-ethosu-tflite-profile.md)
before building or redistributing any image containing that profile.

### Application Storage

Per-application persistent storage is located at
`/var/lib/geisa/persist/<app-id>` and is available inside the managed container
as `/home/geisa`.

Application configuration and host-managed runtime state are stored separately
under `/var/lib/geisa/config/<app-id>` and `/var/lib/geisa/state/<app-id>`.

This image provides the kernel, LXC, cgroup-v2, MQTT, filesystem, execution
profile, and container-base support needed by a GEISA application-management
implementation. The implementation selected by the user is responsible for
translating application declarations into runtime policy and for reporting or
enforcing the limits it supports.

## Build Requirements

Build this image on a Linux host whose filesystem supports normal Linux
ownership, permissions, symbolic links, hard links, and extended attributes.
Eight or more CPU cores and at least 16 GB RAM are a good starting point.

Build systems should allow for at least 300 GB of free local storage, with
~500 GB being more comfortable when retaining downloads, sstate, and multiple
build outputs. The first build will download and compile the NXP BSP,
toolchains, image packages, and managed container base filesystem, and can take
a LONG time. Later changes and rebuilds are usually significantly faster as the
download cache, sstate cache, and build outputs are retained/re-used where
possible.

## Get the Source

    git clone https://github.com/geisa/community.git
    cd community/platforms/nxp-imx93-yocto
    git submodule update --init --recursive

`main` contains the current development version. A published release may name
a tag; to reproduce that release, check out its tag before building. Use
`git tag --list` to see all available tags.

## NXP License Acceptance

Some NXP components require acceptance of the applicable license terms. Review
`sources/meta-freescale/EULA` and `sources/meta-imx/LICENSE.txt`, then express
acceptance in the current shell you'll use for the build later:

    export ACCEPT_FSL_EULA=1

This enables the EULA-gated local build flow. It does not give blanket
redistribution permission for every generated binary. Set it explicitly for
each build command.

## Building the Image

Build the image with:

    ACCEPT_FSL_EULA=1 ./scripts/build.sh development

Before that command can fetch the local profile recipe, stage an archive that
you are authorized to use and redistribute as appropriate:

    ./scripts/stage-nxp-ethosu-profile.sh /path/to/nxp-ethosu-tflite.tar.gz \
        --origin 'source URL or acquisition record'

An optional parse check can catch metadata errors before starting the full
build:

    ACCEPT_FSL_EULA=1 ./scripts/build.sh development -- bitbake -p

Deployment artifacts are written under:

    build-development/tmp/deploy/images/geisa-imx93/

The compressed disk image is named similarly to:

    geisa-dev-image-geisa-imx93.rootfs-<timestamp>.wic.zst

The deployment directory also contains the kernel, device tree,
installed-package manifest, license information, SPDX output, and related build
artifacts.

For general sanity, you should calculate the WIC SHA-256 before writing the
image:

    cd build-development/tmp/deploy/images/geisa-imx93
    sha256sum geisa-dev-image-geisa-imx93.rootfs-<timestamp>.wic.zst

## Published Image Files

A future Community image release may include:

    geisa-dev-image-geisa-imx93.rootfs-<release>.wic.zst
    geisa-dev-image-geisa-imx93.rootfs-<release>.wic.zst.sha256
    geisa-dev-image-geisa-imx93.rootfs-<release>.manifest
    geisa-dev-image-geisa-imx93.rootfs-<release>.spdx.tar.zst
    geisa-dev-image-geisa-imx93.rootfs-<release>.image-manifest.json
    README.md
    NOTICE.md
    Apache-2.0.txt

The package manifest and SPDX archive describe packages and licenses in the
complete image. Package-level licenses remain authoritative for third-party
content. On a running image, the installed-package inventory is available at
`/usr/share/doc/geisa-image/packages.manifest`.

Maintainers preparing a release should follow [RELEASING.md](RELEASING.md).

## Serial Console Access on the FRDM-i.MX93 board

The FRDM-i.MX93 serial console normally uses `115200` baud, 8 data bits, no
parity, 1 stop bit, and no flow control.

On Linux, connect a USB serial adapter, then identify its device:

    ls -l /dev/ttyUSB* /dev/ttyACM* 2>/dev/null
    dmesg | tail -30

Typical devices are `/dev/ttyUSB0` and `/dev/ttyACM0`. Connect with:

    screen /dev/ttyUSB0 115200

or:

    picocom -b 115200 /dev/ttyUSB0

If access is denied, add your user account to the serial-device group, then
start a new login session, e.g.:

    sudo usermod -aG dialout <operator>

On macOS, identify the USB serial device:

    ls -l /dev/cu.usb* /dev/cu.* 2>/dev/null

Typical devices begin with `/dev/cu.usbserial-` or `/dev/cu.usbmodem-`. Connect
with:

    screen /dev/cu.usbserial-<device> 115200

To leave `screen`, press `Ctrl-A`, then `\`, then confirm. To leave `picocom`,
press `Ctrl-A`, then `Ctrl-X`.

Power on or reset the board after connecting. To interrupt U-Boot autoboot,
press any key when its prompt appears. At the U-Boot prompt, use `printenv` to
inspect settings before changing them. Do not run `saveenv` unless the intended
persistent U-Boot change has been reviewed.

## Networking, U-Boot, and SD Boot

DHCP is the default networking configuration. Reservations are optional while
local DNS and NTP are accepted where possible. Note that at least some boards
may generate random MACs on each boot cycle when U-Boot has no saved MACs.

At the serial console, stop autoboot (hit space repeatedly as soon as it
starts to power on), then inspect `printenv ethaddr`, `printenv
eth1addr`, and `printenv bootdelay`. Use unique unicast addresses per board;
do NOT copy this example unchanged:

    setenv ethaddr 02:xx:xx:xx:xx:01
    setenv eth1addr 02:xx:xx:xx:xx:02
    setenv bootdelay 5
    saveenv

`saveenv` persistently writes U-Boot environment; capture existing values and
verify BSP storage first. On the validated FRDM-i.MX93, `ethaddr -> end0` and
`eth1addr -> end1`. Verify with `cat /sys/class/net/end0/address`,
`cat /sys/class/net/end1/address`, `ip -br link`, and `ip -4 -br addr`.

MMC numbering can vary depending on the boot source and board state. The
following U-Boot commands are an example for booting this image from an SD card
when the SD device is `mmc 1` and its Linux root filesystem is
`/dev/mmcblk1p2`:

```
setenv bootargs console=ttyLP0,115200 earlycon \
  root=/dev/mmcblk1p2 rootwait rw
fatload mmc 1:1 0x80400000 Image
fatload mmc 1:1 0x83000000 imx93-11x11-frdm.dtb
booti 0x80400000 - 0x83000000
```

Confirm the actual MMC numbering before using these commands on another board
or after changing the installed media.

Before writing an SD card, use `lsblk` to identify the correct removable device
and verify the downloaded WIC SHA-256. A typical write command is:

```
zstd -dc <image>.wic.zst |
  sudo dd of=<device> bs=16M conv=fsync status=progress
sync
```

Replace `<device>` with the base SD device, e.g. `/dev/sdb`, not one of its
partitions. Writing to the wrong device will destroy its existing contents.

After booting, these commands are useful for confirming the active root and
platform filesystems:

```
findmnt /
findmnt /platform
cat /proc/cmdline
lsblk -f
```

Testing a new image from SD before installing it on eMMC is strongly
recommended. Use the same verified WIC for both steps so the eMMC installation
matches the image that was tested, and keep the validated SD card as recovery
media.

### Known Boot Console Messages

Early boot on the FRDM-i.MX93 can show Ethernet link-up messages, audit notices
while services load, and deferred-probe notices for display-controller or
display regulator devices. These messages were present during successful
Ethernet, container, MQTT, and Ethos-U NPU testing under load and do not
prevent those functions from operating. Wi-Fi regulatory data and Bluetooth
operation remain unvalidated in context of this image at this time.

## Managed Application Container Root Filesystem

This is not a preinstalled application, but a reusable root filesystem used
when creating managed application containers. Application packages are
installed separately by a platform's EMA/application management
implementation.

Tar and SquashFS forms of container root filesystems are stored under
`/platform/base`. An active application-management implementation may select
the form it supports. Exact filenames and SHA-256 hashes are intended to be
recorded in `/etc/geisa/image-manifest.json`.

## Ethos-U

Ethos-U is the neural-processing accelerator/NPU in the i.MX93. It is intended
for efficient inference from compatible TensorFlow Lite models, such as sensor
classification or anomaly-detection workloads.  It is not a general-purpose GPU
and does not train models. It can accelerate only TensorFlow Lite models that
have been prepared for Ethos-U with the Vela tool.

Ethos-U is a relatively low-powered NPU. Its performance depends on model
compatibility and type, Vela compiler and options, model type, tensor
placement, and workload characteristics. It's important for anyone attempting
to evaluate performance that they consider the actual intent, model type and
origin, using identical input inference data rather than relying on
manufacturer published TOPS figures alone.

Note that providing the Ethos toolset, TFLite etc. is an image choice and is
not a GEISA requirement, although it may be useful in doing hardware and/or
performance evaluations.

The development image includes the following Ethos-U enablement components:

| Image component                   | Purpose                                          |
| --------------------------------- | ------------------------------------------------ |
| `ethos-u-driver-stack`            | User-space driver library for `/dev/ethosu0`     |
| `ethos-u-firmware`                | Cortex-M33 firmware for accelerator execution    |
| `tensorflow-lite`                 | TFLite runtime for host and managed applications |
| `tensorflow-lite-ethosu-delegate` | TFLite delegate: `libethosu_delegate.so`         |
| `ethos-u-vela`                    | Model compiler and compatibility checker         |
| `geisa-ethosu-device-permissions` | Group, `geisa` membership, and udev device rule  |
| `geisa-nxp-ethosu-tflite-profile` | Read-only managed-app runtime profile            |

### Device Access Policy

The kernel exposes the accelerator as `/dev/ethosu0`. The image's udev rule
matches `ethosu*` devices and sets ownership to `root:ethosu` with mode `0660`.
The `geisa-ethosu-device-permissions` package creates the system `ethosu`
group and adds the `geisa` account to it. This allows the platform service
account to use the device without making it world-writable or requiring a
root-only application path.

A managed application is intended to receive an explicit narrow Ethos-U device
grant in its rendered LXC configuration. Broader access mechanisms such as use
of `chmod 0666`, broad `/dev` binds, or a root application process should be
avoided and may conflict with GEISA application isolation and/or security
requirements. Other local users also may need deliberate group membership,
and an application should still run as its assigned non-root identity.

    ls -l /dev/ethosu0
    getent group ethosu
    id geisa
    test -r /usr/lib/libethosu_delegate.so
    vela --version

### Using the Ethos NPU in an Application

1. Start with a TensorFlow Lite model that is compatible with the i.MX93
   Ethos-U accelerator, then run Vela to compile it for that hardware. Keep the
   original model, the Vela output, the Vela version, and SHA-256 checksums with
   the application or deployment records.
2. Package it as a managed application and select the `nxp-ethosu-tflite`
   execution profile. The profile, delegate, and approved device grant are
   platform-provided enablers, not files to copy from a source checkout into
   an application.
3. Load `libethosu_delegate.so` through the TensorFlow Lite external-delegate
   path and run inference against the Vela-compatible model.
4. When you run an application intended to run partially or fully on the
   Ethos-U, it may be helpful to log a short run result to confirm that the
   delegate loaded and the process was able to access and use
   `/dev/ethosu0`; you'll generally want to record the number of frames or
   samples, FPS and timing. If the runtime reports CPU fallback, then the model
   was not in fact running on the NPU.

Note that not every TensorFlow Lite model may run entirely on Ethos-U.
Vela offloads only the model operations that it supports. Unsupported layers,
along with some application preprocessing and postprocessing, may still run on
the CPU.

## Tools and Diagnostics

This image is intended to be useful for diagnosis and development, not simply
for running prebuilt applications. The tables below identify some of the main
packages included in this image.

A complete installed-package manifest is available on a running image at
`/usr/share/doc/geisa-image/packages.manifest`.

### Development and Validation

| Tools                     | Use                                                |
| ------------------------- | -------------------------------------------------- |
| `cmake`, `ninja`, `make`  | Configure and build native projects                |
| `gcc`/`g++`/`pkg-config`  | Compile and locate native dependencies             |
| `python3`, `pip3`, `venv` | Python tools and isolated environment support      |
| `node`, `npm`             | Run Node-based development or package tooling      |
| `git`, `file`, `jq`       | Inspect source, artifacts, and JSON data           |
| `python3 -m jsonschema`   | Validate JSON documents against a schema           |
| `/usr/bin/xml`            | Inspect XML                                        |
| `vela`                    | Compile and check Ethos-U compatible TFLite models |

### Editors and File Work

| Tools                         | Use                                             |
| ----------------------------- | ----------------------------------------------- |
| `vim`, `nano`                 | Basic editors.  Sorry, not gonna put EMACs on.. |
| `bash`/`coreutils`/`timeout`  | Useful portable shell and basic tools           |
| `find`, `grep`, `sed`, `gawk` | Basic file search/manipulation tools            |
| `tar`, `gzip`, `zstd`, `xz`   | Inspect and create common archive formats       |
| `rsync`, `diff`, `patch`      | Compare and stage files during diagnosis        |

### Network, Messaging, and Runtime Inspection

| Tools                             | Use                                           |
| --------------------------------- | --------------------------------------------- |
| `ip`, `ss`, `tc`, `ping`          | Inspect links, routes, sockets, and policy    |
| `curl`, `wget`, `nc`, `socat`     | Test HTTP and bounded TCP or Unix sockets     |
| `mosquitto`                       | Local MQTT broker                             |
| `mosquitto_pub`, `mosquitto_sub`  | Publish and subscribe from the command line   |
| `lxc-{ls,info,start,stop,attach}` | Inspect and control managed LXC containers    |
| `systemctl`, `journalctl`         | Inspect system and service state              |
| `htop`/`lsof`/`strace`/`tcpdump`  | Diagnose process, file, syscall, and network  |
| `geisa-system-state`              | GEISA compact health and warning-state helper |

`geisa-system-state` is a community-contributed on-device diagnostic helper.
Its default mode displays a compact, periodically refreshed system summary.
`--once` prints one compact snapshot without refreshing the display, while
`--debug` includes additional network, thermal, LXC, cgroup, socket, and recent
warning-log details.

The script tolerates missing optional tools and reports the information
available on the currently running image.

Usage:

    geisa-system-state [--once] [--debug]

Environment variables:

    INTERVAL=2   Refresh interval in seconds
    NO_CLEAR=1   Do not clear the screen between samples

## Metadata, Package Notes, Licenses, and Limitations

`/etc/geisa/image-manifest.json` is the authoritative machine-readable image
marker, and records image identity, build time, partition layout, artifact
hashes, and Yocto revision state.  `/etc/geisa-image-release` provides a more
concise human-readable companion file.

Bluetooth remains enabled in the image. The NXP IW612 firmware is proprietary
and is available through the NXP BSP layers, but this community image does not
claim to redistribute it unless the final package and license manifests
explicitly list it. Building optional NXP firmware may require acceptance of
the applicable NXP license terms.

`wireless-regdb` provides regional Wi-Fi channel and transmit-power rules. It
is not currently installed as part of this image, although may be added in a
future version if there are requests for it. It is only relevant when Wi-Fi is
enabled; it's expected but untested that normal WiFi will continue to work on
the board using effectively 'global' mode which may not work in some countries.
Its absence does not affect image-enabled current Ethernet and Ethos-U usage.

Community image metadata/docs/scripts are Apache-2.0 only; this does not cover
all Yocto packages, BSP firmware, models, fixtures, or third-party libraries.
Review the Yocto license manifest and package notices before redistribution.
