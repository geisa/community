# Build

Use a Linux host with normal Linux filesystem semantics, at least 16 GB RAM,
and substantial free storage. Retaining downloads and sstate makes later builds
much faster; 300 GB free is a practical starting point.

Initialize submodules, review `sources/meta-freescale/EULA` and
`sources/meta-imx/LICENSE.txt`, then explicitly accept the NXP build terms:

```sh
git submodule update --init --recursive
ACCEPT_FSL_EULA=1 ./scripts/build.sh development
```

`nxp-6.6.52-2.2.2` is the default supported release. Check or select it
explicitly with:

```sh
./scripts/setup-sources.sh --check --release nxp-6.6.52-2.2.2
ACCEPT_FSL_EULA=1 ./scripts/build.sh development \
    --release nxp-6.6.52-2.2.2
```

`setup-sources.sh` checks exact local commits and refuses to switch a dirty
submodule. It does not fetch or discard local work. The experimental
`nxp-6.12.34-2.1.0` profile is intentionally incomplete and fails before
BitBake because its exact source revisions are not verified.

`ACCEPT_FSL_EULA=1` allows the local build flow only. It does not grant
redistribution rights for generated artifacts. The build generates the
Ethos-U/TFLite profile from declared dependencies and a checksum-pinned PyAV
wheel; see `docs/nxp-ethosu-tflite-profile.md`.
Use `ACCEPT_FSL_EULA=1 ./scripts/build.sh development -- bitbake -p` for a
parse-only check.

Development work may build from a deliberately dirty source tree, and the
image metadata records that state. Before publication, use the reviewed clean
revision, confirm it with `git status --short`, and retain the source-state
record alongside the release artifacts.

This platform intentionally stays on the reviewed Scarthgap and NXP 6.6 track.
Do not combine a BSP or kernel migration with a development-image release build.
