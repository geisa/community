SUMMARY = "GEISA API daemon"
DESCRIPTION = "GEISA API daemon that answers to API requests from GEISA application with MQTT."
HOMEPAGE = "https://github.com/geisa/community/tree/main/example-implementations/api/api-mockup"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE;md5=86d3f3a95c324c9479bd8986968f4327"

inherit pkgconfig python3native systemd

SRC_URI = " \
    gitsm://github.com/geisa/community.git;protocol=https;branch=main \
    file://gapi.service \
"
SRCREV = "${AUTOREV}"
S = "${UNPACKDIR}/${BP}/example-implementations/api/api-mockup"

DEPENDS = "protobuf-native python3-protobuf-native nanopb-generator-native nanopb-runtime mosquitto"

RDEPENDS:${PN} = "nanopb-runtime"

TARGET_CC_ARCH += "${LDFLAGS}"

PARALLEL_MAKE = ""

do_compile() {
    # The Makefile stops unless .venv/bin/python exists. That venv normally
    # comes from "make setup-dev", which needs pip and network access. Yocto
    # already provides Python with protobuf, protoc and the nanopb plugin, so
    # link .venv/bin/python to that Python to pass the check.
    install -d ${S}/.venv/bin
    ln -sf "${PYTHON}" ${S}/.venv/bin/python

    protoc_cmd="$(command -v protoc)"
    nanopb_plugin="$(command -v protoc-gen-nanopb)"

    test -n "${protoc_cmd}"
    test -n "${nanopb_plugin}"

    oe_runmake \
        PROTOC_CMD="${protoc_cmd}" \
        NANOPB_PLUGIN="${nanopb_plugin}" \
        build/gapi
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/build/gapi ${D}${bindir}/

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${UNPACKDIR}/gapi.service ${D}${systemd_system_unitdir}/
}

SYSTEMD_SERVICE:${PN} = "gapi.service"
