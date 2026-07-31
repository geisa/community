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
    geisa-platform-mount \
    geisa-sudo-policy \
    mosquitto-clients \
    nodejs \
"

# The FRDM-i.MX93 uses the NXP IW612 SDIO/UART module. This exact package is
# supplied by the pinned NXP BSP and contains the kernel-requested Bluetooth
# firmware path. Keep the assertion close to the selected development image.
GEISA_DEV_IW612_FIRMWARE_INSTALL = "firmware-nxp-wifi-nxpiw612-sdio"

ROOTFS_POSTPROCESS_COMMAND += "geisa_assert_iw612_bluetooth_firmware;"
geisa_assert_iw612_bluetooth_firmware() {
    test -r ${IMAGE_ROOTFS}${nonarch_base_libdir}/firmware/nxp/uartspi_n61x_v1.bin.se || \
        bbfatal "IW612 Bluetooth firmware is missing from the development image"
}

# The selected kernel loads regulatory.db as firmware. Its configuration
# determines whether the detached PKCS#7 signature is also mandatory.
ROOTFS_POSTPROCESS_COMMAND += "geisa_assert_regulatory_database;"
geisa_assert_regulatory_database() {
    kernel_config="${STAGING_KERNEL_BUILDDIR}/.config"

    test -r "$kernel_config" || \
        bbfatal "selected kernel configuration is unavailable for regdb validation"
    grep -qx "CONFIG_CFG80211=y" "$kernel_config" || return 0
    test -r ${IMAGE_ROOTFS}${nonarch_base_libdir}/firmware/regulatory.db || \
        bbfatal "kernel regulatory database is missing from the development image"
    if grep -qx "CONFIG_CFG80211_REQUIRE_SIGNED_REGDB=y" "$kernel_config"; then
        test -r ${IMAGE_ROOTFS}${nonarch_base_libdir}/firmware/regulatory.db.p7s || \
            bbfatal "signed kernel regulatory database is missing from the development image"
    fi
}

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
    ${GEISA_DEV_IW612_FIRMWARE_INSTALL} \
    ${GEISA_DEV_RUNTIME_INSTALL} \
    ${GEISA_DEV_DYNAMIC_RENAMED_INSTALL} \
"
