# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2025-2026 Southern California Edison
#

FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append = " \
    file://mosquitto.conf \
    file://dynamic-security.json \
    file://after-lxc-net.conf \
"

do_install:append() {
    install -D -m 0644 ${UNPACKDIR}/mosquitto.conf ${D}${sysconfdir}/mosquitto/mosquitto.conf
    install -D -m 0644 ${UNPACKDIR}/after-lxc-net.conf ${D}${systemd_system_unitdir}/mosquitto.service.d/after-lxc-net.conf

    install -D -m 0600 ${UNPACKDIR}/dynamic-security.json ${D}/platform/mosquitto/dynamic-security.json
    chown -R mosquitto:mosquitto ${D}/platform/mosquitto
}

FILES:${PN}:append = " \
    platform/mosquitto/dynamic-security.json \
    ${systemd_system_unitdir}/mosquitto.service.d/after-lxc-net.conf \
"