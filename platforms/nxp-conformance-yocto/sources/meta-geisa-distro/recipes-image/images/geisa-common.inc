# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2025 Southern California Edison
#

DESCRIPTION = "A common base for GEISA images"
LICENSE = "CLOSED"
require recipes-core/images/core-image-minimal.bb

# zst compression has the fastest flashing time without a hardware accelerator
IMAGE_FSTYPES += "tar.gz wic.zst wic.bmap"

IMAGE_FEATURES:append = " ssh-server-openssh allow-root-login"

IMAGE_INSTALL:append = " \
    glibc-utils \
    lrzsz \
    lxc \
    lxc-networking \
    kernel-modules \
    iperf3 \
    mosquitto \
"

TOOLCHAIN_TARGET_TASK:append = " \
    wakaama-dev \
    wakaama-staticdev \
"

DEPLOY_DIR_GEISA_CONTAINER ?= "${TOPDIR}/tmp-geisa-container/deploy/images/${MACHINE}"

addtask copy_application_base_image before do_image after do_rootfs
do_copy_application_base_image[mcdepends] += "mc::geisa-container:geisa-application-base:do_image_complete"
do_copy_application_base_image() {
    install -d ${IMAGE_ROOTFS}${sysconfdir}/geisa

    install -m 0644 ${DEPLOY_DIR_GEISA_CONTAINER}/geisa-application-base-${MACHINE}.rootfs.squashfs ${IMAGE_ROOTFS}${sysconfdir}/geisa/
}
