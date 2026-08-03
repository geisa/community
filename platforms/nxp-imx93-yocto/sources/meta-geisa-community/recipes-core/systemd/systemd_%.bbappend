# SPDX-License-Identifier: Apache-2.0
#
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://geisa-networkd-wait-online.conf \
    file://10-geisa-end0.link \
"

do_install:append() {
    if [ -n "${WATCHDOG_TIMEOUT}" ]; then
        sed -i -e 's/#RuntimeWatchdogSec=off/RuntimeWatchdogSec=${WATCHDOG_TIMEOUT}/' \
            ${D}/${sysconfdir}/systemd/system.conf
    fi

    install -d ${D}/${systemd_system_unitdir}/systemd-networkd-wait-online.service.d
    install -m 0644 ${UNPACKDIR}/geisa-networkd-wait-online.conf \
        ${D}/${systemd_system_unitdir}/systemd-networkd-wait-online.service.d/geisa-networkd-wait-online.conf

    install -d ${D}/${sysconfdir}/systemd/network
    install -m 0644 ${UNPACKDIR}/10-geisa-end0.link \
        ${D}/${sysconfdir}/systemd/network/10-geisa-end0.link
}

FILES:${PN}:append = " ${systemd_system_unitdir}/systemd-networkd-wait-online.service.d/geisa-networkd-wait-online.conf ${sysconfdir}/systemd/network/10-geisa-end0.link"
