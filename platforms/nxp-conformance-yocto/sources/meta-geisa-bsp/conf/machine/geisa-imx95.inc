# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2025 Southern California Edison
#

BBMULTICONFIG += "geisa-container"

MACHINEOVERRIDES =. "geisa-imx95-machine:"

BBMASK:append = " \
    meta-imx-frdm/ \
    meta-imx/meta-imx-cockpit \
    meta-imx/meta-imx-ml \
    meta-imx/meta-imx-v2x \
"

WKS_FILE = "geisa-imx-partitioning.wks.in"
