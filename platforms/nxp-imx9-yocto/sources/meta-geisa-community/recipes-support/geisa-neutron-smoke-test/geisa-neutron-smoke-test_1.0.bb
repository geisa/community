# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 PragSol Consulting, LLC
# Website: https://www.pragsolconsulting.com/

SUMMARY = "Small on-device i.MX95 Neutron diagnostic"
DESCRIPTION = "Runs a preconverted fixture through benchmark_model and the Neutron delegate."
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = " \
    file://geisa-neutron-smoke-test \
    file://mobilenet_v1_1.0_224_quant_neutron_imx95.tflite \
    file://README \
"

S = "${UNPACKDIR}"

NEUTRON_FIXTURE_SHA256 = "89e1746ebeae7d54dd4e1017e2c8ef52b2b351dd6eb74791dfc50fa11f106198"

inherit allarch

RDEPENDS:${PN} = " \
    bash \
    coreutils \
    neutron \
    tensorflow-lite \
    tensorflow-lite-neutron-delegate \
    geisa-neutron-device-permissions \
"

do_install() {
    fixture_sha256="$(sha256sum ${S}/mobilenet_v1_1.0_224_quant_neutron_imx95.tflite | awk '{print $1}')"
    [ "$fixture_sha256" = "${NEUTRON_FIXTURE_SHA256}" ] || \
        bbfatal "Neutron smoke fixture SHA-256 mismatch: $fixture_sha256"
    install -d ${D}${bindir}
    install -m 0755 ${S}/geisa-neutron-smoke-test ${D}${bindir}/geisa-neutron-smoke-test
    install -d ${D}${datadir}/geisa/examples/neutron-smoke
    install -m 0644 ${S}/mobilenet_v1_1.0_224_quant_neutron_imx95.tflite \
        ${D}${datadir}/geisa/examples/neutron-smoke/
    install -m 0644 ${S}/README ${D}${datadir}/geisa/examples/neutron-smoke/README
}

FILES:${PN} = " \
    ${bindir}/geisa-neutron-smoke-test \
    ${datadir}/geisa/examples/neutron-smoke \
"
