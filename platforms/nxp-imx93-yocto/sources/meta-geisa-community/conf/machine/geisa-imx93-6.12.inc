# SPDX-License-Identifier: Apache-2.0

# The 6.12 NXP FRDM machine owns its board-support and M33 defaults.
MACHINE_FEATURES:append = " nxpwifi-all-sdio"
PREFERRED_VERSION_firmware-ele-imx = "2.0.3"
PREFERRED_VERSION_firmware-imx = "8.29"
GEISA_TFLITE_RUNTIME_VERSION = "2.19.0"
GEISA_KERNEL_VERSION = "6.12.34"
GEISA_KERNEL_SRCREV = "be78e49cb4339fd38c9a40019df49b72fbb8bcb7"
GEISA_UBOOT_VERSION = "2025.04"
GEISA_UBOOT_SRCREV = "44898b9f3cfe5ff881c11ab1c44b714041f2b4ac"
