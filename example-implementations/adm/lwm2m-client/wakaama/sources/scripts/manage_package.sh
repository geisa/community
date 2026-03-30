#!/bin/sh

# Usage:
#   manage_package.sh install <package_name> <package_version> <package_id> <persistent_size_kib> <tmpfs_size_kib>
#   manage_package.sh uninstall <package_name> <package_version>
#   manage_package.sh verify <package_name> <package_version> <package_manifest> <signature>
#   manage_package.sh activate <package_name>
#   manage_package.sh deactivate <package_name>

set -e

MODE="$1"
PACKAGE_NAME="$2"
shift 2

LXC_ROOT_DIR="/platform/lxc/${PACKAGE_NAME}"
LXC_BASE_DIR="${LXC_ROOT_DIR}/base"
LXC_CONFIG_DIR="${LXC_ROOT_DIR}/cfg"
LXC_APP_DIR="${LXC_ROOT_DIR}/app"
LXC_PERSISTENT_DIR="${LXC_ROOT_DIR}/persistent"
LXC_WORK_DIR="${LXC_ROOT_DIR}/work"
LXC_ROOTFS_DIR="${LXC_ROOT_DIR}/rootfs"

register_app_to_mqtt_broker() {
    local appid="$1"
    local token="$2"
    local mosq_ctrl="mosquitto_ctrl -u admin -P admin dynsec"

    $mosq_ctrl createClient "$appid" -p "$token"

    $mosq_ctrl addGroupClient GeisaApps "$appid"

    local role="AllowGeisa${appid}Topics"
    $mosq_ctrl createRole "$role"

    $mosq_ctrl addRoleACL "$role" publishClientSend "geisa/api/app/manifest/req/$appid" allow
    $mosq_ctrl addRoleACL "$role" subscribeLiteral "geisa/api/app/manifest/rsp/$appid" allow
    $mosq_ctrl addRoleACL "$role" subscribeLiteral "geisa/api/app/platform/status/$appid" allow
    $mosq_ctrl addRoleACL "$role" publishClientSend "geisa/api/waveform/req/$appid" allow
    $mosq_ctrl addRoleACL "$role" subscribeLiteral "geisa/api/waveform/rsp/$appid" allow
    $mosq_ctrl addRoleACL "$role" publishClientSend "geisa/api/message/req/$appid" allow
    $mosq_ctrl addRoleACL "$role" subscribeLiteral "geisa/api/message/rsp/$appid" allow
    $mosq_ctrl addRoleACL "$role" subscribeLiteral "geisa/api/platform/app/status/$appid" allow
    $mosq_ctrl addRoleACL "$role" subscribeLiteral "geisa/api/platform/discovery/req/$appid" allow
    $mosq_ctrl addRoleACL "$role" publishClientSend "geisa/api/platform/discovery/rsp/$appid" allow

    $mosq_ctrl addClientRole "$appid" "$role"
}

deregister_app_to_mqtt_broker() {
    local appid="$1"

    local role="AllowGeisa${appid}Topics"

    local mosq_ctrl="mosquitto_ctrl -u admin -P admin dynsec"

    $mosq_ctrl removeGroupClient GeisaApps "$appid"
    $mosq_ctrl removeClientRole "$appid" "$role"

    $mosq_ctrl removeRoleACL "$role" publishClientSend "geisa/api/app/manifest/req/$appid"
    $mosq_ctrl removeRoleACL "$role" subscribeLiteral "geisa/api/app/manifest/rsp/$appid"
    $mosq_ctrl removeRoleACL "$role" subscribeLiteral "geisa/api/app/platform/status/$appid"
    $mosq_ctrl removeRoleACL "$role" publishClientSend "geisa/api/waveform/req/$appid"
    $mosq_ctrl removeRoleACL "$role" subscribeLiteral "geisa/api/waveform/rsp/$appid"
    $mosq_ctrl removeRoleACL "$role" publishClientSend "geisa/api/message/req/$appid"
    $mosq_ctrl removeRoleACL "$role" subscribeLiteral "geisa/api/message/rsp/$appid"
    $mosq_ctrl removeRoleACL "$role" subscribeLiteral "geisa/api/platform/app/status/$appid"
    $mosq_ctrl removeRoleACL "$role" subscribeLiteral "geisa/api/platform/discovery/req/$appid"
    $mosq_ctrl removeRoleACL "$role" publishClientSend "geisa/api/platform/discovery/rsp/$appid"


    $mosq_ctrl deleteRole "$role"
    $mosq_ctrl deleteClient "$appid"
}

