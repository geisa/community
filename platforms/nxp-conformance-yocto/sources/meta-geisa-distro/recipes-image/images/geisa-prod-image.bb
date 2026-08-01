# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2025 Southern California Edison
#

DESCRIPTION = "A production image for GEISA"
require geisa-common.inc
inherit extrausers

IMAGE_FEATURES:remove = "debug-tweaks"

# Root password for test purpose
EXTRA_USERS_PARAMS = "usermod -p '\$1\$Vk6ADbUw\$konIeE8Ynla0l0bI/bvPu0' root;"
