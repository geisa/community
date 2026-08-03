# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2025 Southern California Edison
#

FILESEXTRAPATHS:prepend:geisa-imx93-machine := "${THISDIR}/${PN}:"

SRCREV:geisa-imx93-machine = "ce32a3dfd3e47ac0cf339ab35a0baee5857cfbc5"

# Use the refreshed FRDM board-support patch for the 6.6.52 U-Boot source.
SRC_URI:remove:geisa-imx93-machine = "file://0002-imx-imx93_frdm-Add-basic-board-support.patch"
SRC_URI:prepend:geisa-imx93-machine = "file://0002-imx-imx93_frdm-Add-basic-board-support-6.6.52.patch "
