# SPDX-License-Identifier: Apache-2.0

SUMMARY = "GEISA i.MX95 Neutron development dependencies"
DESCRIPTION = "Minimal target runtime dependencies for FRDM-i.MX95 Neutron."
LICENSE = "Apache-2.0"

inherit packagegroup

RDEPENDS:${PN} = " \
    neutron \
    tensorflow-lite-neutron-delegate \
    geisa-neutron-device-permissions \
    geisa-neutron-smoke-test \
"
