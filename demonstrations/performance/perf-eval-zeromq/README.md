# ZeroMQ Performance Evaluation

Rust publisher and subscriber programs for evaluating ZeroMQ throughput and
latency on constrained edge hardware.

## Relationship to GEISA

This project contains performance experiments created during GEISA messaging
architecture work.

The publisher sends timestamped data to the subscriber, which reports
throughput and optional latency measurements. The programs were used to
evaluate ZeroMQ as a possible communication mechanism for GEISA-related
workloads.

## Project Information

- **Status:** Maintenance Limited
- **Maintainer:** Rick Steurer (`@ricksteu-utilidata`), original author
- **License:** Apache-2.0
- **GEISA versions tested or supported:** Not tied to a specific GEISA release
- **Support:** Best-effort

## Building or Using the Project

The project contains two independent Rust programs:

- `zmqpub` — publishes timestamped data
- `zmqsub` — receives data and reports performance information

Both projects retain their original `Cargo.lock` files.

Build each program from the project directory:

```console
cd zmqpub
cargo build --locked --release

cd ../zmqsub
cargo build --locked --release
```

Run the subscriber first:

```console
cd zmqsub
cargo run --locked -- -e
```

Then run the publisher from another terminal:

```console
cd zmqpub
cargo run --locked
```

Use `--help` to view the available options:

```console
cargo run --locked -- --help
```

## Cross-Compilation

For a 64-bit Raspberry Pi target:

```console
rustup target add aarch64-unknown-linux-gnu
cargo build --locked --release --target aarch64-unknown-linux-gnu
```

For targets that are difficult to build with a locally configured
cross-toolchain, the `cross` project may be used:

```console
cross build --locked --release --target arm-unknown-linux-gnueabihf
```

Copy the resulting binaries using paths and addresses appropriate for the
local environment. For example:

```console
scp zmqpub/target/aarch64-unknown-linux-gnu/release/zmqpub \
  user@edge-device:~/zmqpub

scp zmqsub/target/aarch64-unknown-linux-gnu/release/zmqsub \
  user@edge-device:~/zmqsub
```

The binaries may need executable permission after transfer:

```console
chmod +x zmqpub zmqsub
```

## Known Limitations

- The programs provide focused performance measurements rather than a complete
  messaging benchmark suite.
- Results vary with hardware, operating system, Rust toolchain, ZeroMQ version,
  message configuration, and system load.
- The programs use local ZeroMQ IPC transport.
- Cross-compilation requires an appropriate target toolchain or `cross`
  environment.
- Benchmark results were not regenerated as part of the Community migration.
- Maintenance is limited, and broad dependency modernization is outside the
  migration scope.
