# SD Installation

Verify the selected WIC SHA-256 and confirm the removable target by model,
capacity, and path before writing. Decompress the exact WIC to the whole SD
device, flush it, then inspect the resulting partition table.

Boot manually and confirm that root is the SD partition 2 and `/platform` is
the matching SD partition 3. The image derives `/platform` from the active
root device; both sources must have the same `mmcblkX` number. Preserve the SD
card as recovery media until the target artifact has completed its documented
hardware validation.

This document intentionally omits a copy-and-paste destructive command because
the target device and operator authorization must be checked for each write.
