# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2025 Southern California Edison
#

DESCRIPTION = "A base image for GEISA application"
LICENSE = "Apache-2.0"
require recipes-core/images/core-image-minimal.bb

IMAGE_FSTYPES = "squashfs tar.gz"

IMAGE_INSTALL:append = " \
    coreutils \
    libasyncns \
    libatomic \
    libmosquitto1 \
    protobuf \
    python3 \
    tensorflow-lite \
    ${GEISA_ACCELERATOR_RUNTIME} \
"

ROOTFS_POSTPROCESS_COMMAND += "geisa_assert_lee_base_libraries;"
geisa_assert_lee_base_libraries() {
    for library in \
        libasyncns.so.* \
        libatomic.so.* \
        libcrypto.so.* \
        libutil.so.* \
        libz.so.* \
        libmosquitto.so.* \
        libprotobuf.so.*; do
        find "${IMAGE_ROOTFS}${libdir}" -maxdepth 1 \
            \( -type f -o -type l \) -name "$library" -print -quit | grep -q . || \
            bbfatal "GEISA LEE base library is missing: $library"
    done

    for library in \
        libc.so.* \
        libgcc_s.so.* \
        libstdc++.so.* \
        libcrypt.so.* \
        libdl.so.* \
        libm.so.* \
        libnsl.so.* \
        libpthread.so.* \
        libresolv.so.* \
        librt.so.* \
        libcap.so.*; do
        if ! find "${IMAGE_ROOTFS}${libdir}" -maxdepth 1 \
            \( -type f -o -type l \) -name "$library" -print -quit | grep -q .; then
            bbnote "GEISA LEE toolchain runtime library is not independently packaged: $library"
        fi
    done
}

ROOTFS_POSTPROCESS_COMMAND += "geisa_application_base_eth0_dhcp;"
geisa_application_base_eth0_dhcp() {
    install -d ${IMAGE_ROOTFS}${sysconfdir}/systemd/network
    cat > ${IMAGE_ROOTFS}${sysconfdir}/systemd/network/10-eth0.network <<'EOF'
[Match]
Name=eth0

[Network]
DHCP=ipv4
EOF
}
