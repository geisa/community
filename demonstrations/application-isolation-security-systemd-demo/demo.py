#!/usr/bin/env python
"""
This script takes a single positional argument: a
filename for creating a file in /dev/shm.

As of 2025-05-12, this script has only been tested with
Python 3.10.12 on a machine running an Ubuntu 22.04
variant.

This script generally just probes the system to inspect
files/directories and logs the output. Most operations
are "non-destructive"/read-only with some exceptions
(e.g. attempt to load spidev kernel module, create socket
to connect to the internet).
"""

import ctypes
import http.client
import mmap
import os
import pwd
import socket
import stat
import subprocess
import sys

# shutil.get_terminal_size() does not work properly here.
# Using subprocess + tput also doesn't work due to lack of
# a set $TERM variable over the ssh connection. Setting the
# $TERM variable to "xterm" doesn't get us the proper width.
# So... this is more or less a hard-code of 125. Oh well,
# good enough for this demo.
STARS = "*" * int(os.environ.get("COLUMNS", 125))


def main(filename: str):
    """Call all helper functions to string everything together."""
    print(STARS)
    print("Starting up demo service script.")
    print(STARS)
    _explore_tmp()
    _explore_home()
    _explore_dev()
    _explore_proc()
    _network_request()
    _create_internet_socket()
    _namespaces()
    _stat_syslog()
    _stat_cgroups()
    _modprobe()
    _read_kallsyms()
    _protect_system()
    _chown()
    _sched()
    _mmap()
    _ipc(filename)
    _umask()
    _hostname()
    print("All done, exiting.")
    print(STARS)


def _list_dir(_dir: str, limit: int = -1):
    """Attempt to list out (log) the contents of a directory up
    to the provided ``limit`` number of listings.
    """
    print(f"Exploring {_dir}...")
    try:
        _listing = os.listdir(_dir)
    except PermissionError:
        print(f"Service does not have permission to list contents of {_dir}")
    else:
        if _listing:
            print(
                f"Contents of {_dir} (showing {limit} items, "
                f"but there are {len(_listing)} total): {_listing[:limit]}"
            )
        else:
            print(
                f"{_dir} is either empty or the service is not "
                "allowed to see the contents!"
            )


def _explore_tmp():
    """Attempt to list out the contents of /tmp."""
    print(STARS)
    print("Systemd directive: PrivateTmp")
    _list_dir("/tmp")


def _explore_home():
    """Attempt to list out the contents of /home."""
    print(STARS)
    print("Systemd directive: ProtectHome")
    _list_dir("/home")


def _explore_dev():
    """Attempt to list out the contents of /dev."""
    print(STARS)
    print("Systemd directive: PrivateDevices")
    _list_dir("/dev", 10)


def _explore_proc():
    """Attempt to list out the contents of /proc."""
    print(STARS)
    print("Systemd directive: ProtectProc")
    print(
        "Note that ProtectProc is only effective for services "
        "which do not run as the root user."
    )
    _list_dir("/proc", 10)


def _network_request():
    """Attempt to establish a connection to 8.8.8.8 and make
    a HEAD request.
    """
    print(STARS)
    print("Systemd directive: PrivateNetwork")
    print("Attempting to access the internet...")
    conn = http.client.HTTPSConnection("8.8.8.8", timeout=0.5)
    try:
        conn.request("HEAD", "/")
    except OSError:
        print("Could not access the internet.")
    else:
        resp = conn.getresponse()
        print(
            "Successfully connected to the internet. "
            f"HEAD response: {resp.status} {resp.reason}"
        )


def _create_internet_socket():
    """Attempt to create an IPv4 socket."""
    print(STARS)
    print("Systemd directive: RestrictAddressFamilies")
    print("Attempting to create an IPv4 internet socket...")
    try:
        with socket.socket(family=socket.AF_INET) as _socket:
            pass
    except OSError:
        print("Could not create a socket.")
    else:
        print(f"Successfully created a socket: {_socket}")


