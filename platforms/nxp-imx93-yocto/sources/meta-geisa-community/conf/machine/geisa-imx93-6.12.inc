# SPDX-License-Identifier: Apache-2.0

# The 6.12 NXP FRDM machine owns its board-support and M33 defaults.
MACHINE_FEATURES:append = " nxpwifi-all-sdio"
PREFERRED_VERSION_firmware-ele-imx = "2.0.3"
PREFERRED_VERSION_firmware-imx = "8.29"
BBMASK:append = " \
    meta-geisa-community/recipes-kernel/linux/linux-imx_6\\.6\\.bbappend \
    meta-geisa-community/recipes-bsp/u-boot/u-boot-imx_2024\\.04\\.bbappend \
    meta-geisa-community/recipes-libraries/tensorflow-lite/tensorflow-lite-host-tools_2\\.16\\.2\\.bbappend \
"
