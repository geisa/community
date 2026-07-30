# GEISA ADM Client Mockup — Wakaama

Wakaama-based Application and Device Management client implementation used for
GEISA conformance development and testing.

## Relationship to GEISA

This project provides a mock implementation of the GEISA ADM client behavior
described by the GEISA Specification.

It is intended to support conformance development and interoperability testing.
It is not a production device-management client.

The implementation is based in part on the Eclipse Wakaama client example code
and adds GEISA-specific software-management and application-package behavior.

## Project Information

- **Status:** Maintenance Limited
- **Maintainers:** Nghia Dam (`@ndam-sfl`) and Kévin L'hôpital (`@kevlhop`), primary historical contributors
- **License:** Mixed; see the project license files and source-file headers
- **GEISA versions tested or supported:** Historical implementation; verify
  against the current GEISA Specification and conformance suites in their
  respective repositories
- **Support:** Best-effort

The project preserves:

- `LICENSE.Apache-2.0`
- `LICENSE.BSD-3-Clause`
- Eclipse Public License 2.0 notices present in Wakaama-derived source files

Individual source-file headers remain authoritative for those files.

## Building or Using the Project

The project requires a separately built Wakaama static library and headers.

Build Wakaama with client support and the required transport and data-format
options. One example configuration is:

```console
git clone https://github.com/eclipse-wakaama/wakaama.git
cd wakaama

cmake -S . -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DWAKAAMA_MODE_CLIENT=ON \
  -DWAKAAMA_MODE_SERVER=OFF \
  -DWAKAAMA_MODE_BOOTSTRAP_SERVER=OFF \
  -DWAKAAMA_CLIENT_INITIATED_BOOTSTRAP=ON \
  -DWAKAAMA_DATA_SENML_JSON=ON \
  -DWAKAAMA_DATA_SENML_CBOR=ON \
  -DWAKAAMA_LOG_LEVEL=INFO \
  -DWAKAAMA_TRANSPORT=TINYDTLS \
  -DWAKAAMA_UNIT_TESTS=OFF \
  -DWAKAAMA_CLI=OFF \
  -DWAKAAMA_PLATFORM=POSIX \
  -DWAKAAMA_ENABLE_EXAMPLES=OFF

cmake --build build
```

Then configure this project with the Wakaama include and library paths:

```console
cmake -S . -B build \
  -DWAKAAMA_INCLUDE_DIR=/path/to/wakaama/include \
  -DWAKAAMA_TINYDTLS_INCLUDE_DIR=/path/to/tinydtls/include \
  -DWAKAAMA_LIB_DIR=/path/to/wakaama/library \
  -DGEISA_PACKAGE_SCRIPT_PATH=/path/to/manage-package-script

cmake --build build
```

Launch the resulting client with an LwM2M server address:

```console
./admclient -4 -h <lwm2m-server-address>
```

Use `-6` instead of `-4` for IPv6 where supported.

## Package Management Behavior

The GEISA-specific package-management script performs privileged platform
operations including:

- mounting application and configuration images
- constructing an overlay filesystem
- managing persistent application storage
- updating MQTT credentials and access-control rules
- mounting the waveform socket
- installing, activating, deactivating, and removing application packages

Review `sources/scripts/manage_package.sh` before using it on a target system.

## Known Limitations

- Wakaama must be built and installed separately before building this project
- Default CMake paths assume system-installed Wakaama headers and libraries
- Package-management behavior assumes Linux, LXC, overlay mounts, Mosquitto
  administration tools, and GEISA platform filesystem conventions
- The implementation includes historical API topic and package-management
  assumptions that may require review against the current Specification
- This migration does not update Wakaama, redesign ADM behavior, or replace the
  implementation with another LwM2M client
- Maintenance is limited and a future client implementation may supersede this
  project
