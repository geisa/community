# NXP Ethos-U/TFLite Execution Profile

The `nxp-ethosu-tflite` profile is generated during the Community image build.
It is not a checked-in archive and does not require a locally curated profile
input.

The generator copies the following pinned Yocto runtime outputs from its
recipe sysroot:

- `tensorflow-lite`
- `tensorflow-lite-ethosu-delegate`
- `ethos-u-driver-stack`
- `python3-numpy`

It also fetches the exact public PyAV 18.0.0 aarch64 wheel from PyPI through
BitBake. The URL, filename, tag, and SHA-256 are declared in
`geisa-nxp-ethosu-tflite-profile.bb`; a changed or unavailable wheel fails the
build. The profile's `profile.json` records this input and its generated file
inventory.

The PyAV project declares BSD-3-Clause for its wheel metadata. The wheel also
contains bundled multimedia libraries. The source tree does not redistribute
the wheel, but a release WIC containing the generated profile must undergo the
same-build SPDX and package-license review, including the NXP BSP and bundled
multimedia components, before binary redistribution.
