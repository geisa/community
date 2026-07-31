# BlueZ packages /etc/bluetooth as 0755 while bluetooth.service requests 0555.
# Match the service's ConfigurationDirectory contract to the packaged directory.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI += "file://geisa-bluetooth.conf"

do_install:append() {
    install -D -m 0644 ${WORKDIR}/geisa-bluetooth.conf \
        ${D}${systemd_system_unitdir}/bluetooth.service.d/geisa.conf
}

FILES:${PN} += "${systemd_system_unitdir}/bluetooth.service.d/geisa.conf"
