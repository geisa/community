# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2025 Southern California Edison
#

DESCRIPTION = "A base image for GEISA application"
LICENSE = "Apache-2.0"
require recipes-core/images/core-image-minimal.bb

IMAGE_FSTYPES = "squashfs"

IMAGE_INSTALL:append = " \
    libatomic \
    libmosquitto1 \
    protobuf \
"
