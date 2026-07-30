# IPC Timings

Code samples and a historical report comparing several inter-process
communication mechanisms.

## Relationship to GEISA

This project contains performance investigations created during GEISA
architecture work.

It includes small C programs for comparing direct memory copy, shared memory,
TCP, UDP, ZeroMQ, and D-Bus communication.

The included report records the results and test context from the original
evaluation.

## Project Information

* **Status:** Maintenance Limited
* **Maintainer:** Norm McEntire (@nmcentire400), original author
* **License:** Apache-2.0
* **GEISA versions tested or supported:** Not tied to a specific GEISA release
* **Support:** Best-effort

## Building or Using the Project

The examples are independent C programs and do not share a common build system.

Each program can be compiled directly with a suitable C compiler. For example:

```
cc -O2 memcpy.c -o memcpy
```

The available examples are:

* `memcpy.c` — direct memory-copy baseline
* `shmemcpy.c` — shared-memory transfer
* `tcpmemcpy.c` — local TCP transfer
* `udpmemcpy.c` — local UDP transfer
* `zmqmemcpy.c` — local ZeroMQ transfer
* `dbusmemcpy.c` — D-Bus transfer

Some examples require additional development libraries:

* `zmqmemcpy.c` requires ZeroMQ
* `dbusmemcpy.c` requires D-Bus

The historical report is available as:

* `GEISA-IPC-Timings-2025-06-24.pdf`

The report and source files are preserved from the original repository without
changing the recorded benchmark results.

## Known Limitations

* There is no common Makefile or automated build.
* The examples were created for a specific evaluation environment and may
  require platform-specific adjustments.
* Benchmark results vary with hardware, operating system, compiler settings,
  library versions, message size, iteration count, and system load.
* The fixed loopback addresses and ports are intended for local test execution.
* ZeroMQ and D-Bus examples require their respective development packages.
* The benchmarks were not rerun as part of the Community repository migration.
* Maintenance is limited, and broader modernization is outside the scope of
  the migration.
