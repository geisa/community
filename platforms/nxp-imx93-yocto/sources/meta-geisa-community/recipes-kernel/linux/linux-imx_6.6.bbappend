# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2025 Southern California Edison
#

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRCREV:geisa-imx93-machine = "5a0a5e71d2bd130577c7330cbb3764c68ebcb3dc"
LINUX_VERSION:geisa-imx93-machine = "6.6.52"

# The 6.6.52 NXP source already contains upstream commit e2ecdddca80dd845df42376e4b0197fe97018ba2, which supersedes this FRDM-layer copy.
SRC_URI:remove:geisa-imx93-machine = "file://0001-gpio-pca953x-fix-pca953x_irq_bus_sync_unlock-race.patch"
# The 6.6.52 NXP source already contains commit 8cf4ccdc7a9bd8840d6ed8a372895469d1aa3063, which supersedes this older FRDM-layer patch.
SRC_URI:remove:geisa-imx93-machine = "file://0020-LF-13459-clk-imx-Fix-the-pll-power-up-flow.patch"
# The target source uses upstream commit 8835cd9bec02a5597f1d9ffcbde594136a4d4b3e, which already reads the signed 16-bit value correctly.
SRC_URI:remove:geisa-imx93-machine = "file://0021-thermal-imx91-bug-fix-Temperature-read-returns-Resou.patch"
# This patch is i.MX91-only and does not belong in the geisa-imx93 patch set.
SRC_URI:remove:geisa-imx93-machine = "file://0022-LF-14498-arm64-dts-imx91-Correct-ENET1_TD3-and-I2C2_.patch"

SRC_URI:append = " \
    file://container.cfg \
    file://development.cfg \
    file://unused-configs.cfg \
"

# The NXP 6.6.52 recipe copies imx_v8_defconfig after kernel-yocto merges
# SRC_URI fragments. Reapply the platform fragments through its supported
# delta-config hook so the final .config retains them.
DELTA_KERNEL_DEFCONFIG:geisa-imx93-machine = "container.cfg development.cfg unused-configs.cfg"

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
