DESCRIPTION = "A debug image for GEISA"
require geisa-common.inc

# Monitoring tools
IMAGE_INSTALL += "htop"

# Network tools
IMAGE_INSTALL += "tcpdump"

# System tools
IMAGE_FEATURES += "allow-empty-password debug-tweaks empty-root-password \
                   post-install-logging"

# Clear REPRODUCIBLE_TIMESTAMP_ROOTFS variable to get the build time in /etc/version
# An NFS CI requires a timestamp to distinguish boots.
unset REPRODUCIBLE_TIMESTAMP_ROOTFS
