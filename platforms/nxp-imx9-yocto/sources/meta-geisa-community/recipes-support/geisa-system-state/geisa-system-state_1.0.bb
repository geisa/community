# Copyright 2026 PragSol Consulting, LLC
# Website: https://www.pragsolconsulting.com/
#
# Licensed under the Apache License, Version 2.0.
# SPDX-License-Identifier: Apache-2.0

SUMMARY = "GEISA system state helper"
DESCRIPTION = "Installs a compact shell diagnostic helper for GEISA board bring-up and image validation."
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = "file://geisa-system-state"

S = "${UNPACKDIR}"

inherit allarch

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${UNPACKDIR}/geisa-system-state ${D}${bindir}/geisa-system-state
}

FILES:${PN} = "${bindir}/geisa-system-state"
