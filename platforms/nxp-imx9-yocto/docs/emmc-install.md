# eMMC Installation

eMMC installation and boot were validated on a specific FRDM-i.MX93 test board
used for this development image. This document covers that i.MX93 validation
only; it is not a universal installer procedure. The shared image also supports
FRDM-i.MX95, but no i.MX95 eMMC procedure is validated here. Do not assume the
i.MX93 result or boot process applies to i.MX95.

Validate an exact WIC on removable media first, retain that media as recovery
media, and make a verified backup of existing eMMC contents before any
destructive operation.

After eMMC boot, confirm root is eMMC partition 2 and `/platform` is the
matching eMMC partition 3. The image derives `/platform` from the active root
device, so both sources must have the same `mmcblkX` number.

The i.MX95 uses a distinct FRDM U-Boot and boot configuration. Establish its
eMMC installation and validation separately before using an i.MX95 image on
eMMC.

This document does not include a generic destructive write command. Board boot
settings and recovery steps must be established for the specific i.MX board
and image.
