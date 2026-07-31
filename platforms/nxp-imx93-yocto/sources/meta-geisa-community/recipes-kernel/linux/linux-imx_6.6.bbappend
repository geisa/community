# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2025 Southern California Edison
#

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRCREV:geisa-imx93-machine = "d23d64eea5111e1607efcce1d601834fceec92cb"
LINUX_VERSION:geisa-imx93-machine = "6.6.36"

SRC_URI:append = " \
    file://container.cfg \
    file://development.cfg \
    file://unused-configs.cfg \
"

# cfg80211 is built into the FRDM development kernel and requests its signed
# regulatory database before the root filesystem is mounted. Stage the exact
# database supplied by wireless-regdb into the kernel source so Kconfig can
# embed it in Image. The dependency and staged files participate in task
# signatures; the rootfs package remains available for runtime inspection.
DEPENDS:append:geisa-imx93-machine = " wireless-regdb"
SYSROOT_DIRS:append:geisa-imx93-machine = " ${nonarch_base_libdir}/firmware"

do_configure:prepend:geisa-imx93-machine() {
    for firmware in regulatory.db regulatory.db.p7s; do
        source="${RECIPE_SYSROOT}${nonarch_base_libdir}/firmware/${firmware}"
        test -r "$source" || \
            bbfatal "wireless-regdb did not stage required firmware: $source"
        install -D -m 0644 "$source" "${S}/firmware/${firmware}"
    done
}
