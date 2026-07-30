#!/usr/bin/env python
"""
Clean up after service run. This script is intended
to be run after the systemd service has stopped.
"""

import ctypes
import mmap
import os
import sys

STARS = "*" * int(os.environ.get("COLUMNS", 125))


def main(filename: str):
    print(STARS)

    libc = ctypes.CDLL("libc.so.6", use_errno=True)

    fd = libc.shm_open(f"/{filename}".encode(), os.O_RDONLY, 0)

    if fd == -1:
        err = ctypes.get_errno()
        print(
            "Systemd already removed the shared memory object at "
            f"/{filename}! Error code for opening: {err}. "
            f"Error message: '{os.strerror(err)}'"
        )
    else:
        with mmap.mmap(fd, 0, mmap.MAP_SHARED, mmap.PROT_READ) as mm:
            contents = mm.read().decode("UTF-8")

        os.close(fd)
        libc.shm_unlink(f"/{filename}")

        print(
            f"Systemd did NOT remove the shared memory object at /{filename}. "
            f"Contents of object: '{contents}'"
        )


if __name__ == "__main__":
    main(sys.argv[1])
