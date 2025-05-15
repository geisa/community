# secure-systemd-poc

Proof of concept demonstration for securing/isolating systemd
services.

## Quick Start

```bash
TARGET='my_user@my_target' ./run.sh
...
diff sandboxed.log unsandboxed.log
```

## Overview

This demonstration installs two systemd services on a remote
(target) machine, executes them, captures log output, and downloads
the logs to the local machine. Both services execute the same script
(`demo.py`) in the exact same way, but one service
(`demo_unsandboxed.service`) is highly insecure and not locked down,
whereas the other (`demo.service`) is locked down nearly as much as
is possible.

The purpose is to show `systemd` unit configuration directives
related to application isolation and security in action and prove
out/demonstrate the effects of the configuration.

It is recommended that you read all configuration files and source code
prior to executing anything. Start with `run.sh` to understand the entire
flow. Roughly, `run.sh` copies files from this repository over to the
remote target machine, executes `target.sh` on the remote target machine,
and finally copies log files back over to the local machine from the
remote machine. `target.sh` installs and starts the two systemd services
mentioned earlier. `demo.py` is executed by these services, 

## Scope, Future Work, References

This demonstration focuses on security and isolation of systemd services,
and does not get into resource control (*e.g.* limiting CPU or memory for
a service). The first draft (2025-05-12) does not leverage `systemd`'s
`DynamicUser` capabilities, nor does it leverage the `RootDirectory` or
`RootImage` directives. `RootDirectory` and/or `RootImage` are both very
useful tools for service isolation. However, in many ways they "get in the
way" of keeping this demonstration simple and proving out other security/
isolation directives. Future work will use overlay mounts coupled with
`RootDirectory`/`RootImage`.

Most of the isolation and security directives explored in this demonstration
are documented in the `systemd.exec` man page, *i.e.* `man systemd.exec`.
Other notable man pages include `systemd.unit`, `systemd.service`, and
`systemd.resource-control`. These man pages can be found on the web at
https://www.freedesktop.org/software/systemd/man/latest/.

