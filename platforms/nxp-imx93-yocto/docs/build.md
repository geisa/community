# Build

Use a Linux host with normal Linux filesystem semantics, at least 16 GB RAM,
and substantial free storage. Retaining downloads and sstate makes later builds
much faster; 300 GB free is a practical starting point.

Initialize submodules, review `sources/meta-freescale/EULA` and
`sources/meta-imx/LICENSE.txt`, then explicitly accept the NXP build terms:

```sh
git submodule update --init --recursive
./scripts/stage-nxp-ethosu-profile.sh /path/to/nxp-ethosu-tflite.tar.gz \
  --origin 'source URL or acquisition record'
ACCEPT_FSL_EULA=1 ./scripts/build.sh development
```

`ACCEPT_FSL_EULA=1` allows the local build flow only. It does not grant
redistribution rights for generated artifacts or locally staged profile input.
Use `ACCEPT_FSL_EULA=1 ./scripts/build.sh development -- bitbake -p` for a
parse-only check.
