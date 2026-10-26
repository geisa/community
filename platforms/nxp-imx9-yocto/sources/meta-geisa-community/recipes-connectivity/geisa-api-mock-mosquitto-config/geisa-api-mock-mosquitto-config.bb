# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2026 PragSol Consulting, LLC
#

SUMMARY = "GEISA API mock Mosquitto configuration"
DESCRIPTION = "Opt-in listener and dynamic-security configuration for the GEISA API mock."
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/Apache-2.0;md5=89aea4e17d99a7cacdbeed46a0096b10"

SRC_URI = " \
    file://geisa-api-mock.conf \
    file://dynamic-security.json \
    file://10-geisa-api-mock.conf \
"
S = "${UNPACKDIR}"

inherit systemd

RDEPENDS:${PN} = "mosquitto"
SYSTEMD_SERVICE:${PN} = ""

do_install() {
    install -D -m 0644 ${UNPACKDIR}/geisa-api-mock.conf ${D}${sysconfdir}/mosquitto/geisa-api-mock.conf
    install -D -m 0600 ${UNPACKDIR}/dynamic-security.json ${D}/platform/mosquitto/dynamic-security.json
    install -D -m 0644 ${UNPACKDIR}/10-geisa-api-mock.conf ${D}${systemd_system_unitdir}/mosquitto.service.d/10-geisa-api-mock.conf
}

pkg_postinst:${PN}() {
    if [ -n "$D" ]; then
        mosquitto_uid="$(awk -F: '$1 == "mosquitto" { print $3; exit }' "$D${sysconfdir}/passwd")"
        mosquitto_gid="$(awk -F: '$1 == "mosquitto" { print $3; exit }' "$D${sysconfdir}/group")"
        if [ -z "$mosquitto_uid" ] || [ -z "$mosquitto_gid" ]; then
            echo "mosquitto user or group is missing from the target rootfs" >&2
            exit 1
        fi
        chown -R "$mosquitto_uid:$mosquitto_gid" "$D/platform/mosquitto"
    else
        chown -R mosquitto:mosquitto /platform/mosquitto
        if command -v systemctl >/dev/null 2>&1 && systemctl is-active --quiet mosquitto.service; then
            systemctl daemon-reload && systemctl restart mosquitto.service
        fi
    fi
}

FILES:${PN} += " \
    ${sysconfdir}/mosquitto/geisa-api-mock.conf \
    /platform/mosquitto/dynamic-security.json \
    ${systemd_system_unitdir}/mosquitto.service.d/10-geisa-api-mock.conf \
"

CONFFILES:${PN} += " \
    ${sysconfdir}/mosquitto/geisa-api-mock.conf \
    /platform/mosquitto/dynamic-security.json \
"
