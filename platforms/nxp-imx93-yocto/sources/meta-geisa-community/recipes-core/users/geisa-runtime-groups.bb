# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2026 PragSol
#

SUMMARY = "GEISA runtime identity"
DESCRIPTION = "Creates the GEISA runtime system user and group for image services."
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

inherit useradd

USERADD_PACKAGES = "${PN}"
GROUPADD_PARAM:${PN} = "--system geisa"
USERADD_PARAM:${PN} = "--system --no-create-home --home-dir /var/lib/geisa --shell /sbin/nologin --gid geisa --password '*' geisa"

do_install() {
    install -d -m 0755 ${D}${sysconfdir}/geisa
    install -d -m 0755 ${D}${localstatedir}/lib/geisa
    chown 0:0 ${D}${localstatedir}/lib/geisa
}

FILES:${PN} = " \
    ${sysconfdir}/geisa \
    ${localstatedir}/lib/geisa \
"

ALLOW_EMPTY:${PN} = "1"
