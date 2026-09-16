<!--
File: README.md
Project: geisa-simple
Purpose: Provides a simple example GEISA-conformant edge application

Copyright 2026 PragSol Consulting LLC.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

SPDX-License-Identifier: Apache-2.0
-->

# geisa-simple

`geisa-simple` is a small example GEISA application. It shows lifecycle status,
startup discovery and manifest requests, sends recurring upstream EVENT messages
by default every 60 seconds, and provides a simple CONFIG example. CONFIG can
change the reporting interval.

`geisa-simple` is intended for example, reference, and basic testing use. It is
not optimized for high-frequency or high-bandwidth processing, and it has not
been designed or validated as a production application.  The implementation
choices made for this app, including message and configuration handling, are
examples and are not the only way to satisfy the specification.

Use it as a starting point, not as production-ready software.

The initial release targets GEISA 0.9.0 behaviors; subsequent releases may
be done as the GEISA specification progresses.

## Supported functionality

| Functionality                 | GEISA 0.9.0 | geisa-simple  |
| ----------------------------- | ----------- | ------------- |
| Lifecycle and status          | Required    | Supported     |
| Global platform status        | Required    | Supported     |
| Platform Discovery            | Required    | Supported     |
| Deployment Manifest retrieval | Conditional | Supported     |
| Platform status requests      | Required    | Supported     |
| Platform shutdown             | Required    | Supported     |
| Application messaging         | Conditional | Supported     |
| EVENT messages                | Conditional | Supported     |
| CONFIG messages               | Conditional | Supported     |
| COMMAND messages              | Conditional | Not supported |
| Sensor APIs                   | Conditional | Not supported |
| Instantaneous data            | Conditional | Not supported |
| Waveform data                 | Conditional | Not supported |
| Actuator APIs                 | Conditional | Not supported |
| Direct/off-device networking  | Conditional | Not used      |
| PII                           | Conditional | Not used      |