case "$MODE" in
    install)
        PACKAGE_VERSION="$1"
        PACKAGE_ID="$2"
        PERSISTENT_SIZE_KIB="${3:-131072}"
        TMPFS_SIZE_KIB="${4:-131072}"

        if [ -z "$PACKAGE_NAME" ] || [ -z "$PACKAGE_ID" ] || [ -z "$PERSISTENT_SIZE_KIB" ] || [ -z "$TMPFS_SIZE_KIB" ]; then
            echo "Usage: $0 install <package_name> <package_version> <package_id> <persistent_size_kib> <tmpfs_size_kib> [base_image.squashfs]"
            exit 1
        fi

        echo "Installing package: $PACKAGE_NAME version: $PACKAGE_VERSION with ID: $PACKAGE_ID"

        BASE_IMAGE="/platform/base/geisa-application-base-$(uname -n).rootfs.squashfs"

        PACKAGE_DIR="/platform/apps/${PACKAGE_NAME}"
        PACKAGE_IMAGE="${PACKAGE_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.squashfs"

        if [ ! -f "$PACKAGE_IMAGE" ]; then
            echo "Package image not found: $PACKAGE_IMAGE"
            exit 2
        fi

        if [ ! -f "$BASE_IMAGE" ]; then
            echo "Base image not found: $BASE_IMAGE"
            exit 3
        fi

        mkdir -p "${LXC_ROOT_DIR}"
        mkdir -p "${LXC_PERSISTENT_DIR}"
        mkdir -p "${LXC_WORK_DIR}"
        mkdir -p "${LXC_ROOTFS_DIR}"
        mkdir -p "${LXC_BASE_DIR}"
        mkdir -p "${LXC_APP_DIR}"
        mkdir -p "${LXC_CONFIG_DIR}"

        # Prepare config image for application (with a minimal GEISA MQTT config)
        CONFIG_SRC="/tmp/${PACKAGE_NAME}-${PACKAGE_VERSION}/cfg"
        mkdir -p "$CONFIG_SRC/opt/geisa"
        PACKAGE_TOKEN=$(openssl rand -hex 32)
        cat > "$CONFIG_SRC/opt/geisa/mqtt.conf" <<EOF
GEISA_MQTT_HOST="127.0.0.1"
GEISA_MQTT_PORT="1883"
GEISA_PACKAGE_ID="${PACKAGE_ID}"
GEISA_PACKAGE_TOKEN="${PACKAGE_TOKEN}"
EOF

        CONFIG_IMAGE="${PACKAGE_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}-config.squashfs"

        mksquashfs "$CONFIG_SRC" "$CONFIG_IMAGE" -noappend > /dev/null
        rm -rf "$CONFIG_SRC"
        chmod 600 "$PACKAGE_IMAGE"

        if ! mountpoint -q "${LXC_BASE_DIR}"; then
            mount -t squashfs "$BASE_IMAGE" "${LXC_BASE_DIR}"
        fi
        if ! mountpoint -q "${LXC_APP_DIR}"; then
            mount -t squashfs "$PACKAGE_IMAGE" "${LXC_APP_DIR}"
        fi
        if ! mountpoint -q "${LXC_CONFIG_DIR}"; then
            mount -t squashfs "$CONFIG_IMAGE" "${LXC_CONFIG_DIR}"
        fi
        if ! mountpoint -q "${LXC_ROOTFS_DIR}"; then
            mount -t overlay overlay \
                -olowerdir="${LXC_BASE_DIR}:${LXC_APP_DIR}:${LXC_CONFIG_DIR}",upperdir="$LXC_PERSISTENT_DIR",workdir="$LXC_WORK_DIR" \
                "$LXC_ROOTFS_DIR"
        fi

        mkdir -p "${LXC_ROOTFS_DIR}/tmp"

        if ! mountpoint -q "${LXC_ROOTFS_DIR}/tmp"; then
            mount -t tmpfs -o size="${TMPFS_SIZE_KIB}k" tmpfs "${LXC_ROOTFS_DIR}/tmp"
        fi

        mkdir -p "/platform/persistent"
        if [ ! -f "/platform/persistent/${PACKAGE_NAME}-${PACKAGE_VERSION}.img" ]; then
            dd if=/dev/zero of="/platform/persistent/${PACKAGE_NAME}-${PACKAGE_VERSION}.img" bs=1K count="$PERSISTENT_SIZE_KIB"
            mkfs.ext4 "/platform/persistent/${PACKAGE_NAME}-${PACKAGE_VERSION}.img"
        fi

        mkdir -p "${LXC_ROOTFS_DIR}/home/geisa"
        mount -o loop "/platform/persistent/${PACKAGE_NAME}-${PACKAGE_VERSION}.img" "${LXC_ROOTFS_DIR}/home/geisa"

        cat > "${LXC_ROOT_DIR}/config" <<EOF
