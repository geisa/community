# SPDX-License-Identifier: Apache-2.0

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRCREV:geisa-imx93-machine = "${GEISA_KERNEL_SRCREV}"
LINUX_VERSION:geisa-imx93-machine = "${GEISA_KERNEL_VERSION}"

SRC_URI:append = " \
    file://container.cfg \
    file://development.cfg \
    file://nxp-6.12.cfg \
    file://unused-configs.cfg \
"

DELTA_KERNEL_DEFCONFIG:geisa-imx93-machine = "container.cfg development.cfg nxp-6.12.cfg unused-configs.cfg"

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
