#!/usr/bin/env bash
set -euo pipefail

platform_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd -P)"
export GEISA_PLATFORM_ROOT="$platform_root"
. "$platform_root/scripts/release-profile.sh"
setup="$platform_root/scripts/setup-sources.sh"
state="$platform_root/scripts/geisa-source-state.sh"
tmp_root="$(mktemp -d "${TMPDIR:-/tmp}/geisa-release-profile-tests.XXXXXX")"
source_test_file="$platform_root/sources/meta-geisa-community/.geisa-source-state-test.$$"
generated_test_root="$(mktemp -d "${TMPDIR:-/tmp}/geisa-source-state-test.XXXXXX")"
trap 'rm -rf "$tmp_root" "$source_test_file" "$generated_test_root"' EXIT

"$setup" --check --release nxp-6.18.2-1.0.0 >"$tmp_root/sources"
grep -Fq 'nxp-6.18.2-1.0.0' "$tmp_root/sources"

release_profile_select nxp-6.18.2-1.0.0
release_profile_check_complete
[[ "$MACHINE_IMX93" = geisa-imx93 && "$MACHINE_IMX95" = geisa-imx95 ]]
[[ "$GEISA_NXP_MACHINE_INCLUDE_IMX93" = conf/machine/imx93-11x11-lpddr4x-frdm.conf ]]
[[ "$GEISA_NXP_MACHINE_INCLUDE_IMX95" = conf/machine/imx95-15x15-lpddr4x-frdm.conf ]]
[[ "$KERNEL_VERSION" = 6.18.2 ]]
[[ "$KERNEL_SRCREV" = f49f45233f7b10006ce7e9c826ee882bb14ac8b5 ]]
[[ "$UBOOT_VERSION" = 2025.04 ]]
[[ "$UBOOT_SRCREV" = 99518e6b6f20cb6a2bf19115e355db9f58100af8 ]]
[[ " $SOURCE_IDS " = *' bitbake '* && " $SOURCE_IDS " = *' openembedded-core '* ]]
[[ " $SOURCE_IDS " = *' meta-yocto '* && " $SOURCE_IDS " != *' poky '* ]]
[[ " $LAYER_PATHS " = *' sources/meta-imx/meta-imx-ml '* ]]
[[ " $LAYER_PATHS " = *' sources/meta-imx/meta-imx-sdk '* ]]
[[ -z "${BBMASK_PATHS:-}" ]]

"$state" --release nxp-6.18.2-1.0.0 "$tmp_root/state-a"
"$state" --release nxp-6.18.2-1.0.0 "$tmp_root/state-b"
cmp "$tmp_root/state-a" "$tmp_root/state-b"

mkdir -p "$generated_test_root/tmp"
printf '%s\n' generated >"$generated_test_root/tmp/generated-file"
"$state" --release nxp-6.18.2-1.0.0 "$tmp_root/state-generated"
cmp "$tmp_root/state-a" "$tmp_root/state-generated"

printf '%s\n' untracked >"$source_test_file"
"$state" --release nxp-6.18.2-1.0.0 "$tmp_root/state-untracked"
grep -Fqx 'GEISA_SOURCE_DIRTY = "true"' "$tmp_root/state-untracked"

echo "release profile tests passed"
