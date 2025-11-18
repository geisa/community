SUMMARY = "Wakaama LWM2M library"
DESCRIPTION = "Wakaama (formerly liblwm2m) is an implementation of the Open Mobile Alliance's LightWeight M2M protocol (LWM2M)."
HOMEPAGE = "https://github.com/eclipse-wakaama/wakaama"
LICENSE = "EPL-2.0 | BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE.EPL-2;md5=6654f12d7f7ba53cf796b622931e86d4 \
                    file://LICENSE.BSD-3-Clause;md5=fdff207498fc09f895880fe73373bae2"

SRC_URI = "gitsm://github.com/eclipse-wakaama/wakaama.git;protocol=https;branch=main"
SRCREV = "${AUTOREV}"

S = "${WORKDIR}/git"

DEPENDS = "autoconf-native automake-native libtool-native pkgconfig-native"

inherit cmake pkgconfig

# Configure Wakaama for client mode with DTLS
EXTRA_OECMAKE = " \
    -DCMAKE_BUILD_TYPE=Release \
    -DWAKAAMA_MODE_CLIENT=ON \
    -DWAKAAMA_DATA_SENML_JSON=ON \
    -DWAKAAMA_DATA_SENML_CBOR=ON \
    -DWAKAAMA_LOG_LEVEL=INFO \
    -DWAKAAMA_TRANSPORT=TINYDTLS \
    -DWAKAAMA_UNIT_TESTS=OFF \
"

EXTRA_OECMAKE:geisa-prod:append = " \
    -DWAKAAMA_ENABLE_EXAMPLES=OFF \
"

do_install() {
    # Install wakaama libraries
    install -d ${D}${libdir}
    install -m 0644 ${B}/libwakaama_static.a ${D}${libdir}/

    # Install tinydtls library
    if [ -f ${B}/external_tinydtls-prefix/src/external_tinydtls-build/.libs/libtinydtls.a ]; then
        install -m 0644 ${B}/external_tinydtls-prefix/src/external_tinydtls-build/.libs/libtinydtls.a ${D}${libdir}/
    fi

    # Install headers
    install -d ${D}${includedir}/wakaama
    install -m 0644 ${S}/include/*.h ${D}${includedir}/wakaama/
    install -m 0644 ${S}/core/*.h ${D}${includedir}/wakaama/

}

FILES:${PN} = ""
FILES:${PN}-staticdev = "${libdir}/*.a"
FILES:${PN}-dev = "${includedir}/wakaama"
