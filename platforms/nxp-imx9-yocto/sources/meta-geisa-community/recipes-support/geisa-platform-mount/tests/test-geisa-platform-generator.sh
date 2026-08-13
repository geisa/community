#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 PragSol Consulting, LLC
# Website: https://www.pragsolconsulting.com/
#
# Fixture tests for root-relative /platform mount generation.
set -eu

generator=${1:?usage: test-geisa-platform-generator.sh /path/to/generator}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT INT TERM
fake="$work/bin"
mkdir -p "$fake"

cat >"$fake/findmnt" <<'EOF'
#!/bin/sh
printf '%s\n' "${GEISA_TEST_ROOT_SOURCE:-}"
EOF
chmod +x "$fake/findmnt"

run_generator() {
    name=$1
    source=$2
    out="$work/$name"
    mkdir -p "$out/normal" "$out/early" "$out/late"
    GEISA_PLATFORM_FINDMNT="$fake/findmnt" GEISA_TEST_ROOT_SOURCE="$source" \
        "$generator" "$out/normal" "$out/early" "$out/late"
    printf '%s\n' "$out/normal"
}

assert_file_contains() {
    grep -Fqx "$2" "$1" >/dev/null || {
        printf 'missing in %s: %s\n' "$1" "$2" >&2
        exit 1
    }
}

emmc=$(run_generator emmc /dev/mmcblk0p2)
assert_file_contains "$emmc/platform.mount" 'What=/dev/mmcblk0p3'
assert_file_contains "$emmc/geisa-platform-fsck.service" 'Type=oneshot'
assert_file_contains "$emmc/geisa-platform-fsck.service" 'RemainAfterExit=yes'
assert_file_contains "$emmc/geisa-platform-fsck.service" 'Requires=dev-mmcblk0p3.device'
assert_file_contains "$emmc/geisa-platform-fsck.service" 'After=dev-mmcblk0p3.device'
assert_file_contains "$emmc/geisa-platform-fsck.service" 'Before=platform.mount local-fs.target'
assert_file_contains "$emmc/geisa-platform-fsck.service" 'ExecStart=/usr/sbin/fsck -a /dev/mmcblk0p3'
assert_file_contains "$emmc/platform.mount" 'Requires=dev-mmcblk0p3.device geisa-platform-fsck.service'
assert_file_contains "$emmc/platform.mount" 'After=dev-mmcblk0p3.device geisa-platform-fsck.service'
[ "$(readlink "$emmc/local-fs.target.requires/platform.mount")" = '../platform.mount' ]
[ "$(find "$emmc/local-fs.target.requires" -maxdepth 1 -type l -printf '%f\n' | sort)" = 'platform.mount' ]
[ ! -e "$emmc/local-fs.target.requires/geisa-platform-fsck.service" ]

sd=$(run_generator sd /dev/mmcblk1p2)
assert_file_contains "$sd/platform.mount" 'What=/dev/mmcblk1p3'
assert_file_contains "$sd/geisa-platform-fsck.service" 'Requires=dev-mmcblk1p3.device'
assert_file_contains "$sd/platform.mount" 'Requires=dev-mmcblk1p3.device geisa-platform-fsck.service'
assert_file_contains "$sd/geisa-platform-fsck.service" 'ExecStart=/usr/sbin/fsck -a /dev/mmcblk1p3'
[ ! -e "$sd/local-fs.target.requires/geisa-platform-fsck.service" ]

# Duplicate labels are intentionally irrelevant: the generator only uses the
# active root mount source and never inspects fstab or filesystem labels.
duplicate=$(run_generator duplicate-labels /dev/mmcblk1p2)
printf '%s\n' 'LABEL=platform /platform ext4 defaults 0 2' >"$work/duplicate-fstab"
assert_file_contains "$duplicate/platform.mount" 'What=/dev/mmcblk1p3'
! grep -q 'LABEL=' "$duplicate/platform.mount"

invalid=$(run_generator invalid /dev/nvme0n1p2)
[ ! -e "$invalid/platform.mount" ]
[ ! -e "$invalid/geisa-platform-fsck.service" ]
[ -f "$invalid/geisa-platform-root-invalid.service" ]
[ "$(readlink "$invalid/local-fs.target.requires/geisa-platform-root-invalid.service")" = '../geisa-platform-root-invalid.service' ]
[ "$(find "$invalid/local-fs.target.requires" -maxdepth 1 -type l -printf '%f\n')" = 'geisa-platform-root-invalid.service' ]

malformed=$(run_generator malformed /dev/mmcblk1p3)
[ ! -e "$malformed/platform.mount" ]
[ ! -e "$malformed/geisa-platform-fsck.service" ]
[ -f "$malformed/geisa-platform-root-invalid.service" ]

printf 'geisa-platform-generator fixture tests: PASS\n'
