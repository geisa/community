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
    libatomic \
    libmosquitto1 \
    protobuf \
    python3 \
    tensorflow-lite \
    tensorflow-lite-ethosu-delegate \
"

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
