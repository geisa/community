# Licensing

Apache-2.0 applies to Community-owned metadata, scripts, and documentation
where stated. It does not license the Yocto image as a whole, NXP BSP content,
firmware, models, media, or third-party runtime components.

Review the same-build package manifest, SPDX archive, and generated license
material before distributing any WIC. NXP EULA acceptance enables local build
handling; it is not blanket permission to redistribute binaries.

The NXP Ethos-U/TFLite profile is generated from declared NXP/Yocto runtime
outputs and a checksum-pinned public PyAV wheel. The Community source tree does
not redistribute a preassembled profile archive. A WIC that includes the
generated profile still requires same-build SPDX and package-license review for
the NXP components and the PyAV wheel's bundled multimedia libraries before
binary redistribution.