lxc.rootfs.path = ${LXC_ROOTFS_DIR}
lxc.uts.name = ${PACKAGE_NAME}
lxc.execute.cmd = /usr/bin/${PACKAGE_NAME}
lxc.autodev = 1
lxc.mount.auto = proc sys
EOF
        echo "Installed $PACKAGE_NAME version $PACKAGE_VERSION"
        ;;


    uninstall)
        PACKAGE_VERSION="$1"
        shift 1
        if [ -z "$PACKAGE_NAME" ]; then
            echo "Usage: $0 uninstall <package_name> <package_version>"
            exit 1
        fi

        LXC_ROOT_DIR="/platform/lxc/${PACKAGE_NAME}"
        PACKAGE_DIR="/platform/apps/${PACKAGE_NAME}"
        PERSISTENT_IMG="/platform/persistent/${PACKAGE_NAME}-${PACKAGE_VERSION}.img"
        PACKAGE_IMAGE="${PACKAGE_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}.squashfs"
        CONFIG_IMAGE="${PACKAGE_DIR}/${PACKAGE_NAME}-${PACKAGE_VERSION}-config.squashfs"

        if mountpoint -q "${LXC_ROOTFS_DIR}/home/geisa"; then
            umount "${LXC_ROOTFS_DIR}/home/geisa"
        fi
        if mountpoint -q "${LXC_ROOTFS_DIR}/tmp"; then
            umount "${LXC_ROOTFS_DIR}/tmp"
        fi
        if mountpoint -q "${LXC_ROOTFS_DIR}"; then
            umount "${LXC_ROOTFS_DIR}"
        fi
        if mountpoint -q "${LXC_APP_DIR}"; then
            umount "${LXC_APP_DIR}"
        fi
        if mountpoint -q "${LXC_CONFIG_DIR}"; then
            umount "${LXC_CONFIG_DIR}"
        fi

        rm -rf "$LXC_ROOT_DIR"
        rm -f "$PACKAGE_IMAGE"
        rm -f "$CONFIG_IMAGE"
        rm -f "$PERSISTENT_IMG"

        echo "Uninstalled $PACKAGE_NAME version $PACKAGE_VERSION"
        ;;


    verify)
        PACKAGE_VERSION="$1"
        shift 1
        echo "verify mode: not yet supported"
        exit 0
        ;;


    activate)
        MQTT_CONF="${LXC_ROOTFS_DIR}/opt/geisa/mqtt.conf"

        if [ ! -f "$MQTT_CONF" ]; then
            echo "Error: $MQTT_CONF not found."
            exit 1
        fi

        PACKAGE_ID=$(grep '^GEISA_PACKAGE_ID=' "$MQTT_CONF" | cut -d'=' -f2)
        PACKAGE_TOKEN=$(grep '^GEISA_PACKAGE_TOKEN=' "$MQTT_CONF" | cut -d'=' -f2)

        if [ -z "$PACKAGE_ID" ] || [ -z "$PACKAGE_TOKEN" ]; then
            echo "Error: PACKAGE_ID or PACKAGE_TOKEN not found in $MQTT_CONF."
            exit 2
        fi

        register_app_to_mqtt_broker "$PACKAGE_ID" "$PACKAGE_TOKEN"
        if ! lxc-execute -P /platform/lxc -n "${PACKAGE_NAME}"; then
            echo "Error: lxc-execute failed for ${PACKAGE_NAME}."
            exit 3
        fi

        echo "Package $PACKAGE_NAME activated."
        exit 0
        ;;


    deactivate)
        MQTT_CONF="${LXC_ROOTFS_DIR}/opt/geisa/mqtt.conf"

        if [ ! -f "$MQTT_CONF" ]; then
            echo "Error: $MQTT_CONF not found."
            exit 1
        fi

        PACKAGE_ID=$(grep '^GEISA_PACKAGE_ID=' "$MQTT_CONF" | cut -d'=' -f2)
        if [ -z "$PACKAGE_ID" ]; then
            echo "Error: PACKAGE_ID not found in $MQTT_CONF."
            exit 2
        fi

        deregister_app_to_mqtt_broker "$PACKAGE_ID"

        # Stop application container
        if lxc-info -P /platform/lxc -n "${PACKAGE_NAME}" | grep -q "RUNNING"; then
            lxc-stop -P /platform/lxc -n "${PACKAGE_NAME}"
        fi

        echo "Package $PACKAGE_NAME deactivated."
        exit 0
        ;;

    *)
        echo "Unknown mode: $MODE"
        echo "Usage:"
        echo "  $0 install <package_name> <package_version> <package_id> <persistent_size_kib> <tmpfs_size_kib>"
        echo "  $0 uninstall <package_name> <package_version>"
        echo "  $0 verify <package_name> <package_version> <package_manifest> <signature>"
        echo "  $0 activate <package_name>"
        echo "  $0 deactivate <package_name>"
        exit 1
        ;;
esac