# SPDX-License-Identifier: LGPL-2.1-or-later

SUMMARY = "Asynchronous name service library"
DESCRIPTION = "Asynchronous DNS resolution library for applications"
HOMEPAGE = "https://0pointer.de/lennart/projects/libasyncns/"
SECTION = "libs"
LICENSE = "LGPL-2.1-or-later"
LIC_FILES_CHKSUM = "file://LICENSE;md5=fad9b3332be894bab9bc501572864b29"

SRC_URI = "https://deb.debian.org/debian/pool/main/liba/libasyncns/libasyncns_${PV}.orig.tar.gz"
SRC_URI[sha256sum] = "4f1a66e746cbe54ff3c2fbada5843df4fbbbe7481d80be003e8d11161935ab74"

S = "${UNPACKDIR}/libasyncns-${PV}"

inherit autotools pkgconfig

EXTRA_OECONF = "--disable-static"
