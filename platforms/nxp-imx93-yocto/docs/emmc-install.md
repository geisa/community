# eMMC Installation

eMMC installation and boot were validated on the specific FRDM-i.MX93 test
board used for this development image. That result does not make this document
a universal installer procedure. Validate an exact WIC on removable media
first, retain that media as recovery media, and make a verified backup of
existing eMMC contents before any destructive operation.

After eMMC boot, confirm root is eMMC partition 2 and `/platform` is the
matching eMMC partition 3. The image derives `/platform` from the active root
device, so both sources must have the same `mmcblkX` number.

Do not publish a generic write command here: board boot policy, target-device
identity, recovery procedure, and operator authorization must be reviewed for
the specific hardware and release artifact.
