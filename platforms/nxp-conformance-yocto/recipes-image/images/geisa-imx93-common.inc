DESCRIPTION = "A common base for geisa-imx93 product"
LICENSE = "CLOSED"
require recipes-core/images/core-image-minimal.bb

# zst compression has the fastest flashing time without a hardware accelerator
IMAGE_FSTYPES += "tar.gz wic.zst wic.bmap"

IMAGE_FEATURES:append = " ssh-server-openssh allow-root-login"

IMAGE_INSTALL:append = " \
    lrzsz \
    lxc \
    kernel-modules \
"
