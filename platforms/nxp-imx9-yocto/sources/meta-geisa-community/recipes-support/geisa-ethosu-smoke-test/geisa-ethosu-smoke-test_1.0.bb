# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2026 PragSol Consulting, LLC
# Website: https://www.pragsolconsulting.com/
#

SUMMARY = "Small on-device Ethos-U diagnostic"
DESCRIPTION = "Runs a single image through Vela, the TFLite Ethos-U delegate, and /dev/ethosu0."
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = " \
    file://geisa-ethosu-smoke-test \
    file://README \
"

S = "${WORKDIR}"

inherit allarch

RDEPENDS:${PN} = " \
    bash \
    coreutils \
    ethos-u-driver-stack \
    ethos-u-vela \
    python3-numpy \
    python3-pillow \
    tensorflow-lite \
    tensorflow-lite-ethosu-delegate \
"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${S}/geisa-ethosu-smoke-test \
        ${D}${bindir}/geisa-ethosu-smoke-test
    sed -i -e 's/@GEISA_TFLITE_RUNTIME_VERSION@/${GEISA_TFLITE_RUNTIME_VERSION}/g' \
        ${D}${bindir}/geisa-ethosu-smoke-test
    install -d ${D}${datadir}/geisa/examples/ethosu-smoke
    install -m 0644 ${S}/README \
        ${D}${datadir}/geisa/examples/ethosu-smoke/README
}

FILES:${PN} = " \
    ${bindir}/geisa-ethosu-smoke-test \
    ${datadir}/geisa/examples/ethosu-smoke/README \
"
