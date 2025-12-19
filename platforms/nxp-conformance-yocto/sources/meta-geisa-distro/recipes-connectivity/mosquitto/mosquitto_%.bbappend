# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2025 Southern California Edison
#

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " \
    file://mosquitto.conf \
    file://dynamic-security.json \
"

do_install:append() {
    install -D -m 0644 ${WORKDIR}/mosquitto.conf ${D}${sysconfdir}/mosquitto/mosquitto.conf

    install -D -m 0600 ${WORKDIR}/dynamic-security.json ${D}/data/mosquitto/dynamic-security.json
    chown -R mosquitto:mosquitto ${D}/data/mosquitto
}

FILES:${PN}:append = " \
    data/mosquitto/dynamic-security.json \
"
