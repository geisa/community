# SPDX-License-Identifier: Apache-2.0

IMXBOOT_TARGETS:imx93-11x11-lpddr4x-frdm = "flash_singleboot"
BBMASK:append = " \
    meta-imx-frdm/meta-nxp-connectivity \
    meta-imx-frdm/meta-nxp-demo-experience \
    meta-imx-frdm/meta-nxp-connectivity/recipes-matter/matter/matter.bbappend \
    meta-imx-frdm/meta-nxp-connectivity/recipes-openthread/packagegroups/packagegroup-nxp-openthread.bbappend \
    meta-imx-frdm/meta-nxp-connectivity/recipes-otbr/packagegroups/packagegroup-nxp-otbr.bbappend \
    meta-imx-frdm/meta-nxp-connectivity/recipes-zigbee-rcp/packagegroups/packagegroup-nxp-zigbee-rcp.bbappend \
    meta-imx-frdm/meta-imx-bsp/recipes-connectivity/nxp-wlan-sdk/nxp-wlan-sdk_git.bbappend \
"
PREFERRED_VERSION_firmware-ele-imx = "2.0.3.1"
WKS_FILE_DEPENDS:remove:imx93-11x11-lpddr4x-frdm = "imx-m33-demos"
IMAGE_BOOT_FILES:remove:imx93-11x11-lpddr4x-frdm = " \
    imx93-11x11-evk_m33_TCM_power_mode_switch.bin \
    imx93-11x11-evk_m33_TCM_rpmsg_lite_pingpong_rtos_linux_remote.bin \
    imx93-11x11-evk_m33_TCM_rpmsg_lite_str_echo_rtos.bin \
"
