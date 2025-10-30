FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRC_URI:append = " \
    file://pfsense_ntp.conf \
"

do_install:append() {
    if [ -n "${WATCHDOG_TIMEOUT}" ]; then
        sed -i -e 's/#RuntimeWatchdogSec=off/RuntimeWatchdogSec=${WATCHDOG_TIMEOUT}/' \
            ${D}/${sysconfdir}/systemd/system.conf
    fi

    install -d ${D}/${sysconfdir}/systemd/timesyncd.conf.d
    install -m 0644 ${WORKDIR}/pfsense_ntp.conf ${D}/${sysconfdir}/systemd/timesyncd.conf.d/
}
