# Systemd Application Isolation Demonstration

Proof-of-concept demonstration comparing an unsecured systemd service with a
service configured using systemd isolation and hardening controls.

## Relationship to GEISA

This project demonstrates systemd service controls relevant to GEISA Linux
Execution Environment application isolation.

Both services execute the same Python program. The unsecured unit runs with
few restrictions, while the hardened unit uses systemd controls to restrict
filesystem, device, network, namespace, capability, and process access.

This is a focused demonstration rather than a complete GEISA application
isolation or resource-management implementation.

## Project Information

- **Status:** Maintenance Limited
- **Maintainer:** Brandon Thayer (`@blthayer`), original author
- **License:** Apache-2.0
- **GEISA versions tested or supported:** Not tied to a specific GEISA release
- **Support:** Best-effort

## Building or Using the Project

The demonstration requires:

- a local Unix-like system with Bash, SSH, SCP, and `diff`
- a disposable remote Linux target running systemd 249 or later
- SSH access to the target
- sudo access on the target
- Python 3 on the target

Review `run.sh`, `target.sh`, the service units, and the Python scripts before
running the demonstration.

From this project directory:

```console
TARGET='user@target-host' ./run.sh
```

The script:

1. copies the demonstration files to the target
2. installs temporary systemd units and a system user
3. runs unsecured and hardened versions of the service
4. captures their journal output
5. retrieves the resulting logs
6. removes the installed demonstration files, units, and user

Compare the results with:

```console
diff sandboxed.log unsandboxed.log
```

Use only a disposable or easily restored target. The scripts perform
privileged system modifications, including creating a system user and copying
files into `/usr/bin` and `/etc/systemd/system`.

## Demonstrated Controls

The hardened service explores controls including:

- `PrivateTmp`
- `ProtectHome`
- `PrivateDevices`
- `ProtectProc`
- `PrivateNetwork`
- `RestrictAddressFamilies`
- `RestrictNamespaces`
- `PrivateUsers`
- `ProtectControlGroups`
- `ProtectKernelModules`
- `ProtectKernelTunables`
- `ProtectSystem`
- `NoNewPrivileges`
- `CapabilityBoundingSet`
- `MemoryDenyWriteExecute`
- `RemoveIPC`
- `UMask`

The exact behavior and `systemd-analyze security` output depend on the target
systemd version, kernel, distribution, and host configuration.

## Known Limitations

- The project does not implement CPU, memory, storage, or network-volume
  resource controls.
- It does not provide a managed-container lifecycle or GEISA application
  manager.
- It does not validate LXC or complete cgroup-v2 behavior.
- Several probes depend on Ubuntu-like assumptions, including
  `/var/log/syslog`, the `syslog` account, and availability of the `spidev`
  kernel module.
- Some isolation controls are demonstrated indirectly or remain incomplete.
- Runtime validation requires a disposable Linux target and was not performed
  as part of the Community migration.
