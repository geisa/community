DESCRIPTION = "A common base for geisa-imx93 product"
LICENSE = "CLOSED"
require recipes-core/images/core-image-minimal.bb

# Testing tools
IMAGE_INSTALL += "cukinia-tests"

# zst compression has the fastest flashing time without a hardware accelerator
IMAGE_FSTYPES += "tar.gz wic.zst wic.bmap"
