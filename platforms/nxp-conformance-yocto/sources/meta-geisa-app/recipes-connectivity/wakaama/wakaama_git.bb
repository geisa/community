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
    -DWAKAAMA_MODE_SERVER=OFF \
    -DWAKAAMA_MODE_BOOTSTRAP_SERVER=OFF \
    -DWAKAAMA_CLIENT_INITIATED_BOOTSTRAP=ON \
    -DWAKAAMA_DATA_SENML_JSON=ON \
    -DWAKAAMA_DATA_SENML_CBOR=ON \
    -DWAKAAMA_LOG_LEVEL=INFO \
    -DWAKAAMA_TRANSPORT=TINYDTLS \
    -DWAKAAMA_UNIT_TESTS=OFF \
    -DWAKAAMA_CLI=OFF \
    -DWAKAAMA_PLATFORM=POSIX \
"

EXTRA_OECMAKE:append:geisa-prod = " \
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

    # Install Wakaama headers
    install -d ${D}${includedir}/wakaama
    install -m 0644 ${S}/include/*.h ${D}${includedir}/wakaama/
    install -m 0644 ${S}/core/*.h ${D}${includedir}/wakaama/

    # Install TinyDTLS public headers
    install -d ${D}${includedir}/tinydtls/include/tinydtls
    install -m 0644 ${S}/transport/tinydtls/include/tinydtls/*.h ${D}${includedir}/tinydtls/include/tinydtls/

    # Install TinyDTLS third_party headers
    install -d ${D}${includedir}/tinydtls/third_party/tinydtls
    find ${S}/transport/tinydtls/third_party/tinydtls -type f -name '*.h' | while IFS= read -r header; do
        relpath=${header#${S}/transport/tinydtls/third_party/tinydtls/}
        install -d ${D}${includedir}/tinydtls/third_party/tinydtls/$(dirname "$relpath")
        install -m 0644 "$header" ${D}${includedir}/tinydtls/third_party/tinydtls/"$relpath"
    done
}

FILES:${PN} = ""
FILES:${PN}-staticdev = "${libdir}/*.a"
FILES:${PN}-dev = "${includedir}/wakaama ${includedir}/tinydtls"
