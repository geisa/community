#!/usr/bin/bash -ex
# This script is intended to be used by sfl.Jenkinsfile

# Reboot target
ssh $SSH_OPTS root@$TARGET_IP /sbin/reboot || true
sleep 10

# Wait for target to come back online
let TARGET_UP_TIMEOUT=60
while ! ssh $SSH_OPTS root@$TARGET_IP echo hello; do
    let TARGET_UP_TIMEOUT--
    if [ $TARGET_UP_TIMEOUT -eq 0 ]; then
        echo "Failed to contact board"
        exit 1
    fi
    sleep 1
done

# Run tests on target
test $(ssh $SSH_OPTS root@$TARGET_IP cat /etc/version) -eq $ROOTFS_VERSION
ssh $SSH_OPTS root@$TARGET_IP /usr/sbin/cukinia -f junitxml -o /var/run/cukinia-$TARGET_JENKINS_NODE.xml || true
scp $SSH_OPTS root@$TARGET_IP:/var/run/cukinia-$TARGET_JENKINS_NODE.xml cukinia.xml
