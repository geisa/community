# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2026 PragSol Consulting, LLC
#

SUMMARY = "GEISA i.MX93 Ethos-U development dependencies"
DESCRIPTION = "Target runtime and validation dependencies for FRDM-i.MX93 Ethos-U."
LICENSE = "Apache-2.0"

inherit packagegroup

# Keep this packagegroup limited to package names that are safe in allarch
# packagegroups. Dynamically renamed shlib packages, such as tensorflow-lite,
# are installed directly by geisa-dev-image instead.
RDEPENDS:${PN} = " \
    ethos-u-driver-stack \
    ethos-u-firmware \
    ethos-u-vela \
    geisa-ethosu-device-permissions \
    geisa-ethosu-smoke-test \
    tensorflow-lite-ethosu-delegate \
"
