SUMMARY = "GEISA API daemon"
DESCRIPTION = "GEISA API daemon that answers to API requests from GEISA application with MQTT."
HOMEPAGE = "https://github.com/geisa/api-mockup"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

inherit systemd

SRC_URI = " \
    gitsm://github.com/geisa/api-mockup.git;protocol=https;branch=main \
    file://gapi.service \
"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

DEPENDS = "protobuf-native nanopb-generator-native nanopb-runtime mosquitto"

RDEPENDS:${PN} = "nanopb-runtime"

TARGET_CC_ARCH += "${LDFLAGS}"

PARALLEL_MAKE = ""

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/build/gapi ${D}${bindir}/

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/gapi.service ${D}${systemd_system_unitdir}/
}

SYSTEMD_SERVICE:${PN} = "gapi.service"
