# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2026 PragSol Consulting, LLC
#

SUMMARY = "GEISA target-local builder dependencies"
DESCRIPTION = "Optional on-target build and packaging tools for GEISA platform work."
LICENSE = "Apache-2.0"

inherit packagegroup

# Keep this packagegroup limited to stable tool/packagegroup names. Libraries
# that are dynamically renamed by shlib packaging are installed directly by
# geisa-dev-image while the dev image remains broad.
RDEPENDS:${PN} = " \
    ca-certificates \
    cmake \
    dosfstools \
    e2fsprogs-mke2fs \
    git \
    mosquitto-dev \
    make \
    mbedtls \
    nanopb-generator \
    nodejs-npm \
    ninja \
    openssl \
    packagegroup-core-buildessential \
    pkgconfig \
    python3 \
    python3-jsonschema \
    python3-pip \
    python3-protobuf \
    squashfs-tools \
"
