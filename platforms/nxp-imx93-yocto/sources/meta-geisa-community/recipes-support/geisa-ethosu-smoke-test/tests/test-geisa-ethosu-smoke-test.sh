#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 PragSol Consulting, LLC
# Website: https://www.pragsolconsulting.com/
#

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
script="$script_dir/files/geisa-ethosu-smoke-test"
workdir=$(mktemp -d "${TMPDIR:-/tmp}/geisa-ethosu-smoke-test.XXXXXX")
trap 'rm -rf "$workdir"' EXIT
mkdir -p "$workdir/bin"
profile="$workdir/profile"
mkdir -p "$profile/lib" "$profile/python"

model="$workdir/model.tflite"
input="$workdir/input.bmp"
delegate="$workdir/delegate.so"
device="$workdir/ethosu0"
printf 'model fixture\n' > "$model"
printf 'input fixture\n' > "$input"
: > "$delegate"
: > "$device"
model_sha256=$(sha256sum "$model" | awk '{print $1}')

cat > "$workdir/bin/vela" <<'EOF'
#!/bin/sh
output_dir=
while [ "$#" -gt 0 ]; do
    case "$1" in
        --output-dir) output_dir=$2; shift 2 ;;
        *) model=$1; shift ;;
    esac
done
cp "$model" "$output_dir/$(basename "${model%.tflite}")_vela.tflite"
EOF
chmod +x "$workdir/bin/vela"

cat > "$workdir/bin/python3" <<'EOF'
#!/bin/sh
cat >/dev/null
echo 'EthosuDelegate: 1 nodes delegated out of 1 nodes'
echo 'Result: argmax=653 expected=653 inference_ms=1'
EOF
chmod +x "$workdir/bin/python3"

cat > "$profile/profile.json" <<EOF
{
  "loader": {
    "library-path": ["lib", "python/av.libs"],
    "pythonpath": "python"
  }
}
EOF
: > "$profile/lib/libethosu_delegate.so"
mkdir -p "$profile/python/av.libs"

GEISA_ETHOSU_SMOKE_MODEL="$model" \
GEISA_ETHOSU_SMOKE_INPUT="$input" \
GEISA_ETHOSU_SMOKE_DELEGATE="$delegate" \
GEISA_ETHOSU_SMOKE_DEVICE="$device" \
GEISA_ETHOSU_SMOKE_VELA="$workdir/bin/vela" \
GEISA_ETHOSU_SMOKE_PYTHON="$workdir/bin/python3" \
GEISA_ETHOSU_SMOKE_EXPECTED_MODEL_SHA256="$model_sha256" \
"$script" > "$workdir/output"

grep -F 'Delegation: EthosuDelegate: 1 nodes delegated out of 1 nodes' "$workdir/output"
grep -F 'Compiled model:' "$workdir/output"
grep -F 'Ethos-U smoke test passed' "$workdir/output"
grep -F 'PASS' "$workdir/output"

GEISA_ETHOSU_SMOKE_MODEL="$model" \
GEISA_ETHOSU_SMOKE_INPUT="$input" \
GEISA_ETHOSU_SMOKE_DEVICE="$device" \
GEISA_ETHOSU_SMOKE_VELA="$workdir/bin/vela" \
GEISA_ETHOSU_SMOKE_PYTHON="$workdir/bin/python3" \
GEISA_ETHOSU_SMOKE_EXPECTED_MODEL_SHA256="$model_sha256" \
"$script" --profile "$profile" > "$workdir/profile-output"

grep -F 'Mode: profile' "$workdir/profile-output"
grep -F "Profile: $profile" "$workdir/profile-output"
grep -F "Profile pythonpath: $profile/python" "$workdir/profile-output"
grep -F "Profile library path: $profile/lib:$profile/python/av.libs" "$workdir/profile-output"
grep -F "Delegate: $profile/lib/libethosu_delegate.so" "$workdir/profile-output"
grep -F 'Compiled model:' "$workdir/profile-output"
grep -F 'PASS' "$workdir/profile-output"
