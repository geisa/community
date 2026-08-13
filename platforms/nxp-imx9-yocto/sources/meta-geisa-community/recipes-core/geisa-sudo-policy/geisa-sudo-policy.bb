# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2026 PragSol Consulting, LLC
#

SUMMARY = "GEISA development-image sudo policy"
DESCRIPTION = "Enables authenticated sudo for members of the standard sudo group."
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = "file://10-sudo-group"
S = "${UNPACKDIR}"

RDEPENDS:${PN} = "sudo"

do_install() {
    install -d -m 0750 ${D}${sysconfdir}/sudoers.d
    install -m 0440 ${UNPACKDIR}/10-sudo-group ${D}${sysconfdir}/sudoers.d/10-sudo-group
}

FILES:${PN} = "${sysconfdir}/sudoers.d/10-sudo-group"
