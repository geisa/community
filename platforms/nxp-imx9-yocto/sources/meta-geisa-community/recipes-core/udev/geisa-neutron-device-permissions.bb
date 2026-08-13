# SPDX-License-Identifier: Apache-2.0

SUMMARY = "GEISA Neutron device permissions"
DESCRIPTION = "Installs the GEISA Neutron runtime group and udev rule."
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = "file://99-geisa-neutron.rules"
S = "${UNPACKDIR}"

inherit allarch useradd

USERADD_PACKAGES = "${PN}"
GROUPADD_PARAM:${PN} = "--system neutron"
GROUPMEMS_PARAM:${PN} = "--group neutron --add geisa"

do_install() {
    install -d ${D}${nonarch_base_libdir}/udev/rules.d
    install -m 0644 ${UNPACKDIR}/99-geisa-neutron.rules \
        ${D}${nonarch_base_libdir}/udev/rules.d/99-geisa-neutron.rules
}

FILES:${PN} = "${nonarch_base_libdir}/udev/rules.d/99-geisa-neutron.rules"
RDEPENDS:${PN} += "udev geisa-runtime-groups"
DEPENDS += "geisa-runtime-groups"
