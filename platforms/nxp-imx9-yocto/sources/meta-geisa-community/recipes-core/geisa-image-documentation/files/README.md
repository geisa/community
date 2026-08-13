<!-- markdownlint-configure-file {"MD013": false, "MD046": false} -->

# GEISA Development Image for NXP i.MX9

Prepared by [PragSol Consulting LLC](https://www.pragsolconsulting.com).

This community development image is based on the GEISA specification. For more
information, see <https://lfenergy.org/projects/geisa/>.

## Overview

This Community-contributed development and validation image targets the NXP
FRDM-i.MX93 and FRDM-i.MX95. It uses the NXP 6.18.2-1.0.0 BSP on Whinlatter
with Linux kernel 6.18.2.

The image provides common GEISA runtime and development tools and also enables
the relevant NPU/accelerator for the selected machine (Ethos-U or Neutron). It
is intended as a development image, not for production use.  It does not include
any GEISA applications, LEE or API implementations.

Source is available at:

<https://github.com/geisa/community/tree/main/platforms/nxp-imx9-yocto>

Community questions and issue reports belong in the
[GEISA Community issue tracker](https://github.com/geisa/community/issues).

## Supported Boards

| Machine       | Board       | Accelerator | Validation                   |
| ------------- | ----------- | ----------- | ---------------------------- |
| `geisa-imx93` | FRDM-i.MX93 | Ethos-U     | `geisa-ethosu-smoke-test`    |
| `geisa-imx95` | FRDM-i.MX95 | Neutron     | `geisa-neutron-smoke-test`   |

## Image Contents

The common image includes the GEISA application base, managed LXC containers,
cgroup v2 support, Mosquitto, protobuf, systemd services, SSH, and
development tools. The application base is installed under `/platform/base`
as SquashFS and tar archives. The `/platform` filesystem also holds execution
profiles and other image-provided validation data.  `/platform` is an image
choice and not mandated by the GEISA specification at time of publishing.

System configuration selects the accelerator support; Ethos-U for the i.MX93
and Neutron for the i.MX95.

The WIC layout is:

| Disk area     | Mount       | Format | Size        | Purpose                     |
| ------------- | ----------- | ------ | ----------- | --------------------------- |
| Raw boot area | Not mounted | Raw    | BSP-defined | i.MX boot image and U-Boot  |
| Partition 1   | `/boot`     | FAT    | 256 MiB     | Boot files and device trees |
| Partition 2   | `/`         | ext4   | 8 GiB       | Operating system and tools  |
| Partition 3   | `/platform` | ext4   | 6 GiB       | Application base and data   |

The platform generator derives `/platform` from the active root device. The
root and platform partitions must be partitions 2 and 3 of the same MMC
device.

### Included Tools

The following are a subset of the included dev image tools, not a complete
package list.

| Tool or Package                  | Use                        |
| -------------------------------- | -------------------------- |
| GCC/G++, make, CMake, Ninja      | Native builds              |
| Git, rsync, jq, file             | Source and artifact work   |
| Python/pip, Node/npm             | Script and package tooling |
| protoc, protobuf, nanopb         | GEISA/protobuf development |
| iproute2, ethtool, tcpdump, curl | Network diagnosis          |
| LXC tools, systemctl, journalctl | Containers and services    |
| vim, nano, less                  | Editing and log viewing    |

The complete installed package inventory is at
`/usr/share/doc/geisa-image/packages.manifest`.

## Building the Image

Clone the Community repository and change to the shared NXP i.MX9 platform:

```sh
git clone https://github.com/geisa/community.git
cd community/platforms/nxp-imx9-yocto
```

Initialize the pinned source trees and check the selected release profile:

```sh
git submodule update --init --recursive
./scripts/setup-sources.sh --check --release nxp-6.18.2-1.0.0
```

Build the i.MX93 image with:

```sh
ACCEPT_FSL_EULA=1 ./scripts/build.sh development \
    --release nxp-6.18.2-1.0.0 \
    --machine geisa-imx93 -- bitbake geisa-dev-image
```

Build the i.MX95 image with:

```sh
ACCEPT_FSL_EULA=1 ./scripts/build.sh development \
    --release nxp-6.18.2-1.0.0 \
    --machine geisa-imx95 -- bitbake geisa-dev-image
```

The build uses the existing download and sstate caches when `DL_DIR` and
`SSTATE_DIR` are configured. Otherwise it uses the default cache locations
under `$HOME/yocto-cache`.

The 12/8/8 settings have been used on a 32 GiB, 12-vCPU build host. Reduce
the make job count if the host runs short on memory:

```sh
BB_NUMBER_THREADS=12
PARALLEL_MAKE="-j 8"
PARALLEL_MAKEINST="-j 8"
```

Build outputs are written at:

`build-development-nxp-6.18.2-1.0.0/tmp/deploy/images/<machine>/`

## On-device Scripts

A few scripts are included for convenience:

- `geisa-system-state` provides a refreshed view of system stats including
  container usage, useful for small screens.

  - Use `geisa-system-state --once` for a single more detailed snapshot for
  capturing diagnostics or logs.

  - Add `--debug` to include routes, listening sockets, and recent warning or
  error messages.

- `geisa-ethosu-smoke-test` gives a quick and dirty PASS/FAIL indication for
  inferencing using the Ethos-U NPU on the i.MX93

- `geisa-neutron-smoke-test` does similarly for the i.MX95 Neutron NPU, with
  a bit more detail and statistics.

Both of the NPU tests are intended as simple smoke checks to ensure the required
drivers, software and permissions are usable for inferencing on the respective
boards.

See [SD installation](https://github.com/geisa/community/tree/main/platforms/nxp-imx9-yocto/docs/sd-install.md),
[eMMC installation](https://github.com/geisa/community/tree/main/platforms/nxp-imx9-yocto/docs/emmc-install.md),
and [commissioning](https://github.com/geisa/community/tree/main/platforms/nxp-imx9-yocto/docs/commissioning.md)
for additional information.

## Licensing

NXP components are subject to the applicable NXP license terms. See
[`docs/licensing.md`](https://github.com/geisa/community/tree/main/platforms/nxp-imx9-yocto/docs/licensing.md)
for the image's licensing notes.
