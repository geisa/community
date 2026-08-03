# SPDX-License-Identifier: Apache-2.0

SUMMARY = "Root-relative GEISA platform partition mount generator"
DESCRIPTION = "Mounts /platform from partition 3 on the device that supplies the active root filesystem."
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = "file://geisa-platform-generator"
S = "${UNPACKDIR}"

inherit allarch

RDEPENDS:${PN} = "util-linux"

do_install() {
    install -d -m 0755 ${D}${nonarch_libdir}/systemd/system-generators
    install -m 0755 ${UNPACKDIR}/geisa-platform-generator \
        ${D}${nonarch_libdir}/systemd/system-generators/geisa-platform-generator
}

FILES:${PN} = "${nonarch_libdir}/systemd/system-generators/geisa-platform-generator"
