# SPDX-License-Identifier: Apache-2.0
#
DESCRIPTION = "A debug image for GEISA"
require geisa-common.inc

# Developer tools are collected in packagegroup-geisa-dev-tools.

# System tools
IMAGE_FEATURES += "allow-empty-password debug-tweaks empty-root-password \
                   post-install-logging"

# Clear REPRODUCIBLE_TIMESTAMP_ROOTFS variable to get the build time in /etc/version
# An NFS CI requires a timestamp to distinguish boots.
unset REPRODUCIBLE_TIMESTAMP_ROOTFS

# geisa-dev-image is the board-owner validation image. It intentionally boots
# with a writable rootfs so platform and managed-application development flows
# can be tested directly. Production policy is outside this initial release.
IMAGE_FEATURES:remove = "read-only-rootfs"
GEISA_ROOTFS_FIXED_SIZE = "8192"
GEISA_PLATFORM_FIXED_SIZE = "6144"

ROOTFS_POSTPROCESS_COMMAND += "geisa_dev_rootfs_rw;"
geisa_dev_rootfs_rw() {
    sed -i -e 's#^/dev/root[[:space:]]\+/[[:space:]]\+auto[[:space:]]\+ro#/dev/root            /                    auto       rw#' \
        ${IMAGE_ROOTFS}${sysconfdir}/fstab
}

# Runtime tools needed by the development image. coreutils provides
# /usr/bin/install and /usr/bin/timeout; nodejs supports local development.
GEISA_DEV_RUNTIME_INSTALL = " \
    coreutils \
    geisa-sudo-policy \
    mosquitto-clients \
    nodejs \
"

# Dynamically renamed shlib packages must not live inside allarch packagegroups.
# Keep them directly in the dev image while this image intentionally remains
# broad enough to build, package, and validate GEISA application flows locally.
GEISA_DEV_DYNAMIC_RENAMED_INSTALL = " \
    gnutls \
    libcoap \
    nanopb-runtime \
    protobuf \
    protobuf-c \
    tensorflow-lite \
    tensorflow-lite-host-tools \
    yajl \
    yajl-bin \
"

# Default GEISA developer image packagegroups.
#
# The dev image is intentionally broad while capability enablers and
# package-install behavior are still being refined. It includes target-local
# build/toolchain dependencies so the board can build, package, and validate
# GEISA application flows directly.
IMAGE_INSTALL += " \
    packagegroup-geisa-dev-tools \
    packagegroup-geisa-platform-builder-deps \
    packagegroup-geisa-imx93-ethosu-dev \
    geisa-nxp-ethosu-tflite-profile \
    ${GEISA_DEV_RUNTIME_INSTALL} \
    ${GEISA_DEV_DYNAMIC_RENAMED_INSTALL} \
"
