# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2025 Southern California Edison
#

BBMULTICONFIG += "geisa-container"

MACHINEOVERRIDES =. "geisa-imx93-machine:"

BBMASK:append = " \
    meta-imx/ \
    meta-imx-frdm/meta-nxp-connectivity \
    meta-imx-frdm/meta-nxp-demo-experience \
"
