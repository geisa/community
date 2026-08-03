# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2026 PragSol Consulting, LLC
#

SUMMARY = "GEISA Ethos-U device permissions"
DESCRIPTION = "Installs the GEISA Ethos-U runtime group and udev rule."
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = "file://99-geisa-ethosu.rules"

S = "${UNPACKDIR}"

inherit allarch useradd

USERADD_PACKAGES = "${PN}"
GROUPADD_PARAM:${PN} = "--system ethosu"
GROUPMEMS_PARAM:${PN} = "--group ethosu --add geisa"

do_install() {
    install -d ${D}${nonarch_base_libdir}/udev/rules.d
    install -m 0644 ${UNPACKDIR}/99-geisa-ethosu.rules \
        ${D}${nonarch_base_libdir}/udev/rules.d/99-geisa-ethosu.rules
}

FILES:${PN} = " \
    ${nonarch_base_libdir}/udev/rules.d/99-geisa-ethosu.rules \
"

RDEPENDS:${PN} += "udev geisa-runtime-groups"

DEPENDS += "geisa-runtime-groups"
