# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2026 PragSol
#

SUMMARY = "GEISA developer and bring-up tools"
DESCRIPTION = "Convenience tools for GEISA platform development, inspection, and board bring-up."
LICENSE = "Apache-2.0"

inherit packagegroup

RDEPENDS:${PN} = " \
    curl \
    dtc \
    ethtool \
    file \
    bpftool \
    can-utils \
    xz \
    which \
    tree \
    tar \
    sed \
    less \
    gzip \
    grep \
    gawk \
    findutils \
    diffutils \
    bzip2 \
    bash \
    bluez5 \
    htop \
    i2c-tools \
    iperf3 \
    iproute2 \
    iproute2-ss \
    iproute2-tc \
    iputils \
    ldd \
    iw \
    libgpiod-tools \
    libxml2-utils \
    jq \
    lrzsz \
    lsof \
    nftables \
    perf \
    rfkill \
    gdb \
    gdbserver \
    mmc-utils \
    procps \
    rsync \
    strace \
    tcpdump \
    xmlstarlet \
    wireless-regdb-static \
    netcat \
    nano \
    geisa-system-state \
    psmisc \
    socat \
    vim \
    usbutils \
    util-linux \
"
