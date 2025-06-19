SUMMARY = "cukinia-tests"
DESCRIPTION = "Cukinia test files"
HOMEPAGE = "https://github.com/savoirfairelinux/cukinia"
LICENSE = "CLOSED"

SRC_URI = "\
    file://cukinia.conf \
    file://tests.d \
"

inherit allarch

RDEPENDS:${PN} += "cukinia"

do_install () {
    install -m 0755 -d ${D}${sysconfdir}/cukinia/
    install -m 0644 ${WORKDIR}/cukinia.conf ${D}${sysconfdir}/cukinia

    install -m 0755 -d ${D}${sysconfdir}/cukinia/tests.d/
    for file in ${WORKDIR}/tests.d/*; do
        install -m 0644 "${file}" ${D}${sysconfdir}/cukinia/tests.d/
    done
}

do_install:append:geisa-imx93-dbg() {
    rm -rf ${D}${sysconfdir}/cukinia/tests.d/06-security.conf
}
