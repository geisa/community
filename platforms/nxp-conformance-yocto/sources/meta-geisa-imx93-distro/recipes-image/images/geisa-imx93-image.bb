DESCRIPTION = "A production image for geisa-imx93"
require geisa-imx93-common.inc
inherit extrausers

IMAGE_FEATURES:remove = "debug-tweaks"

# Root password for test purpose: geisa-imx93
EXTRA_USERS_PARAMS = "usermod -p '\$1\$Vk6ADbUw\$konIeE8Ynla0l0bI/bvPu0' root;"
