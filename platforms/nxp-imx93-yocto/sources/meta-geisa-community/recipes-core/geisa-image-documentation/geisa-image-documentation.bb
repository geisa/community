# SPDX-License-Identifier: Apache-2.0

SUMMARY = "GEISA NXP i.MX93 community image documentation"
DESCRIPTION = "Installs the canonical community-image documentation and license material."
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"
SRC_URI = "file://README.md file://NOTICE.md file://Apache-2.0.txt"
S = "${UNPACKDIR}"
inherit allarch

do_install() {
    install -d -m 0755 ${D}${datadir}/doc/geisa-image/LICENSES
    install -m 0644 ${UNPACKDIR}/README.md ${D}${datadir}/doc/geisa-image/README.md
    install -m 0644 ${UNPACKDIR}/NOTICE.md ${D}${datadir}/doc/geisa-image/NOTICE.md
    install -m 0644 ${UNPACKDIR}/Apache-2.0.txt ${D}${datadir}/doc/geisa-image/LICENSES/Apache-2.0.txt
}

# Keep the published image documentation in the installable main package.
FILES:${PN}-doc = ""
FILES:${PN} = "${datadir}/doc/geisa-image"
