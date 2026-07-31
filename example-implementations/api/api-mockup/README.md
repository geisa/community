# GEISA API Mockup

## Relationship to GEISA

This project is a mock implementation of portions of the GEISA Application
Programming Interface. It was developed primarily to support API conformance
testing and integration work.

It is a Community example implementation, not the GEISA specification or a
complete production platform. The published GEISA specification remains the
authoritative source for API requirements and behavior.

## Project Information

- **Status:** Maintained
- **Maintainer:** Kévin L'hôpital (`@kevlhop`)
- **License:** Apache License 2.0
- **Original repository:** `geisa/api-mockup`
- **Imported source revision:** `423ac34815099fa7bda455099c6ab654b24b3d7d`

The project retains its original Git history and pinned dependencies.

## Building and Using

The project uses two Git submodules:

- nanopb, pinned to the version used by the original implementation
- the GEISA schemas revision used by the original implementation

From the root of the Community repository, initialize them with:

    git submodule update --init --recursive \
      example-implementations/api/api-mockup/nanopb \
      example-implementations/api/api-mockup/schemas

Build from the project directory:

    cd example-implementations/api/api-mockup
    make

The build requires a C compiler, make, pkg-config, protoc, Mosquitto development
files, nanopb runtime development files, and the nanopb Python generator.

The original CQFD build environment remains under `.cqfd/` and can also be used
from the project directory:

    cqfd run

After building, run the API mockup with:

    ./gapi

## Known Limitations

- The implementation covers the API behavior needed by its existing
  conformance and integration tests; it is not a complete GEISA platform.
- It assumes a Linux-style environment with Mosquitto and Unix-domain socket
  support.
- MQTT users, permissions, topics, and supporting platform services must be
  configured separately.
- The schema submodule points to the historical standalone `geisa/schemas`
  repository so the imported build remains reproducible.
- Updating the project to schemas maintained with the GEISA specification
  should be handled as a separate functional change.
- The original pinned nanopb version is retained during migration.
