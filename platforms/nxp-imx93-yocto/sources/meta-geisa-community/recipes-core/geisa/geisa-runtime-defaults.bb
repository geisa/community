# SPDX-License-Identifier: Apache-2.0
#
SUMMARY = "GEISA runtime defaults"
DESCRIPTION = "Installs shared GEISA runtime defaults for clean FRDM-i.MX93 images."
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = "file://geisa"

S = "${WORKDIR}"

inherit allarch

do_install() {
    install -d ${D}${sysconfdir}/default
    install -m 0644 ${WORKDIR}/geisa ${D}${sysconfdir}/default/geisa
}

FILES:${PN} = "${sysconfdir}/default/geisa"
