# SPDX-License-Identifier: Apache-2.0

GEISA_ACCELERATOR = "neutron"
GEISA_ACCELERATOR_PACKAGEGROUP = "packagegroup-geisa-imx95-neutron-dev"
GEISA_ACCELERATOR_RUNTIME = "tensorflow-lite-neutron-delegate"
GEISA_ACCELERATOR_DEVICE_GROUP = "neutron"
GEISA_ACCELERATOR_PROFILE = ""
GEISA_ACCELERATOR_PROFILE_PATH = ""

# meta-freescale also provides an imx95-evk.inc include whose legacy
# _evk_defconfig suffix does not match the FRDM U-Boot config in NXP's
# Whinlatter source. Keep the FRDM SD build explicit here.
UBOOT_CONFIG[sd] = "${UBOOT_CONFIG_BASENAME}_defconfig,sdcard"
