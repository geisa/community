# SPDX-License-Identifier: Apache-2.0

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRCREV:geisa-imx93-machine = "be78e49cb4339fd38c9a40019df49b72fbb8bcb7"
LINUX_VERSION:geisa-imx93-machine = "6.12.34"

SRC_URI:append = " \
    file://container.cfg \
    file://development.cfg \
    file://unused-configs.cfg \
"

DELTA_KERNEL_DEFCONFIG:geisa-imx93-machine = "container.cfg development.cfg unused-configs.cfg"

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
