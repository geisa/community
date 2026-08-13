#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2026 PragSol Consulting, LLC

set -eu

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
script="$script_dir/files/geisa-neutron-smoke-test"
workdir=$(mktemp -d "${TMPDIR:-/tmp}/geisa-neutron-smoke-test.XXXXXX")
trap 'rm -rf "$workdir"' EXIT

mkdir -p "$workdir/bin"
model="$workdir/model.tflite"
delegate="$workdir/libneutron_delegate.so"
driver="$workdir/libNeutronDriver.so"
firmware="$workdir/NeutronFirmware.elf"
device="$workdir/neutron0"
benchmark="$workdir/bin/benchmark_model"
printf 'converted fixture\n' >"$model"
: >"$delegate"
: >"$driver"
: >"$firmware"
: >"$device"
model_sha256=$(sha256sum "$model" | awk '{print $1}')

cat >"$benchmark" <<'EOF'
#!/bin/sh
case " $* " in
    *" --enable_op_profiling=true "*) ;;
    *) echo 'missing profiling option' >&2; exit 2 ;;
esac
case " $* " in
    *" --warmup_runs=2 "*) ;;
    *) echo 'missing warmup_runs option' >&2; exit 2 ;;
esac
cat <<'LOG'
INFO: NeutronDelegate delegate: 1 nodes delegated out of 3 nodes with 1 partitions
NeutronDelegate delegate version: v1.0.0-test zerocp enabled
INFO: Created TensorFlow Lite XNNPACK delegate for CPU.
Operator-wise Profiling Info for Regular Benchmark Runs:
============================== Run Order ==============================
NeutronDelegate 1.100 ms
SOFTMAX 0.024 ms
QUANTIZE 0.005 ms
LOG
EOF
chmod +x "$benchmark"

run() {
    GEISA_NEUTRON_SMOKE_MODEL="$model" \
    GEISA_NEUTRON_SMOKE_DELEGATE="$delegate" \
    GEISA_NEUTRON_SMOKE_DRIVER="$driver" \
    GEISA_NEUTRON_SMOKE_FIRMWARE="$firmware" \
    GEISA_NEUTRON_SMOKE_DEVICE="$device" \
    GEISA_NEUTRON_SMOKE_BENCHMARK="$benchmark" \
    GEISA_NEUTRON_SMOKE_EXPECTED_MODEL_SHA256="$model_sha256" \
    "$script"
}

run >"$workdir/pass"
grep -F 'GEISA Neutron smoke test' "$workdir/pass"
grep -F 'Delegation:   1/3 nodes, 1 partitions' "$workdir/pass"
grep -F 'PASS: Neutron accelerated inference verified' "$workdir/pass"
! grep -F 'benchmark output' "$workdir/pass"

expect_fail() {
    if "$@" >"$workdir/fail" 2>&1; then
        echo "expected failure did not occur: $*" >&2
        exit 1
    fi
}

expect_fail env GEISA_NEUTRON_SMOKE_MODEL="$model" \
    GEISA_NEUTRON_SMOKE_DELEGATE="$delegate" GEISA_NEUTRON_SMOKE_DRIVER="$driver" \
    GEISA_NEUTRON_SMOKE_FIRMWARE="$firmware" GEISA_NEUTRON_SMOKE_DEVICE="$device" \
    GEISA_NEUTRON_SMOKE_BENCHMARK="$benchmark" \
    GEISA_NEUTRON_SMOKE_EXPECTED_MODEL_SHA256=wrong "$script"
expect_fail env GEISA_NEUTRON_SMOKE_MODEL="$workdir/missing-model" \
    GEISA_NEUTRON_SMOKE_DELEGATE="$delegate" GEISA_NEUTRON_SMOKE_DRIVER="$driver" \
    GEISA_NEUTRON_SMOKE_FIRMWARE="$firmware" GEISA_NEUTRON_SMOKE_DEVICE="$device" \
    GEISA_NEUTRON_SMOKE_BENCHMARK="$benchmark" "$script"
rm "$delegate"
expect_fail run
touch "$delegate"
rm "$driver"
expect_fail run
touch "$driver"
rm "$firmware"
expect_fail run
touch "$firmware"
rm "$device"
expect_fail run
touch "$device"
cat >"$benchmark" <<'EOF'
#!/bin/sh
echo 'INFO: NeutronDelegate delegate: 0 nodes delegated out of 31 nodes with 0 partitions.'
EOF
chmod +x "$benchmark"
expect_fail run

cat >"$benchmark" <<'EOF'
#!/bin/sh
echo 'INFO: NeutronDelegate delegate: 1 nodes delegated out of 3 nodes with 0 partitions.'
EOF
chmod +x "$benchmark"
expect_fail run

cat >"$benchmark" <<'EOF'
#!/bin/sh
echo 'INFO: NeutronDelegate delegate: malformed summary'
EOF
chmod +x "$benchmark"
expect_fail run

cat >"$benchmark" <<'EOF'
#!/bin/sh
echo 'INFO: NeutronDelegate delegate: 0 nodes delegated out of 31 nodes with 0 partitions.'
echo '============================== Run Order =============================='
echo 'CONV_2D 4.0 ms'
EOF
chmod +x "$benchmark"
expect_fail run

cat >"$benchmark" <<'EOF'
#!/bin/sh
echo 'NeutronDelegate delegate: 1 nodes delegated out of 3 nodes with 1 partitions'
echo 'Operator-wise Profiling Info for Regular Benchmark Runs:'
echo '============================== Run Order =============================='
echo 'CONV_2D 4.0 ms'
EOF
chmod +x "$benchmark"
expect_fail run

cat >"$benchmark" <<'EOF'
#!/bin/sh
echo 'INFO: NeutronDelegate delegate: 1 nodes delegated out of 3 nodes with 1 partitions.'
echo 'ERROR: unresolved custom op: NeutronGraph'
EOF
chmod +x "$benchmark"
expect_fail run

cat >"$benchmark" <<'EOF'
#!/bin/sh
echo 'NeutronDelegate delegate: 1 nodes delegated out of 3 nodes with 1 partitions'
echo 'Operator-wise Profiling Info for Regular Benchmark Runs:'
EOF
chmod +x "$benchmark"
expect_fail run
grep -F 'NeutronDelegate delegate: 1 nodes delegated out of 3 nodes with 1 partitions' "$workdir/fail"

cat >"$benchmark" <<'EOF'
#!/bin/sh
echo 'NeutronDelegate delegate: 1 nodes delegated out of 3 nodes with 1 partitions'
echo 'benchmark stderr: delegate application failed'
EOF
chmod +x "$benchmark"
expect_fail run

cat >"$benchmark" <<'EOF'
#!/bin/sh
echo 'NeutronDelegate delegate: 1 nodes delegated out of 3 nodes with 1 partitions'
echo 'Operator-wise Profiling Info for Regular Benchmark Runs:'
echo '============================== Run Order =============================='
echo 'NeutronDelegate 1.0 ms'
echo 'SOFTMAX 0.1 ms'
echo 'QUANTIZE 0.1 ms'
EOF
chmod +x "$benchmark"
chmod 000 "$device"
expect_fail run
chmod 600 "$device"

cat >"$benchmark" <<'EOF'
#!/bin/sh
echo 'benchmark output before failure' >&2
exit 1
EOF
chmod +x "$benchmark"
expect_fail run
grep -F 'benchmark output before failure' "$workdir/fail"

echo 'geisa-neutron-smoke-test fixture tests: PASS'
