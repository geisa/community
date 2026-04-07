SUMMARY = "GEISA Wakaama LwM2M ADM client"
LICENSE = "Apache-2.0 & BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE.BSD-3-Clause;md5=fdff207498fc09f895880fe73373bae2 \
                    file://LICENSE.Apache-2.0;md5=3b83ef96387f14655fc854ddc3c6bd57"

DEPENDS = "wakaama"

SRC_URI = " \
    git://github.com/geisa/adm-mockup.git;protocol=https;branch=main \
"

SRCREV = "cf821dec06efa6fe56693fcf86067180fbd074a2"

RDEPENDS:${PN} = "e2fsprogs-mke2fs squashfs-tools"

S = "${WORKDIR}/git"

inherit cmake

EXTRA_OECMAKE = " \
    -DWAKAAMA_INCLUDE_DIR=${STAGING_INCDIR}/wakaama \
    -DWAKAAMA_TINYDTLS_INCLUDE_DIR=${STAGING_INCDIR}/tinydtls \
    -DWAKAAMA_LIB_DIR=${STAGING_LIBDIR} \
"

do_install() {
    install -d ${D}${bindir}
    install -m 0755 ${B}/adm_client ${D}${bindir}/

    install -d ${D}${sbindir}
    install -m 0755 ${S}/sources/scripts/manage_package.sh ${D}${sbindir}/
}

FILES:${PN} = " \
    ${bindir}/adm_client \
    ${sbindir}/manage_package.sh \
"
