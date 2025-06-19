#!/bin/bash

set -ex

if [ -z "${DEPLOY_METHOD}" ]; then
    DEPLOY_METHOD="NFS"
fi
if [ -z "${DEPLOY_USER}" ]; then
    DEPLOY_USER=${USER}
fi
if [ -z "${DEPLOY_DIR_IMAGE}" ]; then
    DEPLOY_DIR_IMAGE="build/tmp/deploy/images"
fi

SERVER_IP=clarisse
TARGET_HOSTNAME="geisa-imx93"
TARGET_DEPLOY_DIR_IMAGE="$DEPLOY_DIR_IMAGE/geisa-imx93"
# NFS/TFTP files
TARGET_IMAGE="Image"
TARGET_DTB="geisa-imx93.dtb"
TARGET_ROOTFS="geisa-imx93-dbg-image-geisa-imx93"
if [ -n "${ROOTFS_VERSION}" ]; then
    ROOTFS_VERSION=-${ROOTFS_VERSION}
fi
DEPLOY_PREFIX=${TARGET_HOSTNAME}-${DEPLOY_USER}
# SWUPDATE files
SWU_IMAGE="${TARGET_ROOTFS}.swu"
SWU_PUBKEY=""

if [ "${DEPLOY_METHOD}" = "NFS" ]; then
    scp "${TARGET_DEPLOY_DIR_IMAGE}/${TARGET_IMAGE}" "${SERVER_IP}:/srv/tftpboot/${DEPLOY_PREFIX}-${TARGET_IMAGE}"
    scp "${TARGET_DEPLOY_DIR_IMAGE}/${TARGET_DTB}" "${SERVER_IP}:/srv/tftpboot/${DEPLOY_PREFIX}-${TARGET_DTB}"

    # It's way faster to copy the archive and extract it on the server than extract then copy each file
    scp "${TARGET_DEPLOY_DIR_IMAGE}/${TARGET_ROOTFS}.tar.gz" "${SERVER_IP}:/srv/nfsroot/${DEPLOY_PREFIX}-${TARGET_ROOTFS}.tar.gz"
    ssh ${SERVER_IP} << heredoc
        set -ex
        cd /srv/nfsroot
        sudo rm -rf "${DEPLOY_PREFIX}-${TARGET_ROOTFS}${ROOTFS_VERSION}"
        mkdir -p "${DEPLOY_PREFIX}-${TARGET_ROOTFS}${ROOTFS_VERSION}"
        sudo tar --same-owner --preserve-permissions \
            -xf "${DEPLOY_PREFIX}-${TARGET_ROOTFS}.tar.gz" \
            -C "${DEPLOY_PREFIX}-${TARGET_ROOTFS}${ROOTFS_VERSION}"
heredoc

elif [ "${DEPLOY_METHOD}" = "SWUpdate" ]; then
    if [ -z "${SWU_PUBKEY}" ]; then
        scp $SSH_OPTS "$TARGET_DEPLOY_DIR_IMAGE/$SWU_IMAGE" "root@$TARGET_IP:/tmp/"
        ssh $SSH_OPTS "root@$TARGET_IP" swupdate -i "/tmp/$SWU_IMAGE"
    else
        scp $SSH_OPTS "$TARGET_DEPLOY_DIR_IMAGE/$SWU_IMAGE" "yocto-build/keys/$SWU_PUBKEY" "root@$TARGET_IP:/tmp/"
        ssh $SSH_OPTS "root@$TARGET_IP" swupdate -i "/tmp/$SWU_IMAGE" --ca-path "/tmp/$SWU_PUBKEY"
    fi
fi
