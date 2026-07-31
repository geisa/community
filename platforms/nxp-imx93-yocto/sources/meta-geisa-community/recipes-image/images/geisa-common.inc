# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2025 Southern California Edison
#

DESCRIPTION = "A common base for GEISA images"
LICENSE = "Apache-2.0"
require recipes-core/images/core-image-minimal.bb
include ${TOPDIR}/conf/geisa-source-state.conf

# zst compression has the fastest flashing time without a hardware accelerator
IMAGE_FSTYPES += "tar.gz wic.zst wic.bmap"

GEISA_ROOTFS_FIXED_SIZE ?= "1024"
GEISA_PLATFORM_FIXED_SIZE ?= "256"

IMAGE_FEATURES:append = " ssh-server-openssh allow-root-login"

# Keep common images limited to platform substrate. Mock/reference
# implementations belong in packagegroup-geisa-mock-reference or a
# conformance/demo image.
IMAGE_INSTALL:append = " \
    geisa-runtime-defaults \
    geisa-runtime-groups \
    geisa-image-documentation \
    glibc-utils \
    kernel-modules \
    lxc \
    lxc-networking \
    mosquitto \
"

DEPLOY_DIR_GEISA_CONTAINER ?= "${TOPDIR}/tmp-geisa-container/deploy/images/${MACHINE}"

addtask copy_application_base_image before do_image after do_rootfs
do_copy_application_base_image[mcdepends] += "mc::geisa-container:geisa-application-base:do_image_complete"
fakeroot do_copy_application_base_image() {
    install -d ${IMAGE_ROOTFS}/platform/base

    install -m 0644 -o 0 -g 0 ${DEPLOY_DIR_GEISA_CONTAINER}/geisa-application-base-${MACHINE}.rootfs.squashfs ${IMAGE_ROOTFS}/platform/base/
    install -m 0644 -o 0 -g 0 ${DEPLOY_DIR_GEISA_CONTAINER}/geisa-application-base-${MACHINE}.rootfs.tar.gz ${IMAGE_ROOTFS}/platform/base/

    # This image-owned marker is the first authority for GEISA validation.
    # It intentionally describes only seeded, immutable image inputs.
    install -d -m 0755 ${IMAGE_ROOTFS}${sysconfdir}/geisa
    app_base_tar="${IMAGE_ROOTFS}/platform/base/geisa-application-base-${MACHINE}.rootfs.tar.gz"
    app_base_squashfs="${IMAGE_ROOTFS}/platform/base/geisa-application-base-${MACHINE}.rootfs.squashfs"
    profile_root="${IMAGE_ROOTFS}/platform/profiles/nxp-ethosu-tflite"
    build_timestamp="$(date -u +%Y-%m-%dT%H:%M:%SZ)"
    app_base_tar_sha256="$(sha256sum "$app_base_tar" | awk '{print $1}')"
    app_base_squashfs_sha256="$(sha256sum "$app_base_squashfs" | awk '{print $1}')"
    if [ -d "$profile_root" ]; then
        profile_tree_sha256="$(find "$profile_root" -type f -print | LC_ALL=C sort | xargs sha256sum | sha256sum | awk '{print $1}')"
    else
        profile_tree_sha256="absent"
    fi
    cat >${IMAGE_ROOTFS}${sysconfdir}/geisa/image-manifest.json <<EOF
{"schema":"geisa.image-manifest.v1","image_name":"${IMAGE_BASENAME}","image_recipe":"${PN}","machine":"${MACHINE}","distro":"${DISTRO}","build_timestamp":"$build_timestamp","wks_file":"${WKS_FILE}","partitions":{"boot_fixed_size_mib":256,"rootfs_fixed_size_mib":${GEISA_ROOTFS_FIXED_SIZE},"platform_fixed_size_mib":${GEISA_PLATFORM_FIXED_SIZE}},"application_base":{"tar":{"path":"/platform/base/geisa-application-base-${MACHINE}.rootfs.tar.gz","sha256":"$app_base_tar_sha256"},"squashfs":{"path":"/platform/base/geisa-application-base-${MACHINE}.rootfs.squashfs","sha256":"$app_base_squashfs_sha256"}},"profiles":[{"path":"/platform/profiles/nxp-ethosu-tflite","tree_sha256":"$profile_tree_sha256"}],"fixtures":[{"path":"/platform/inputs/vision/people-detection.mp4","sha256":"18ffe8672d741e3e29c9d891d22c59d453720b086c25b35c88b393d55f92f693","seeded":false,"policy":"validation-staged-not-image-seeded"}],"package_cache_seeded":[],"documentation":{"package":"geisa-image-documentation","version":"1.0"},"yocto":{"source_revision":"${GEISA_SOURCE_REVISION}","source_dirty":${GEISA_SOURCE_DIRTY},"source_dirty_digest":"${GEISA_SOURCE_DIRTY_DIGEST}"}}
EOF
    chmod 0644 ${IMAGE_ROOTFS}${sysconfdir}/geisa/image-manifest.json
    cat >${IMAGE_ROOTFS}${sysconfdir}/geisa-image-release <<EOF
IMAGE_NAME=${IMAGE_BASENAME}
IMAGE_VERSION=${PV}
BUILD_TIMESTAMP=$build_timestamp
MACHINE=${MACHINE}
DISTRO=${DISTRO}
SOURCE_REVISION=${GEISA_SOURCE_REVISION}
SOURCE_DIRTY=${GEISA_SOURCE_DIRTY}
SOURCE_DIRTY_DIGEST=${GEISA_SOURCE_DIRTY_DIGEST}
WKS_FILE=${WKS_FILE}
EOF
    chmod 0644 ${IMAGE_ROOTFS}${sysconfdir}/geisa-image-release
}

