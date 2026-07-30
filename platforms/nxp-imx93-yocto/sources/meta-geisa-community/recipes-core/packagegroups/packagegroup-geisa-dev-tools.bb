# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2026 PragSol
#

SUMMARY = "GEISA developer and bring-up tools"
DESCRIPTION = "Convenience tools for GEISA platform development, inspection, and board bring-up."
LICENSE = "Apache-2.0"

inherit packagegroup

RDEPENDS:${PN} = " \
    file \
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
    htop \
    iperf3 \
    iproute2 \
    iproute2-ss \
    iproute2-tc \
    iputils \
    libxml2-utils \
    jq \
    lrzsz \
    lsof \
    procps \
    rsync \
    strace \
    tcpdump \
    xmlstarlet \
    netcat \
    nano \
    geisa-system-state \
    psmisc \
    socat \
    vim \
    util-linux \
"
