# Build Requirements

Build on Linux with normal filesystem support for ownership, permissions,
symbolic links, hard links, and extended attributes. Allow at least 16 GiB of
RAM and 300 GiB of free disk space when retaining downloads, sstate, and build
outputs.

## Source Setup

Initialize the pinned source trees, then check the default release profile:

```sh
git submodule update --init --recursive
./scripts/setup-sources.sh --check --release nxp-6.18.2-1.0.0
```

The default profile is `nxp-6.18.2-1.0.0`, using NXP 6.18.2 on Whinlatter.
The check requires each profile source to be initialized and at its pinned
commit. It reports whether each source checkout has tracked or untracked
changes, but does not reject those changes. It does not fetch, switch, reset,
or otherwise change repositories.

The build records the Community repository revision and dirty state separately
from the pinned source check. A dirty Community tree is therefore allowed for
development builds and is recorded in the generated image metadata. A dirty
source checkout is also reported, but a source-changing setup operation will
refuse to switch it to another commit.

## Build

The build script requires local NXP license acceptance for every invocation:

```sh
ACCEPT_FSL_EULA=1 ./scripts/build.sh development \
    --release nxp-6.18.2-1.0.0 \
    --machine geisa-imx93 -- bitbake geisa-dev-image
```

For i.MX95, use:

```sh
ACCEPT_FSL_EULA=1 ./scripts/build.sh development \
    --release nxp-6.18.2-1.0.0 \
    --machine geisa-imx95 -- bitbake geisa-dev-image
```

Omitting `--machine` is supported, but selects `geisa-imx93` by default. Use
an explicit machine for reproducible board builds.

The script uses `DL_DIR` and `SSTATE_DIR` from the environment when set.
Otherwise it uses:

```text
$HOME/yocto-cache/downloads
$HOME/yocto-cache/sstate
```

The following settings have been used on a 32 GiB, 12-vCPU build host:

```sh
BB_NUMBER_THREADS=12
PARALLEL_MAKE="-j 8"
PARALLEL_MAKEINST="-j 8"
```

Reduce the make job count if your build host runs short on memory.

## Checks and Outputs

Run a parse-only check with the same release and machine selection used for
the build:

```sh
ACCEPT_FSL_EULA=1 ./scripts/build.sh development \
    --release nxp-6.18.2-1.0.0 \
    --machine geisa-imx93 -- bitbake -p
```

Use `--machine geisa-imx95` for the i.MX95 parse check. The build outputs are
written below:

```text
build-development-nxp-6.18.2-1.0.0/tmp/deploy/images/<machine>/
```

The build script requires `ACCEPT_FSL_EULA=1` after reviewing the applicable
NXP terms in `sources/meta-freescale/EULA` and `sources/meta-imx/LICENSE.txt`.
That setting enables the local build flow; it does not grant redistribution
rights for generated images or other build outputs.
