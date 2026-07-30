# Known Issues

## First-boot `/platform` filesystem check

A first-boot `/platform` filesystem-check failure has been observed and is an
unresolved release blocker. The inspected WIC reports both ext4 filesystems as
`clean`, but the current vm-lts `e2fsck` (1.46.5) cannot read the image's
`FEATURE_C12` ext4 feature and exits with status 12. This does not prove the
filesystem is inconsistent or correct.

`scripts/inspect-wic.sh` now extracts the root and platform partitions and runs
`e2fsck -fn` without modifying them. A candidate must be checked using an
e2fsprogs version that supports all enabled filesystem features before release.

## NXP Ethos-U/TFLite profile redistribution

The archived runtime profile used for validation is not committed because its
origin, complete component inventory, and redistribution authorization have not
been established. It remains a local build input only. No public image
containing it may be released until that review is complete.

## Production image

There is no validated production image, production policy, or i.MX95 support in
this initial contribution.
