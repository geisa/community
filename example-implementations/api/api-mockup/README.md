[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](https://opensource.org/licenses/Apache-2.0)

# GEISA API Mockup - a GEISA Application Programming Interface mockup implementation

GEISA API Mockup is designed to be able to run conformance tests. It aims to provide a mockup implementation of the GEISA Application Programming Interface as defined in the
[GEISA Specification](https://github.com/geisa/specification).

The GEISA Specification is an effort by the Grid Edge Interoperability and Security Alliance to define a consistent, secure, and interoperable computing environment for embedded devices at the very edge of the electric utility grid, like electric meters and distribution automation devices, for the benefit of utilities, platform vendors, and software vendors. If you would like to get involved, please head over to our Wiki page for details on participation (https://lfenergy.org/projects/geisa/).  Follow the onboarding link for details about participating in our community process.

## Getting started

To get started with the GEISA API Mockup application, you need to clone the repository and initialize the submodules.

```bash
git submodule update --init --recursive
```

## Building the application

### Requirements

To build the GEISA API Mockup application, you need to have the following tools installed on your system:
  - make
  - libmosquitto-dev

A docker support is also available to launch the compilation, it requires:
  - cqfd (See [requirements](https://github.com/savoirfairelinux/cqfd?tab=readme-ov-file#requirements) and [installation](https://github.com/savoirfairelinux/cqfd?tab=readme-ov-file#installingremoving-cqfd) steps on github)

### Build using make
To build the GEISA API Mockup application using make, run the following command in the root directory of the project:

```bash
make
```

### Build using cqfd
To build the GEISA API Mockup application using cqfd, run the following command in the root directory of the project:

```bash
cqfd run
```

## Launch the application

To launch the GEISA API Mockup application, simply run the following command:

```bash
./gapi
```

## Linting and formatting

Linting is done using `clang-tidy` and formatting using `clang-format`.
To lint and format the GEISA API Mockup application code, run the following
command:

```bash
make lint
```

or with cqfd:

```bash
cqfd -b lint
```
