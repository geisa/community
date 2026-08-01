SUMMARY = "GEISA Wakaama LwM2M ADM client"
LICENSE = "Apache-2.0 & BSD-3-Clause"
LIC_FILES_CHKSUM = "file://LICENSE.BSD-3-Clause;md5=9c1fba5d05e6bcd61ed03aa4dfb5d31e \
                    file://LICENSE.Apache-2.0;md5=3b83ef96387f14655fc854ddc3c6bd57"

DEPENDS = "wakaama"

SRC_URI = " \
    git://github.com/geisa/community.git;protocol=https;branch=main \
"

SRCREV = "${AUTOREV}"
S = "${WORKDIR}/git/example-implementations/adm/lwm2m-client/wakaama"

RDEPENDS:${PN} = "e2fsprogs-mke2fs squashfs-tools"


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
