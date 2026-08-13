# SPDX-License-Identifier: Apache-2.0

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

# Whinlatter linux-yocto rejects DELTA_KERNEL_DEFCONFIG. The supported
# kernel-yocto path is to supply ordinary .cfg fragments through SRC_URI.
SRC_URI:append = " \
    file://container.cfg \
    file://development.cfg \
"

DEPENDS:append = " wireless-regdb"
SYSROOT_DIRS:append = " ${nonarch_base_libdir}/firmware"

do_configure:prepend() {
    for firmware in regulatory.db regulatory.db.p7s; do
        source="${RECIPE_SYSROOT}${nonarch_base_libdir}/firmware/${firmware}"
        test -r "$source" || \
            bbfatal "wireless-regdb did not stage required firmware: $source"
        install -D -m 0644 "$source" "${S}/firmware/${firmware}"
    done
}