def _namespaces():
    """Does nothing right now besides log."""
    print(STARS)
    print("Systemd directive: RestrictNamespaces")
    print("Proving out namespace restrictions is an outstanding TODO.")
    print(
        "os.setns and os.unshare were only introduced in Python 3.12 "
        "which is not yet standard system Python, and this script is "
        "intended to be portable."
    )


def _stat_and_log(path):
    """Log 'n stat"""
    print(f"stat'ing {path}...")
    return os.stat(path)


def _stat_syslog():
    """Log who owns the /var/log/syslog file."""
    print(STARS)
    print("Systemd directive: PrivateUsers")
    _path = "/var/log/syslog"
    stat_result = _stat_and_log(_path)
    if stat_result.st_uid == pwd.getpwnam("nobody").pw_uid:
        print(f"{_path} appears to be owned by the 'nobody' user!")
    elif stat_result.st_uid == 0:
        print(
            f"{_path} appears to be owned by the 'root' user, "
            "which is unexpected!"
        )
    elif stat_result.st_uid == pwd.getpwnam("syslog").pw_uid:
        print(f"{_path} appears to be owned by the 'syslog' user!")
    else:
        print(f"Unsure which user owns {_path}. UID: {stat_result.st_uid}")


def _stat_cgroups():
    """Doesn't do anything besides log right now."""
    print(STARS)
    print("Systemd directive: ProtectControlGroups")
    print(
        "Have not yet found a good way to prove out protection "
        "of /sys/fs/cgroup via the ProtectControlGroups directive..."
    )
    # _path = "/sys/fs/cgroup"
    # stat_result = _stat_and_log(_path)
    # mode = stat_result.st_mode
    # owner_write = 0b010000000
    # if owner_write & mode:
    #     print(f"{_path} is writeable by the owner!")
    # else:
    #     print(f"{_path} is NOT writeable by the owner!")
    # result = subprocess.run(["findmnt", "/sys/fs/cgroup"], capture_output=True)
    # print(result.stdout.decode("utf-8"))
    # _list_dir(_path)


