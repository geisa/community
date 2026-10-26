# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2026 PragSol Consulting, LLC
#

SUMMARY = "GEISA mock and reference implementations"
DESCRIPTION = "Opt-in GEISA API mock and its Mosquitto broker configuration."
LICENSE = "Apache-2.0"

inherit packagegroup

RDEPENDS:${PN} = " \
    gapi \
    geisa-api-mock-mosquitto-config \
"
