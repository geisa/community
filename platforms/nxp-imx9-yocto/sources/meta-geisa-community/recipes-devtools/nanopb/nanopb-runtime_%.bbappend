# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2026 PragSol Consulting, LLC
# Website: https://www.pragsolconsulting.com/
#
# Keep the development source interface aligned with the nanopb runtime
# compiled by the base recipe. This does not replace its normal debug-source
# package or create a second source fetch.

PACKAGES += "${PN}-source"
FILES:${PN}-source = "${prefix}/src/nanopb"

do_install:append() {
    install -d ${D}${prefix}/src/nanopb
    for source in \
        pb.h \
        pb_common.c \
        pb_common.h \
        pb_decode.c \
        pb_decode.h \
        pb_encode.c \
        pb_encode.h; do
        install -m 0644 ${S}/$source ${D}${prefix}/src/nanopb/$source
    done
}
