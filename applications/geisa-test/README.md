<!--
README.md

Project documentation for building, configuring, and extending the standalone
GEISA geisa-test application.

Copyright 2026 PragSol Consulting LLC.
SPDX-License-Identifier: Apache-2.0
-->

<!-- markdownlint-configure-file {"MD060": {"style": "aligned"}} -->

# geisa-test

`geisa-test` is a standalone GEISA application that generates API traffic and
resource loads on a GEISA-conformant LEE edge environment. It can be used as an
example application, for manual testing, or as a workload source for a test or
conformance harness. It is not itself a GEISA conformance test.

The initial target is GEISA v0.9.0 and the corresponding `geisa/schemas`
release. The application covers basic application behavior plus selected API
and LEE tests, with additional tests expected over time.

## Project information

<!-- markdownlint-disable MD013 -->
| Item                | Value                                                                         |
| ------------------- | ----------------------------------------------------------------------------- |
| Status              | Experimental                                                                  |
| Maintainer          | [PragSol Consulting LLC](https://pragsolconsulting.com)                       |
| License             | Apache-2.0                                                                    |
| Application version | 0.8.9                                                                         |
| GEISA compatibility | [Specification v0.9.0](https://spec.geisa-energy.org/0.9.0/)                  |
| Schema dependency   | [`geisa/schemas` tag `v0.9.0`](https://github.com/geisa/schemas/tree/v0.9.0)  |
<!-- markdownlint-enable MD013 -->

The GEISA Community repository is the authoritative source for this
application. Please refer to the repository's guidance for contributions:
[Contributing](https://github.com/geisa/community/blob/main/CONTRIBUTING.md).

## Support and maintenance

`geisa-test` is maintained as an Experimental GEISA Community contribution.
Issues should be reported in the
[GEISA Community issue tracker](https://github.com/geisa/community/issues).
For contributions, use the
[contribution guidance](https://github.com/geisa/community/blob/main/CONTRIBUTING.md).
PragSol Consulting LLC is the original contributor and initial maintainer.
Community applications are provided without a support commitment or SLA.

`geisa-test` is versioned independently from GEISA. Releases identify the
GEISA specification and schema version they target; application release
`0.8.9` currently targets GEISA v0.9.0.

## Build and developer smoke tests

### Prerequisites

The build requires a C11 compiler, GNU Make, Git, Python 3 with `venv` and pip
support, the Protocol Buffers compiler (`protoc`), `pkg-config`, and the
libmosquitto development headers and library. The compiler and platform must
provide pthread support for `-pthread`.

On Debian or Ubuntu, common package names are `build-essential`, `git`,
`python3`, `python3-venv`, `protobuf-compiler`, `pkg-config`, and
`libmosquitto-dev`. Package names vary across distributions.

Network access is needed when dependency setup fetches the pinned sources and
installs nanopb. Once setup is complete, builds, tests, and generation use the
local dependencies.

### Dependency setup

`make setup-dev` prepares ignored, app-local development dependencies under
`.deps/` and `.venv/`. By default it fetches `geisa/schemas` at tag `v0.9.0`
and nanopb `0.4.9.1`, then installs the nanopb generator into the virtual
environment.

To use an existing schemas checkout, set `GEISA_SCHEMAS_DIR` to its root:

```sh
GEISA_SCHEMAS_DIR=/path/to/schemas make setup-dev
```

The directory must contain the GEISA `.proto` files and `nanopb_options/`.
`make setup-dev` also prepares the pinned nanopb checkout and Python
environment.

### Build and local tests

```sh
make setup-dev
make
make test
make verify-generated
```

`make` builds `build/geisa-test`. `make test` builds and runs the app-local C
smoke tests. `make verify-generated` checks the generated protobuf bindings.

Generated nanopb sources are kept under `build/nanopb/`; executables, test
programs, object files, and dependency information also stay under the ignored
`build/` tree.

The local smoke tests exercise runtime implementation paths and handlers using
test fixtures. They do *not* exercise those paths end-to-end against a
live GEISA platform or MQTT broker.

The smoke tests are local implementation tests. The runtime tests below run
through the GEISA APIs and depend on the platform for the behavior being tested.

Primary build targets:

* `make check-dev-deps` — verifies the required local schemas, nanopb, Python
  environment, and libmosquitto development files are available.
* `make regenerate` — regenerates nanopb bindings under `build/nanopb/`.
* `make verify-generated` — checks the generated bindings are present and
  records dependency information.
* `make dependency-info` — shows the schemas and nanopb revisions in use.
* `make clean` — removes the `build/` tree, while local dependencies in
  `.deps/` and `.venv/` are kept.
* `make help` — shows the available targets.

The test targets group the local smoke tests by area:

<!-- markdownlint-disable MD013 -->

| Target                | Covers                                                                                                    |
| --------------------- | --------------------------------------------------------------------------------------------------------- |
| `make test-api`       | Messaging, CONFIG/COMMAND handling, response correlation, accounting, quotas, and reporting configuration |
| `make test-lee`       | CPU, memory, persistent-storage, and transient-storage probes                                             |
| `make test-lifecycle` | Subscription setup, startup ordering, status controls, and manifest handling                              |
| `make test-summary`   | Final JSON summary output and escaping                                                                    |
| `make test`           | Build the app and run all local smoke tests                                                               |

<!-- markdownlint-enable MD013 -->

## Runtime operations

The runtime tests exercise specific GEISA API and LEE behavior. `geisa-test`
generates the traffic or resource load and records the application-side result.
Platform-side behavior, including resource enforcement, must be observed
separately.

These tests are not a complete GEISA conformance suite.

<!-- markdownlint-disable MD013 -->

| Pillar    | Area                                                  | Test / operation                     | Test behavior                                                                                                                                                         | Result                                                                                                                                  |
| --------- | ----------------------------------------------------- | ------------------------------------ | --------------------------------------------------------------------------------------------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------- |
| LEE       | Application Isolation / Container Resource Management | `cpu`                                | Runs a bounded CPU workload at the configured intensity. The workload can be set above the CPU allocation in the application manifest.                                | Reports duration, CPU time, work completed, and whether the workload completed.                                                         |
| LEE       | Application Isolation / Container Resource Management | `memory`                             | Allocates and touches memory in chunks up to the configured target, then releases it. The target can be set above the memory allocation in the application manifest.  | Reports requested, allocated, and touched bytes along with completion and cleanup status.                                               |
| LEE       | Application Isolation / Container Resource Management | `persistent_storage`                 | Writes and syncs data under `GEISA_TEST_PERSISTENT_STORAGE_DIR` up to the configured target, then removes the probe file and its empty `geisa-test-probes` directory. | Reports bytes written, failures, and cleanup status. The configured storage root is not removed.                                        |
| LEE       | Application Isolation / Container Resource Management | `transient_storage`                  | Writes and syncs data under `GEISA_TEST_TRANSIENT_STORAGE_DIR` up to the configured target, then removes the probe file and its empty `geisa-test-probes` directory.  | Reports bytes written, failures, and cleanup status. The configured storage root is not removed.                                        |
| API       | Application Messaging and Configuration               | Normal reporting                     | Sends recurring EVENT, ALARM, APP_DATA, and/or TELEMETRY messages using the configured message types and reporting interval.                                          | Correlates and counts responses. `reporting_message_types` selects the messages and `reporting_interval_seconds` sets the interval.     |
| API       | Application Messaging and Configuration               | `payload`                            | Sends one application message with the configured payload size and pattern. A size of zero uses the normal payload for the selected message type.                     | Correlates the response and records the payload bytes sent.                                                                             |
| API       | Application Messaging and Configuration               | `burst`                              | Sends `burst_count` normal application messages separated by `burst_interval_ms`.                                                                                     | Correlates and counts each response.                                                                                                    |
| API       | Application Messaging and Configuration               | `quota`                              | Sends application messages until the platform reports `GEISA_APP_MESSAGE_STATUS_QUOTA_EXCEEDED` or `quota_max_attempts` is reached.                                   | Passes when the platform reports quota exceeded. Fails if the attempt limit is reached first.                                           |
| API       | Application Messaging and Configuration               | CONFIG `set_configuration`           | Changes supported test, reporting, payload, message-type, and resource settings.                                                                                      | Accepts a valid configuration and rejects invalid or unsupported values. Selecting test mode and a test schedules one run.              |
| API       | Application Messaging and Configuration               | CONFIG `get_effective_configuration` | Reads the current effective configuration, either in full or for selected keys.                                                                                       | Prints the selected values to stdout and returns an accepted response.                                                                  |
| API       | Application Messaging and Configuration               | COMMAND `run_once`                   | Runs the currently selected test again. Outside test mode, triggers one normal reporting cycle.                                                                       | Returns an accepted response when the run is queued or started. Test results are reported through the normal API or probe result paths. |
| API       | Application Messaging and Configuration               | COMMAND `report_counters`            | Reports the current message attempt, accepted, and rejected counters.                                                                                                 | Prints the counters to stdout without changing them.                                                                                    |
| API       | Application Messaging and Configuration               | COMMAND `reset_counters`             | Resets the message counters. Probe state is cleared only when no probe is running.                                                                                    | Resets the supported counters without resetting unrelated final-summary state.                                                          |
| API       | API Architecture                                      | Startup subscriptions                | Subscribes to the API topics required by the application and waits for all required SUBACKs before continuing startup.                                                | Startup stops if a required subscription fails. Lower MQTT QoS grants are accepted when allowed by MQTT.                                |
| API       | Platform Discovery                                    | Platform Discovery                   | Requests platform Discovery after the required subscriptions are ready.                                                                                               | Requires a successful Discovery response before test execution starts.                                                                  |
| API       | Platform Discovery                                    | Application Deployment Manifest      | Requests the application's current Deployment Manifest during startup.                                                                                                | Records successful, failed, malformed, and timed-out responses. Manifest failure causes the final result to fail.                       |
| API       | Platform and App Status                               | RUNNING / status request             | Publishes RUNNING during startup and when the platform requests application status.                                                                                   | Publishes application status at QoS 0. Status handling is independent of normal reporting.                                              |
| API       | Platform and App Status                               | Periodic status                      | Publishes RUNNING every 60 seconds while the application remains active.                                                                                              | Provides periodic application status independently of the normal reporting interval.                                                    |
| API       | Platform and App Status                               | CLEAR_PII                            | Handles the platform CLEAR_PII control. `geisa-test` does not currently retain user PII.                                                                              | Publishes `CLEARED_PII` without clearing unrelated test, configuration, or protocol state.                                              |
| API       | Platform and App Status                               | Shutdown                             | Handles a platform-directed shutdown or local SIGINT/SIGTERM.                                                                                                         | Publishes SHUTTING_DOWN, stops an active probe, writes the final summary, and exits.                                                    |
| API       | Application Messaging and Configuration               | `api_all`                            | Runs `payload`, `burst`, and `quota` in sequence.                                                                                                                     | Reports each test result and the aggregate API result.                                                                                  |
| LEE       | Application Isolation / Container Resource Management | `lee_all`                            | Runs `cpu`, `memory`, `persistent_storage`, and `transient_storage` in sequence.                                                                                      | Reports each workload result and the aggregate LEE result. Both storage roots must be configured.                                       |
| API + LEE | Combined                                              | `all`                                | Runs the API and LEE tests in the defined sequence.                                                                                                                   | Reports each test result and the final aggregate result. Both storage roots must be configured.                                         |

### Runtime control and results

CONFIG and COMMAND requests are GEISA downstream `GeisaAppMessage_Req`
protobuf envelopes with `content_type` set to `application/json`. Send them to
`geisa/api/message/downstream/req/<USERID>`; responses arrive on
`geisa/api/message/downstream/rsp/<USERID>` as GEISA app-message responses.
There is currently no runtime CLI for selecting or starting tests.

Select `payload` with CONFIG. A valid test selection schedules one run:

```json
{"operation":"set_configuration","values":{"mode":"test","test":"payload"}}
```

Select the API aggregate the same way:

```json
{"operation":"set_configuration","values":{"mode":"test","test":"api_all"}}
```

After the selected run finishes, COMMAND can request another run:

```json
{"command":"run_once"}
```

Read back selected effective values:

```json
{"operation":"get_effective_configuration","keys":["mode","test"]}
```

A successful CONFIG response means the configuration was accepted; it does not
mean the selected test passed. API diagnostics and command results are written
to stdout and the response topic. `geisa-test-summary` is written to stdout
when the process exits. The application normally remains running after a test
completes.

`final_pass`, counters, aggregate state, and probe measurements are
application-side results. Quota decisions, LEE enforcement, and platform
resource measurements must be checked separately.

For a conformant v0.9.0 LEE deployment, the platform provides
`/etc/geisa/mqtt.conf` inside the application container. It contains `HOST`,
`PORT`, `USERID`, and `PASSWORD`.

For local development, `geisa-test` falls back to `localhost:1883` if the file
is absent. That fallback is not an alternate LEE deployment mechanism. Refer
to the
[API Architecture](https://spec.geisa-energy.org/0.9.0/api/architecture.html)
for the platform contract.

`GEISA_TEST_APP_CONFIG_FILE` provides local application settings and test
bindings. `GEISA_TEST_APP_HOST`, `GEISA_TEST_APP_PORT`, and
`GEISA_TEST_APP_USERID` can override the local broker connection; equivalent
`HOST`, `PORT`, and `USERID` keys may be placed in the local config file.
These development controls do not replace `/etc/geisa/mqtt.conf` in a LEE
deployment.

Storage tests use `GEISA_TEST_PERSISTENT_STORAGE_DIR` and
`GEISA_TEST_TRANSIENT_STORAGE_DIR`. See
[`config/example.env`](config/example.env) for the remaining local test
settings.

## Adding new runtime tests

Runtime tests are defined in `src/geisa-test.c`. Add the test name to
`test_mode_from_string` and `test_mode_to_string`, then add its execution path
in `start_pending_test` and either `dispatch_next_message` or the probe path as
appropriate.

Tests included in `api_all`, `lee_all`, or `all` are listed explicitly in
`configure_aggregate`. Keep that ordering explicit rather than deriving it
from the enum or registration order.

New configuration values belong in `struct app_options` and the CONFIG
handling path. Update `config/config.json` at the same time.

API response handling belongs in `on_message`; test completion goes through
`complete_current_test`.

Local smoke tests live under `tools/` and are separate from runtime test
registration. Add the smoke executable to `TESTS` and the appropriate
`test-*` Make target.

The GEISA Community repository is the intended upstream; follow its
contribution guidance when preparing a contribution.

Copyright 2026 PragSol Consulting LLC. Licensed under Apache-2.0; see
[`LICENSE`](LICENSE).
