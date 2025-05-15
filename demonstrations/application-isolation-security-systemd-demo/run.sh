#!/usr/bin/env bash

set -euo pipefail

echo "Copying files over to remote target machine..."
echo ""
scp ./demo.py ./cleanup.py ./demo.service ./demo_unsandboxed.service ./target.sh "${TARGET}":~
echo ""
echo "Executing target.sh on remote target machine..."
ssh -t "$TARGET" "TERM=xterm ./target.sh"
echo ""
echo "Copying log files from remote target machine to local machine..."
echo ""
scp "$TARGET:~/*.log" .
