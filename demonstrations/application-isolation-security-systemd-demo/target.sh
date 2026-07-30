#!/usr/bin/env bash

set -euo pipefail

COLS=$(tput cols)

print_equals() {
	printf "%0.s=" $(seq 1 "$COLS")
}

cleanup() {
	print_equals
	echo "Cleaning up..."
	sudo rm -f \
		/usr/bin/demo.py \
		/usr/bin/cleanup.py \
		/etc/systmd/system/demo.service \
		/etc/systemd/system/demo_unsandboxed.service
	# No need for --remove because the user was created with
	# the --system flag.
	sudo userdel demo || true
	sudo systemctl daemon-reload
	echo "All done."
}

trap cleanup EXIT

# Setup/install
print_equals
echo "Performing initial setup/installation and running the services..."
sudo useradd --system --shell=/sbin/nologin demo &>/dev/null || true
sudo cp ~/demo.service /etc/systemd/system/demo.service
sudo cp ~/demo_unsandboxed.service /etc/systemd/system/demo_unsandboxed.service
sudo cp ~/demo.py ~/cleanup.py /usr/bin/
sudo systemctl daemon-reload
sudo systemctl restart demo_unsandboxed || true
sudo systemctl restart demo || true
sleep 1
# Print results
print_equals
echo "Results for UNSANDBOXED service:"
print_equals
sudo journalctl --output cat --no-pager --since="-10s" -u demo_unsandboxed | tee ~/unsandboxed.log
# IPC removal is run *AFTER* ExecStopPost, so run the cleanup script here instead
# of as an ExecStopPost directive in the unit file.
sudo cleanup.py unsandboxed | tee -a ~/unsandboxed.log
print_equals
echo "Results for SANDBOXED service:"
print_equals
sudo journalctl --output cat --no-pager --since="-10s" -u demo | tee ~/sandboxed.log
sudo -u demo cleanup.py sandboxed | tee -a ~/sandboxed.log
print_equals
echo "Run 'systemd-analyze security' for SANDBOXED service:"
print_equals
sudo systemd-analyze --no-pager security demo