# Create /platform buckets for application packages, base rootfs closures,
# profiles, inputs/media, logs, state, and implementer extensions.
ROOTFS_POSTPROCESS_COMMAND += "create_platform_layout;"
create_platform_layout() {
    install -d -m 0755 ${IMAGE_ROOTFS}/platform/base
    install -d -m 0755 ${IMAGE_ROOTFS}/platform/profiles
    install -d -m 0755 ${IMAGE_ROOTFS}/platform/inputs

    geisa_gid="$(awk -F: '$1 == "geisa" { print $3; exit }' ${IMAGE_ROOTFS}${sysconfdir}/group)"
    if [ -z "$geisa_gid" ]; then
        bbfatal "geisa group missing from image rootfs"
    fi

    if [ -d ${IMAGE_ROOTFS}/platform/profiles/nxp-ethosu-tflite ]; then
        chown -R 0:${geisa_gid} ${IMAGE_ROOTFS}/platform/profiles/nxp-ethosu-tflite
        chmod -R go-w ${IMAGE_ROOTFS}/platform/profiles/nxp-ethosu-tflite
    fi
}

ROOTFS_POSTPROCESS_COMMAND += "create_geisa_runtime_layout;"
create_geisa_runtime_layout() {
    geisa_gid="$(awk -F: '$1 == "geisa" { print $3; exit }' ${IMAGE_ROOTFS}${sysconfdir}/group)"
    [ -n "$geisa_gid" ] || bbfatal "geisa group missing from image rootfs"
    install -d -m 0755 ${IMAGE_ROOTFS}/opt/geisa
    chown 0:0 ${IMAGE_ROOTFS}/opt/geisa
    install -d -m 0755 ${IMAGE_ROOTFS}${localstatedir}/lib/geisa
    chown 0:0 ${IMAGE_ROOTFS}${localstatedir}/lib/geisa
    for directory in apps packages config state persist; do
        install -d -m 0755 ${IMAGE_ROOTFS}${localstatedir}/lib/geisa/$directory
        chown 0:0 ${IMAGE_ROOTFS}${localstatedir}/lib/geisa/$directory
    done
    install -d -m 2775 ${IMAGE_ROOTFS}${localstatedir}/lib/geisa/eventlog
    chown 0:$geisa_gid ${IMAGE_ROOTFS}${localstatedir}/lib/geisa/eventlog
}

ROOTFS_POSTUNINSTALL_COMMAND += "install_geisa_package_manifest"
install_geisa_package_manifest() {
    [ -s "${IMAGE_MANIFEST}" ] || bbfatal "generated package manifest unavailable: ${IMAGE_MANIFEST}"
    install -d -m 0755 ${IMAGE_ROOTFS}${datadir}/doc/geisa-image
    install -m 0644 ${IMAGE_MANIFEST} ${IMAGE_ROOTFS}${datadir}/doc/geisa-image/packages.manifest
}

addtask deploy_geisa_image_manifest after do_image_complete before do_build
do_deploy_geisa_image_manifest() {
    source="${IMAGE_ROOTFS}${sysconfdir}/geisa/image-manifest.json"
    [ -s "$source" ] || bbfatal "image manifest unavailable after image completion: $source"
    install -d ${DEPLOY_DIR_IMAGE}
    install -m 0644 "$source" ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}.image-manifest.json
}

ROOTFS_POSTPROCESS_COMMAND += "geisa_assert_no_baked_ssh_authorized_keys;"
geisa_assert_no_baked_ssh_authorized_keys() {
    baked_keys="$(find ${IMAGE_ROOTFS}/root ${IMAGE_ROOTFS}/home -path '*/.ssh/authorized_keys' -type f -size +0c -print 2>/dev/null || true)"
    if [ -n "$baked_keys" ]; then
        bbfatal "Refusing to build image with baked SSH authorized_keys: $baked_keys"
    fi
}
