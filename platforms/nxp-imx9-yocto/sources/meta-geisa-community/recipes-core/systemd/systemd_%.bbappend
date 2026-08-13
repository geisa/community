# SPDX-License-Identifier: Apache-2.0
#
FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

PACKAGECONFIG:remove:geisa-container = "sysvinit"

SRC_URI:append = " \
    file://10-geisa-end0.link \
"

do_install:append() {
    if [ -n "${WATCHDOG_TIMEOUT}" ]; then
        sed -i -e 's/#RuntimeWatchdogSec=off/RuntimeWatchdogSec=${WATCHDOG_TIMEOUT}/' \
            ${D}/${sysconfdir}/systemd/system.conf
    fi

    install -d ${D}/${sysconfdir}/systemd/network
    install -m 0644 ${UNPACKDIR}/10-geisa-end0.link \
        ${D}/${sysconfdir}/systemd/network/10-geisa-end0.link
}

FILES:${PN}:append = " ${sysconfdir}/systemd/network/10-geisa-end0.link"
