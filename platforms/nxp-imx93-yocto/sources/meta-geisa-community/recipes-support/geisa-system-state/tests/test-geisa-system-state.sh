#!/bin/sh
# Fixture test for the human-readable GEISA system-state report.
set -eu

script=${1:?usage: test-geisa-system-state.sh /path/to/geisa-system-state}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT INT TERM
fake="$work/bin"
mkdir -p "$fake" "$work/sys/fs/cgroup" "$work/sys/class/thermal/thermal_zone0" \
    "$work/proc" "$work/state/app-registry"

cat >"$work/sys/fs/cgroup/cgroup.controllers" <<'EOF'
cpuset cpu io memory pids
EOF
printf 'cpu-thermal\n' >"$work/sys/class/thermal/thermal_zone0/type"
printf '63400\n' >"$work/sys/class/thermal/thermal_zone0/temp"
cat >"$work/proc/meminfo" <<'EOF'
MemTotal: 1880000 kB
MemAvailable: 1006000 kB
SwapTotal: 0 kB
SwapFree: 0 kB
EOF
cat >"$work/state/app-registry/managed-apps.json" <<'EOF'
{"app_instances":[{"app_id":"com.example.alpha","display_name":"Alpha","install_state":"installed","execution_state":"running"}]}
EOF

make_tool() {
    cat >"$fake/$1"
    chmod +x "$fake/$1"
}

make_tool date <<'EOF'
#!/bin/sh
printf '2026-07-29 12:23:43 UTC\n'
EOF
make_tool hostname <<'EOF'
#!/bin/sh
printf 'geisa-imx93\n'
EOF
make_tool id <<'EOF'
#!/bin/sh
[ "$1" = geisa ] && { printf 'uid=999(geisa) gid=999(geisa) groups=999(geisa),995(ethosu)\n'; exit 0; }
exit 1
EOF
make_tool stat <<'EOF'
#!/bin/sh
printf 'cgroup2fs\n'
EOF
make_tool ip <<'EOF'
#!/bin/sh
case "$*" in
  '-o -4 addr show') printf '2: end0    inet 10.0.0.188/24 brd 10.0.0.255 scope global end0\n' ;;
  '-o link show') printf '1: lo: <LOOPBACK,UP> mtu 65536 state UNKNOWN mode DEFAULT\n2: end0: <BROADCAST,UP> mtu 1500 state UP mode DEFAULT\n3: lxcbr0: <NO-CARRIER,BROADCAST> mtu 1500 state DOWN mode DEFAULT\n' ;;
  *) : ;;
esac
EOF
make_tool df <<'EOF'
#!/bin/sh
printf '%s\n' 'Filesystem Size Used Avail Use% Mounted on' '/dev/mmcblk0p2 7.7G 1.3G 6.0G 18% /' '/dev/mmcblk0p3 5.8G 225M 5.2G 5% /platform'
EOF
make_tool uptime <<'EOF'
#!/bin/sh
printf ' 12:23:44 up 1:36, 0 user, load average: 2.18, 1.16, 0.61\n'
EOF
make_tool free <<'EOF'
#!/bin/sh
printf '%s\n' '              total used free shared buff/cache available' 'Mem: 1880 874 250 25 870 1006' 'Swap: 0 0 0'
EOF
make_tool lxc-ls <<'EOF'
#!/bin/sh
printf '%s\n' 'NAME STATE AUTOSTART GROUPS IPV4 IPV6 UNPRIVILEGED' 'geisa-app-alpha RUNNING 0 - 10.0.3.6 - false'
EOF
make_tool systemctl <<'EOF'
#!/bin/sh
exit 0
EOF
make_tool clear <<'EOF'
#!/bin/sh
printf clear-called >&2
EOF
make_tool journalctl <<'EOF'
#!/bin/sh
exit 0
EOF
make_tool ss <<'EOF'
#!/bin/sh
exit 0
EOF

output="$work/once.out"
PATH="$fake:$PATH" SYSFS_ROOT="$work/sys" PROC_ROOT="$work/proc" GEISA_STATE_ROOT="$work/state" \
    "$script" --once >"$output"

assert() { grep -Fqx "$1" "$output" >/dev/null || { printf 'missing: %s\n' "$1" >&2; exit 1; }; }
assert 'GEISA system state: 2026-07-29 12:23:43 UTC'
assert 'Hostname: geisa-imx93'
assert 'GEISA account: uid=999(geisa) gid=999(geisa) groups=999(geisa),995(ethosu)'
assert 'Cgroups: cgroup2fs controllers=cpuset cpu io memory pids'
assert 'Network: end0 UP 10.0.0.188/24; lxcbr0 DOWN <no IP>'
assert 'Thermal: cpu-thermal 63.4 C'
assert 'Alpha (com.example.alpha) state=running'
assert 'Systemd: failed=0'
grep -F 'Filesystem Size Used Avail Use% Mounted on' "$output" >/dev/null
grep -F 'Swap: 0 0 0' "$output" >/dev/null
grep -F 'geisa-app-alpha RUNNING' "$output" >/dev/null

# A heading without a container row is an explicit empty LXC state.
make_tool lxc-ls <<'EOF'
#!/bin/sh
printf '%s\n' 'NAME STATE AUTOSTART GROUPS IPV4 IPV6 UNPRIVILEGED'
EOF
PATH="$fake:$PATH" SYSFS_ROOT="$work/sys" PROC_ROOT="$work/proc" GEISA_STATE_ROOT="$work/state" \
    "$script" --once >"$work/no-lxc.out"
grep -Fxq 'none' "$work/no-lxc.out"

# Optional facilities must degrade independently.
PATH="$fake:$PATH" SYSFS_ROOT="$work/no-thermal" PROC_ROOT="$work/proc" GEISA_STATE_ROOT="$work/no-runtime" \
    "$script" --once >"$work/optional.out"
grep -Fqx 'Thermal: unavailable' "$work/optional.out" >/dev/null
grep -Fqx 'runtime not installed or registry unavailable' "$work/optional.out" >/dev/null

# Refresh redirected to a file must not emit terminal controls or call clear.
PATH="$fake:$PATH" SYSFS_ROOT="$work/sys" PROC_ROOT="$work/proc" GEISA_STATE_ROOT="$work/state" \
    INTERVAL=1 timeout 2 "$script" >"$work/refresh.out" 2>"$work/refresh.err" || true
[ ! -s "$work/refresh.err" ]
[ "$(grep -c '^GEISA system state:' "$work/refresh.out")" -ge 1 ]
! grep -q "$(printf '\033')" "$work/refresh.out"

printf 'geisa-system-state fixture tests: PASS\n'
