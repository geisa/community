# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2025 Southern California Edison
#

FILESEXTRAPATHS:prepend := "${THISDIR}/${PN}:"

SRCREV:geisa-imx93-machine = "d23d64eea5111e1607efcce1d601834fceec92cb"
LINUX_VERSION:geisa-imx93-machine = "6.6.36"

SRC_URI:append = " \
    file://container.cfg \
    file://unused-configs.cfg \
"