def _modprobe():
    """Attempt to load the spidev kernel module."""
    print(STARS)
    print("Systemd directive: ProtectKernelModules")
    print("Attempting to call modprobe to load the spidev kernel module...")
    try:
        subprocess.check_call(
            ["modprobe", "spidev"],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
    except subprocess.CalledProcessError:
        print("Calling 'modprobe spidev' failed.")
    else:
        print("Calling 'modprobe spidev' succeeded.")


def _read_kallsyms():
    """Attempt to read a line from /proc/kallsyms."""
    print(STARS)
    print("Systemd directive: ProtectKernelTunables")
    _path = "/proc/kallsyms"
    print(f"Attempting to read {_path}")
    try:
        with open(_path) as f:
            line = f.readline()
    except PermissionError:
        print(f"Could not open {_path} for reading!")
    else:
        print(f"Successfully opened {_path} for reading. First line: {line!r}")


def _protect_system():
    """Log whether / is mounted read-only or not."""
    print(STARS)
    print("Systemd directive: ProtectSystem")
    result = os.statvfs("/")
    if result.f_flag & os.ST_RDONLY:
        print("/ is mounted read-only!")
    else:
        print("/ is NOT mounted read-only!")


def _chown():
    """Does nothing but log right now."""
    print(STARS)
    print("Systemd directive: RestrictSUIDSGID")
    # chown/chgrp not working even for valid invocations and
    # with RestrictSUIDSGID=false.
    # I believe due to "nosuid" mounting.
    print(
        "This service can only write to /tmp and /var/tmp, "
        "but those directories are mounted with 'nosuid' and "
        "as such it's very challenging (if not impossible) to "
        "prove out the RestrictSUIDSGID setting."
    )


def _sched():
    """Does nothing but log right now."""
    print(STARS)
    print("Systemd directive: RestrictRealtime")
    print(
        "With other service restrictions in place, it is difficult "
        "or impossible to modify the service schedule."
    )
    # param = os.sched_param(os.sched_get_priority_max(os.SCHED_FIFO))
    # os.sched_setscheduler(0, os.SCHED_FIFO, param)
    # param = os.sched_param(os.sched_get_priority_max(os.SCHED_RR))
    # os.sched_setscheduler(0, os.SCHED_RR, param)


def _mmap():
    """Attempt to create a memory map which is both executable and
    writeable.
    """
    print(STARS)
    print("Systemd directive: MemoryDenyWriteExecute")
    _path = "/tmp/mmap.txt"
    with open(_path, "wb") as f:
        f.write(b"Hello, memory map")

    with open(_path, "r+b") as f:
        try:
            mm = mmap.mmap(
                f.fileno(), length=0, prot=mmap.PROT_EXEC | mmap.PROT_WRITE
            )
        except PermissionError:
            print(
                "Could not create mmap which is both executable and writeable."
            )
        else:
            mm.close()
            print(
                "Successfully created an mmap which is both executable and writeable."
            )
    os.remove(_path)


def _ipc(filename: str):
    """
    Create a shared memory object using shm_open.
    """
    print(STARS)
    print("Systemd directive: RemoveIPC")

    libc = ctypes.CDLL("libc.so.6", use_errno=True)
    content = b"hello"

    fd = libc.shm_open(f"/{filename}".encode(), os.O_CREAT | os.O_RDWR, 0o600)

    if fd == -1:
        raise OSError(ctypes.get_errno(), os.strerror(ctypes.get_errno()))

    os.ftruncate(fd, len(content))

    with mmap.mmap(fd, 0) as mm:
        mm.write(content)

    os.close(fd)

    print(f"POSIX shared memory object (for IPC) created at /{filename}.")


def _umask():
    """Create a file in /tmp with default permissions, log mode/permissions,
    remove file.
    """
    print(STARS)
    print("Systemd directive: UMask")
    # Originally, I used tempfile like so:
    #
    # with tempfile.NamedTemporaryFile(mode="w") as f:
    #
    # However, it would appear that tempfile creates files which are
    # not world-readable by default, and the point here is to prove
    # the systemd UMask directive alters default world-readable permissions.
    with open("/tmp/test.txt", "w") as f:
        stat_result = _stat_and_log(f.name)
        print(
            "File created with the following permissions: "
            f"{oct(stat.S_IMODE(stat_result.st_mode))}"
        )

    os.remove("/tmp/test.txt")


def _hostname():
    """Currently only logs."""
    print(STARS)
    print("Systemd directive: ProtectHostname")
    print(
        "Not proving this out for now, as the RestrictAddressFamilies=none "
        "directive prevents the use of hostnamectl."
    )

    # The idea was to call `hostnamectl hostname` to get the hostname,
    # then again call `hostnamectl hostname <hostname>` to prove out
    # the protection in place. This needs paired with passwordless sudo for
    # the service user to call hostnamectl, which can be achieved by creating
    # the file "/etc/sudoers.d/hostnamectl" with the following contents:
    # "<user> ALL = (root) NOPASSWD: /usr/bin/hostnamectl"
    # where "<user>" should be replaced with the service user.
    #
    # However, RestrictAddressFamilies=none completely prevents using the
    # hostnamectl command (get the following error: "Failed to connect to bus:
    # Address family not supported by protocol," presumably because
    # RestrictAddressFamilies=none prevents working with, e.g., local Unix
    # sockets, and it would appear hostnamectl likely uses a Unix socket under
    # the hood.), so we'll leave this code in for future reference.
    # query_result = subprocess.run(
    #     ["/usr/bin/hostnamectl", "hostname"], capture_output=True, check=False
    # )
    # print(query_result.stderr.decode("UTF-8"))
    # try:
    #     subprocess.run(
    #         ["/usr/bin/hostnamectl", "hostname", query_result.stdout],
    #         capture_output=True,
    #         check=True,
    #     )
    # except subprocess.CalledProcessError:
    #     print("Service could NOT change the hostname.")
    # else:
    #     print("Service COULD change the hostname.")


if __name__ == "__main__":
    main(sys.argv[1])
