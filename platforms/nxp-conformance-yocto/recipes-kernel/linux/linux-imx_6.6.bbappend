FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRCREV = "d23d64eea5111e1607efcce1d601834fceec92cb"
LINUX_VERSION = "6.6.36"

SRC_URI:append = " \
    file://container.cfg \
"