`Conditional` means the GEISA requirements apply when an application uses that
capability. See the
[GEISA 0.9.0 specification](https://github.com/geisa/specification/tree/v0.9.0)
for the complete requirements.

## GEISA application baseline

This is a short guide to the GEISA 0.9.0 application baseline, not a replacement
for the specification.

| Requirement            | What an application does                                                                                                          |
| ---------------------- | --------------------------------------------------------------------------------------------------------------------------------- |
| Platform configuration | Uses the API connection information and platform credentials, normally through `/etc/geisa/mqtt.conf` on a LEE                    |
| MQTT subscriptions     | Subscribes to the response and control topics it needs and waits for successful SUBACKs before sending requests depending on them |
| Global platform status | Subscribes to `geisa/api/platform/status` and logs valid broadcast notifications                                                   |
| Application status     | Publishes `RUNNING` when the application is operational                                                                           |
| Platform Discovery     | Calls the Platform Discovery API on every startup                                                                                 |
| Deployment Manifest    | Retrieves the current manifest when the application uses one                                                                      |
| API requests           | Publishes GEISA API requests using MQTT QoS 1                                                                                     |
| Status requests        | Responds when the platform requests an immediate application status update                                                        |
| Periodic status        | Continues publishing application status at the advertised interval when an interval is configured                                 |
| Platform shutdown      | Accepts a platform-initiated shutdown request                                                                                     |
| Shutdown status        | Publishes `SHUTTING_DOWN` when a clean shutdown begins                                                                            |

`geisa-simple` waits for the subscriptions it needs to receive successful
SUBACKs, publishes RUNNING status, then runs Platform Discovery and requests
its Deployment Manifest. It then validates both responses before beginning
sending periodic EVENT messages.

See [API architecture](https://github.com/geisa/specification/blob/v0.9.0/source/api/architecture.rst),
[API status](https://github.com/geisa/specification/blob/v0.9.0/source/api/status.rst),
and [Platform Discovery](https://github.com/geisa/specification/blob/v0.9.0/source/api/discovery.rst)
for the authoritative requirements.

## Project information

| Item                | Value                                                       |
| ------------------- | ----------------------------------------------------------- |
| Status              | Experimental                                                |
| Maintainer          | [PragSol Consulting LLC](https://pragsolconsulting.com)     |
| License             | Apache-2.0                                                  |
| Application version | 0.9.0                                                       |
| GEISA baseline      | 0.9.0                                                       |
| Schemas tested      | GEISA Specification `schemas-v0.9.0`                         |

`geisa-simple` uses SemVer syntax for versioning. While GEISA is pre-1.0, its
major and minor version identify the GEISA release family it targets, with
Patch releases containing app fixes and small updates. The app version will
not be moved to `1.x.x` until it supports the GEISA 1.x baseline.

## Build requirements

Install or ensure your system has the following tools:

* Git
* GNU Make
* a C compiler
* Python 3 with `venv`
* Protocol Buffers compiler (`protoc`)
* `pkg-config`
* Mosquitto client library and development headers

Then set up the project dependencies and build:

```sh
make setup-dev
make
make test
```

`make setup-dev` will download the pinned GEISA Specification
`schemas-v0.9.0` source and nanopb `0.4.9.1` used to generate the required
GEISA protobuf bindings. This preserved tag exposes the historical Schemas
tree at the Specification checkout root, so the existing protobuf paths remain
unchanged.

Note that the tests run locally and do not require a GEISA LEE or MQTT broker.
They check CONFIG handling, protobuf encoding and decoding, and startup
subscription and message ordering. A GEISA platform is only needed to run the
application itself and test the live API exchanges.

## Running geisa-simple

On a GEISA LEE platform, `/etc/geisa/mqtt.conf` provides the MQTT connection
settings and application credentials. For local development, set
`GEISA_MQTT_CONFIG` to a file with the same fields.

```sh
GEISA_MQTT_CONFIG=/path/to/mqtt.conf ./build/geisa-app
```

## Testing in an LXC container

If you already have a GEISA LEE container with MQTT access and
`/etc/geisa/mqtt.conf` configured, you can copy the built application into the
container and run it directly.

The exact container path and copy method depend on your LEE setup. Once the
binary is available in the container:

```sh
./geisa-app
```

The application should:

1. connect using `/etc/geisa/mqtt.conf`;
2. subscribe to the GEISA topics it uses and wait for successful SUBACKs;
3. publish `RUNNING`;
4. request Platform Discovery;
5. request its Deployment Manifest;
6. begin normal EVENT reporting;
7. remain available for CONFIG, status, and shutdown requests.

From the platform side, you can then verify that the application responds to an
immediate status request and a platform-initiated shutdown. On shutdown it
publishes `SHUTTING_DOWN` before exiting.

For a quick local test inside a container with a different MQTT configuration:

```sh
GEISA_MQTT_CONFIG=/path/to/mqtt.conf ./geisa-app
```

## Schema development

`GEISA_SPECIFICATION_REF` selects a preserved Schemas tag or other Specification
commit whose tree exposes the schemas at the checkout root. Use `GEISA_SPECIFICATION_REPO` to override the
repository URL and `GEISA_SPECIFICATION_DIR` to build from a local
Specification checkout.

```sh
GEISA_SPECIFICATION_REF=<tag|branch|commit> make setup-dev
GEISA_SPECIFICATION_DIR=/path/to/specification make setup-dev
GEISA_SPECIFICATION_DIR=/path/to/specification make regenerate
make dependency-info
```

`make dependency-info` shows the selected schema source and commit and the
nanopb version.

## Packaging and signing

`geisa-simple-manifest.json` is the application manifest. The builder must
provide the toolchain used for the deployable artifact:

```sh
make manifest \
  TOOLCHAIN_ID=your-toolchain-id \
  TOOLCHAIN_VERSION=your-toolchain-version
```

`toolchain-id` and `toolchain-version` identify the build toolchain, not the
GEISA release. The artifact and manifest signature fields are still
placeholders until the publisher supplies image metadata, signatures, and the
final deployment manifest. GEISA deployment artifacts and manifests require
signing.

Note that the `geisa-simple` app does not manage keys or signing; the
application publisher or package workflow should do that work.