The primary creator of `systemd` maintains a blog site that has plenty of
helpful articles: https://0pointer.net/blog/. The
[post on dynamic users](https://0pointer.net/blog/dynamic-users-with-systemd.html)
is especially interesting.

There's a useful guide on systemd service hardening that can be found
here: https://linux-audit.com/systemd/how-to-harden-a-systemd-service-unit/

## Setup/Assumptions

- Your local machine runs a *Nix like OS and has `bash`, `scp`, `ssh`,
  and `diff` installed locally and on the system `PATH`.
- You have a separate remote machine (the "target") running Ubuntu 22.04
  or similar. The machine should have systemd installed (minimum version: 249).
- You're able to connect to the remote target over `ssh`
- The user used to connect to the remote target has `sudo` access
- The remote target machine has outbound internet access

**MODIFICATIONS REQUIRING SUPER USER ACCESS WILL BE MADE TO
THIS MACHINE.** That dire warning aside, no destructive operations are
performed and no services are enabled to start on boot. Read the source
to understand all system modifications that are made - you've been warned.

Ideally, your remote target machine is "disposable" in that it's simple to
re-image/re-install the OS "fresh." While the code exectuted here should
clean up after itself, you probably don't want to risk running it against
an important remote target machine that's difficult to re-tool.

For proving out the isolation capabilities, a couple more assumptions are
made:

- It is assumed that `/var/log/syslog` exists and is owned by
  the `syslog` user.
- It is assumed that `spidev` kernel module exists and is loadable.

The demonstration script, `demo.py`, primarily probes standard
files and directories of the Linux operating system. The script
should be considered "self-documenting" as it's very simple and
readable. This script has only been tested using the system Python
interpreter (version 3.10) that ships with Ubuntu 22.04.

## Usage

Change directories into this repository and then execute
`TARGET='myuser@mymachine' ./run.sh`. You will be prompted for
a `sudo` password.

Example output:

```
Copying files over to remote target machine...

demo.py                                                                                   100%   12KB   3.0MB/s   00:00
cleanup.py                                                                                100% 1042   630.4KB/s   00:00
demo.service                                                                              100% 3658     1.8MB/s   00:00
demo_unsandboxed.service                                                                  100%  268   186.9KB/s   00:00
target.sh                                                                                 100% 1660     1.1MB/s   00:00

Executing target.sh on remote target machine...
============================================================================================================================
Performing initial setup/installation and running the services...
[sudo] password for your-user:
============================================================================================================================
Results for UNSANDBOXED service:
============================================================================================================================
Started Demonstration of non-isolated service.
****************************************************************************************************************************
*
Starting up demo service script.
****************************************************************************************************************************
*
****************************************************************************************************************************
*
Systemd directive: PrivateTmp
Exploring /tmp...
Contents of /tmp (showing -1 items, but there are 22 total): ['systemd-private-eb8226ca2c334f5695988122838859db-bluetooth.se
rvice-CWMqz7', 'systemd-private-eb8226ca2c334f5695988122838859db-colord.service-LrqLBO', '.font-unix', 'nvscsock', '.ICE-uni
x', 'systemd-private-eb8226ca2c334f5695988122838859db-power-profiles-daemon.service-jzBReU', 'camsock', 'systemd-private-eb8
226ca2c334f5695988122838859db-systemd-logind.service-r9lnFr', 'argus_socket', 'ssh-XXXXVi57Cr', '.XIM-unix', 'systemd-privat
e-eb8226ca2c334f5695988122838859db-ModemManager.service-XAXjI9', '.X11-unix', 'systemd-private-eb8226ca2c334f569598812283885
9db-systemd-resolved.service-p89eUT', '.Test-unix', 'systemd-private-eb8226ca2c334f5695988122838859db-haveged.service-1r89hB
', 'systemd-private-eb8226ca2c334f5695988122838859db-switcheroo-control.service-Hjr1ct', 'systemd-private-eb8226ca2c334f5695
988122838859db-fwupd.service-qTgR60', 'systemd-private-eb8226ca2c334f5695988122838859db-chrony.service-t4KXch', 'systemd-pri
vate-eb8226ca2c334f5695988122838859db-upower.service-JSMJ4U', 'systemd-private-eb8226ca2c334f5695988122838859db-demo.service
-N9L3Q1']
****************************************************************************************************************************
*
Systemd directive: ProtectHome
Exploring /home...
Contents of /home (showing -1 items, but there are 2 total): ['other-user']
****************************************************************************************************************************
*
Systemd directive: PrivateDevices
Exploring /dev...
Contents of /dev (showing 10 items, but there are 399 total): ['printer', 'i2c-11', 'nvidia-modeset', 'zram5', 'zram4', 'zra
m3', 'zram2', 'zram1', 'zram0', 'nvme-fabrics']
****************************************************************************************************************************
*
Systemd directive: ProtectProc
Note that ProtectProc is only effective for services which do not run as the root user.
Exploring /proc...
Contents of /proc (showing 10 items, but there are 316 total): ['fb', 'fs', 'bus', 'irq', 'mtd', 'net', 'sys', 'tty', 'keys'
, 'kmsg']
****************************************************************************************************************************
*
Systemd directive: PrivateNetwork
Attempting to access the internet...
Successfully connected to the internet. HEAD response: 302 Found
****************************************************************************************************************************
*
Systemd directive: RestrictAddressFamilies
Attempting to create an IPv4 internet socket...
Successfully created a socket: <socket.socket [closed] fd=-1, family=AddressFamily.AF_INET, type=SocketKind.SOCK_STREAM, pro
to=0>
*****************************************************************************************************************************
Systemd directive: RestrictNamespaces
Proving out namespace restrictions is an outstanding TODO.
os.setns and os.unshare were only introduced in Python 3.12 which is not yet standard system Python, and this script is intended to be portable.
*****************************************************************************************************************************
Systemd directive: PrivateUsers
stat'ing /var/log/syslog...
/var/log/syslog appears to be owned by the 'syslog' user!
*****************************************************************************************************************************
Systemd directive: ProtectControlGroups
Have not yet found a good way to prove out protection of /sys/fs/cgroup via the ProtectControlGroups directive...
*****************************************************************************************************************************
Systemd directive: ProtectKernelModules
Attempting to call modprobe to load the spidev kernel module...
Calling 'modprobe spidev' succeeded.
*****************************************************************************************************************************
Systemd directive: ProtectKernelTunables
Attempting to read /proc/kallsyms
Successfully opened /proc/kallsyms for reading. First line: 'ffffa829527d0000 T _text\n'
*****************************************************************************************************************************
Systemd directive: ProtectSystem
/ is NOT mounted read-only!
*****************************************************************************************************************************
Systemd directive: RestrictSUIDSGID
This service can only write to /tmp and /var/tmp, but those directories are mounted with 'nosuid' and as such it's very challenging (if not impossible) to prove out the RestrictSUIDSGID setting.
*****************************************************************************************************************************
Systemd directive: RestrictRealtime
With other service restrictions in place, it is difficult or impossible to modify the service schedule.
*****************************************************************************************************************************
Systemd directive: MemoryDenyWriteExecute
Successfully created an mmap which is both executable and writeable.
*****************************************************************************************************************************
Systemd directive: RemoveIPC
POSIX shared memory object (for IPC) created at /unsandboxed.
*****************************************************************************************************************************
Systemd directive: UMask
stat'ing /tmp/test.txt...
File created with the following permissions: 0o644
*****************************************************************************************************************************
Systemd directive: ProtectHostname
Not proving this out for now, as the RestrictAddressFamilies=none directive prevents the use of hostnamectl.
All done, exiting.
*****************************************************************************************************************************
demo_unsandboxed.service: Deactivated successfully.
*****************************************************************************************************************************
Systemd did NOT remove the shared memory object at /unsandboxed. Contents of object: 'hello'
============================================================================================================================Results for SANDBOXED service:
============================================================================================================================Started Application isolation demonstration.
*****************************************************************************************************************************
Starting up demo service script.
*****************************************************************************************************************************
*****************************************************************************************************************************
Systemd directive: PrivateTmp
Exploring /tmp...
/tmp is either empty or the service is not allowed to see the contents!
*****************************************************************************************************************************
Systemd directive: ProtectHome
Exploring /home...
Service does not have permission to list contents of /home
*****************************************************************************************************************************
Systemd directive: PrivateDevices
Exploring /dev...
Contents of /dev (showing 10 items, but there are 17 total): ['stderr', 'stdout', 'stdin', 'fd', 'tty', 'urandom', 'random', 'full', 'zero', 'null']
*****************************************************************************************************************************
Systemd directive: ProtectProc
Note that ProtectProc is only effective for services which do not run as the root user.
Exploring /proc...
Contents of /proc (showing 10 items, but there are 56 total): ['fb', 'fs', 'bus', 'irq', 'mtd', 'net', 'sys', 'tty', 'keys', 'kmsg']
*****************************************************************************************************************************
Systemd directive: PrivateNetwork
Attempting to access the internet...
Could not access the internet.
*****************************************************************************************************************************
Systemd directive: RestrictAddressFamilies
Attempting to create an IPv4 internet socket...
Could not create a socket.
*****************************************************************************************************************************
Systemd directive: RestrictNamespaces
Proving out namespace restrictions is an outstanding TODO.
os.setns and os.unshare were only introduced in Python 3.12 which is not yet standard system Python, and this script is intended to be portable.
*****************************************************************************************************************************
Systemd directive: PrivateUsers
stat'ing /var/log/syslog...
/var/log/syslog appears to be owned by the 'nobody' user!
*****************************************************************************************************************************
Systemd directive: ProtectControlGroups
Have not yet found a good way to prove out protection of /sys/fs/cgroup via the ProtectControlGroups directive...
*****************************************************************************************************************************
Systemd directive: ProtectKernelModules
Attempting to call modprobe to load the spidev kernel module...
Calling 'modprobe spidev' failed.
*****************************************************************************************************************************
Systemd directive: ProtectKernelTunables
Attempting to read /proc/kallsyms
Could not open /proc/kallsyms for reading!
*****************************************************************************************************************************
Systemd directive: ProtectSystem
/ is mounted read-only!
*****************************************************************************************************************************
Systemd directive: RestrictSUIDSGID
This service can only write to /tmp and /var/tmp, but those directories are mounted with 'nosuid' and as such it's very challenging (if not impossible) to prove out the RestrictSUIDSGID setting.
*****************************************************************************************************************************
Systemd directive: RestrictRealtime
With other service restrictions in place, it is difficult or impossible to modify the service schedule.
*****************************************************************************************************************************
Systemd directive: MemoryDenyWriteExecute
Could not create mmap which is both executable and writeable.
*****************************************************************************************************************************
Systemd directive: RemoveIPC
POSIX shared memory object (for IPC) created at /sandboxed.
*****************************************************************************************************************************
Systemd directive: UMask
stat'ing /tmp/test.txt...
File created with the following permissions: 0o600
*****************************************************************************************************************************
Systemd directive: ProtectHostname
Not proving this out for now, as the RestrictAddressFamilies=none directive prevents the use of hostnamectl.
All done, exiting.
*****************************************************************************************************************************
demo.service: Deactivated successfully.
*****************************************************************************************************************************
Systemd already removed the shared memory object at /sandboxed! Error code for opening: 2. Error message: 'No such file or directory'
============================================================================================================================Run 'systemd-analyze security' for SANDBOXED service:
============================================================================================================================  NAME                                                     DESCRIPTION                                              EXPOSURE
✓ PrivateNetwork=                                          Service has no access to the host's network
✓ User=/DynamicUser=                                       Service runs under a static non-root user identity
✓ CapabilityBoundingSet=~CAP_SET(UID|GID|PCAP)             Service cannot change UID/GID identities/capabilities
✓ CapabilityBoundingSet=~CAP_SYS_ADMIN                     Service has no administrator privileges
✓ CapabilityBoundingSet=~CAP_SYS_PTRACE                    Service has no ptrace() debugging abilities
✓ RestrictAddressFamilies=~AF_(INET|INET6)                 Service cannot allocate Internet sockets
✓ RestrictNamespaces=~CLONE_NEWUSER                        Service cannot create user namespaces
✓ RestrictAddressFamilies=~…                               Service cannot allocate exotic sockets
✓ CapabilityBoundingSet=~CAP_(CHOWN|FSETID|SETFCAP)        Service cannot change file ownership/access mode/capabi…
✓ CapabilityBoundingSet=~CAP_(DAC_*|FOWNER|IPC_OWNER)      Service cannot override UNIX file/IPC permission checks
✓ CapabilityBoundingSet=~CAP_NET_ADMIN                     Service has no network configuration privileges
✓ CapabilityBoundingSet=~CAP_SYS_MODULE                    Service cannot load kernel modules
✓ CapabilityBoundingSet=~CAP_SYS_RAWIO                     Service has no raw I/O access
✓ CapabilityBoundingSet=~CAP_SYS_TIME                      Service processes cannot change the system clock
✗ DeviceAllow=                                             Service has a device ACL with some special devices            0.1
✓ IPAddressDeny=                                           Service blocks all IP address ranges
✓ KeyringMode=                                             Service doesn't share key material with other services
✓ NoNewPrivileges=                                         Service processes cannot acquire new privileges
✓ NotifyAccess=                                            Service child processes cannot alter service state
✓ PrivateDevices=                                          Service has no access to hardware devices
✓ PrivateMounts=                                           Service cannot install system mounts
✓ PrivateTmp=                                              Service has no access to other software's temporary fil…
✓ PrivateUsers=                                            Service does not have access to other users
✓ ProtectClock=                                            Service cannot write to the hardware clock or system cl…
✓ ProtectControlGroups=                                    Service cannot modify the control group file system
✓ ProtectHome=                                             Service has no access to home directories
✓ ProtectKernelLogs=                                       Service cannot read from or write to the kernel log rin…
✓ ProtectKernelModules=                                    Service cannot load or read kernel modules
✓ ProtectKernelTunables=                                   Service cannot alter kernel tunables (/proc/sys, …)
✓ ProtectProc=                                             Service has restricted access to process tree (/proc hi…
✓ ProtectSystem=                                           Service has strict read-only access to the OS file hier…
✓ RestrictAddressFamilies=~AF_PACKET                       Service cannot allocate packet sockets
✓ RestrictSUIDSGID=                                        SUID/SGID file creation by service is restricted
✓ SystemCallArchitectures=                                 Service may execute system calls only with native ABI
✓ SystemCallFilter=~@clock                                 System call allow list defined for service, and @clock …
✓ SystemCallFilter=~@debug                                 System call allow list defined for service, and @debug …
✓ SystemCallFilter=~@module                                System call allow list defined for service, and @module…
✓ SystemCallFilter=~@mount                                 System call allow list defined for service, and @mount …
✓ SystemCallFilter=~@raw-io                                System call allow list defined for service, and @raw-io…
✓ SystemCallFilter=~@reboot                                System call allow list defined for service, and @reboot…
✓ SystemCallFilter=~@swap                                  System call allow list defined for service, and @swap i…
✓ SystemCallFilter=~@privileged                            System call allow list defined for service, and @privil…
✓ SystemCallFilter=~@resources                             System call allow list defined for service, and @resour…
✓ AmbientCapabilities=                                     Service process does not receive ambient capabilities
✓ CapabilityBoundingSet=~CAP_AUDIT_*                       Service has no audit subsystem access
✓ CapabilityBoundingSet=~CAP_KILL                          Service cannot send UNIX signals to arbitrary processes
✓ CapabilityBoundingSet=~CAP_MKNOD                         Service cannot create device nodes
✓ CapabilityBoundingSet=~CAP_NET_(BIND_SERVICE|BROADCAST|… Service has no elevated networking privileges
✓ CapabilityBoundingSet=~CAP_SYSLOG                        Service has no access to kernel logging
✓ CapabilityBoundingSet=~CAP_SYS_(NICE|RESOURCE)           Service has no privileges to change resource use parame…
✓ RestrictNamespaces=~CLONE_NEWCGROUP                      Service cannot create cgroup namespaces
✓ RestrictNamespaces=~CLONE_NEWIPC                         Service cannot create IPC namespaces
✓ RestrictNamespaces=~CLONE_NEWNET                         Service cannot create network namespaces
✓ RestrictNamespaces=~CLONE_NEWNS                          Service cannot create file system namespaces
✓ RestrictNamespaces=~CLONE_NEWPID                         Service cannot create process namespaces
✓ RestrictRealtime=                                        Service realtime scheduling access is restricted
✓ SystemCallFilter=~@cpu-emulation                         System call allow list defined for service, and @cpu-em…
✓ SystemCallFilter=~@obsolete                              System call allow list defined for service, and @obsole…
✓ RestrictAddressFamilies=~AF_NETLINK                      Service cannot allocate netlink sockets
✗ RootDirectory=/RootImage=                                Service runs within the host's root directory                 0.1
✓ SupplementaryGroups=                                     Service has no supplementary groups
✓ CapabilityBoundingSet=~CAP_MAC_*                         Service cannot adjust SMACK MAC
✓ CapabilityBoundingSet=~CAP_SYS_BOOT                      Service cannot issue reboot()
✓ Delegate=                                                Service does not maintain its own delegated control gro…
✓ LockPersonality=                                         Service cannot change ABI personality
✓ MemoryDenyWriteExecute=                                  Service cannot create writable executable memory mappin…
✓ RemoveIPC=                                               Service user cannot leave SysV IPC objects around
✓ RestrictNamespaces=~CLONE_NEWUTS                         Service cannot create hostname namespaces
✓ UMask=                                                   Files created by service are accessible only by service…
✓ CapabilityBoundingSet=~CAP_LINUX_IMMUTABLE               Service cannot mark files immutable
✓ CapabilityBoundingSet=~CAP_IPC_LOCK                      Service cannot lock memory into RAM
✓ CapabilityBoundingSet=~CAP_SYS_CHROOT                    Service cannot issue chroot()
✓ ProtectHostname=                                         Service cannot change system host/domainname
✓ CapabilityBoundingSet=~CAP_BLOCK_SUSPEND                 Service cannot establish wake locks
✓ CapabilityBoundingSet=~CAP_LEASE                         Service cannot create file leases
✓ CapabilityBoundingSet=~CAP_SYS_PACCT                     Service cannot use acct()
✓ CapabilityBoundingSet=~CAP_SYS_TTY_CONFIG                Service cannot issue vhangup()
✓ CapabilityBoundingSet=~CAP_WAKE_ALARM                    Service cannot program timers that wake up the system
✓ RestrictAddressFamilies=~AF_UNIX                         Service cannot allocate local sockets
✗ ProcSubset=                                              Service has full access to non-process /proc files (/pr…      0.1

→ Overall exposure level for demo.service: 0.2 SAFE 😀
============================================================================================================================Cleaning up...
All done.
Connection to 192.168.168.50 closed.

Copying log files from remote target machine to local machine...

sandboxed.log                                                                             100% 5601   444.6KB/s   00:00
unsandboxed.log                                                                           100% 6867     2.8MB/s   00:00
```

Note that most, but not all systemd isolation/sandboxing settings are proved out
via the `demo.py` script, but it is not practical/easy to prove out absolutely
everything.

And here is a sample diff of the logs (`diff sandboxed.log unsandboxed.log`):

```diff
1c1
< Started Application isolation demonstration.
---
> Started Demonstration of non-isolated service.
8c8
< /tmp is either empty or the service is not allowed to see the contents!
---
> Contents of /tmp (showing -1 items, but there are 22 total): ['systemd-private-eb8226ca2c334f5695988122838859db-bluetooth.
service-CWMqz7', 'systemd-private-eb8226ca2c334f5695988122838859db-colord.service-LrqLBO', '.font-unix', 'nvscsock', '.ICE-u
nix', 'systemd-private-eb8226ca2c334f5695988122838859db-power-profiles-daemon.service-jzBReU', 'camsock', 'systemd-private-e
b8226ca2c334f5695988122838859db-systemd-logind.service-r9lnFr', 'argus_socket', 'ssh-XXXXVi57Cr', '.XIM-unix', 'systemd-priv
ate-eb8226ca2c334f5695988122838859db-ModemManager.service-XAXjI9', '.X11-unix', 'systemd-private-eb8226ca2c334f5695988122838
859db-systemd-resolved.service-p89eUT', '.Test-unix', 'systemd-private-eb8226ca2c334f5695988122838859db-haveged.service-1r89
hB', 'systemd-private-eb8226ca2c334f5695988122838859db-switcheroo-control.service-Hjr1ct', 'systemd-private-eb8226ca2c334f56
95988122838859db-fwupd.service-qTgR60', 'systemd-private-eb8226ca2c334f5695988122838859db-chrony.service-t4KXch', 'systemd-p
rivate-eb8226ca2c334f5695988122838859db-upower.service-JSMJ4U', 'systemd-private-eb8226ca2c334f5695988122838859db-demo.servi
ce-N9L3Q1']
12c12
< Service does not have permission to list contents of /home
---
> Contents of /home (showing -1 items, but there are 2 total): ['other-user']
16c16
< Contents of /dev (showing 10 items, but there are 17 total): ['stderr', 'stdout', 'stdin', 'fd', 'tty', 'urandom', 'random
', 'full', 'zero', 'null']
---
> Contents of /dev (showing 10 items, but there are 399 total): ['printer', 'i2c-11', 'nvidia-modeset', 'zram5', 'zram4', 'z
ram3', 'zram2', 'zram1', 'zram0', 'nvme-fabrics']
21c21
< Contents of /proc (showing 10 items, but there are 56 total): ['fb', 'fs', 'bus', 'irq', 'mtd', 'net', 'sys', 'tty', 'keys
', 'kmsg']
---
> Contents of /proc (showing 10 items, but there are 316 total): ['fb', 'fs', 'bus', 'irq', 'mtd', 'net', 'sys', 'tty', 'key
s', 'kmsg']
25c25
< Could not access the internet.
---
> Successfully connected to the internet. HEAD response: 302 Found
29c29
< Could not create a socket.
---
> Successfully created a socket: <socket.socket [closed] fd=-1, family=AddressFamily.AF_INET, type=SocketKind.SOCK_STREAM, p
roto=0>
37c37
< /var/log/syslog appears to be owned by the 'nobody' user!
---
> /var/log/syslog appears to be owned by the 'syslog' user!
44c44
< Calling 'modprobe spidev' failed.
---
> Calling 'modprobe spidev' succeeded.
48c48
< Could not open /proc/kallsyms for reading!
---
> Successfully opened /proc/kallsyms for reading. First line: 'ffffa829527d0000 T _text\n'
51c51
< / is mounted read-only!
---
> / is NOT mounted read-only!
60c60
< Could not create mmap which is both executable and writeable.
---
> Successfully created an mmap which is both executable and writeable.
63c63
< POSIX shared memory object (for IPC) created at /sandboxed.
---
> POSIX shared memory object (for IPC) created at /unsandboxed.
67c67
< File created with the following permissions: 0o600
---
> File created with the following permissions: 0o644
73c73
< demo.service: Deactivated successfully.
---
> demo_unsandboxed.service: Deactivated successfully.
75c75
< Systemd already removed the shared memory object at /sandboxed! Error code for opening: 2. Error message: 'No such file or directory'
---
> Systemd did NOT remove the shared memory object at /unsandboxed. Contents of object: 'hello'
```
