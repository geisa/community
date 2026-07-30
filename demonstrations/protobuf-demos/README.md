# protobuf-demos

Protocol Buffers demonstrations originally developed during GEISA architecture
work.

## Relationship to GEISA

This project contains small educational demonstrations used to evaluate
Protocol Buffers behavior and tradeoffs relevant to GEISA API design.

The examples are historical and may not be maintained. They do not define
current GEISA requirements or constitute a conformance test.

## Project Information

- **Status:** Maintenance Limited
- **Maintainer:** Brandon Thayer (@blthayer), original author
- **License:** Apache-2.0
- **GEISA versions tested or supported:** Not tied to a specific GEISA release
- **Support:** Best-effort

## Demonstrations

### `backwards-compatibility`

Demonstrates Protocol Buffers compatibility behavior when a newer message
definition adds a field and an older consumer reads the serialized message.

See
[`backwards-compatibility/README.md`](backwards-compatibility/README.md)
for setup and usage instructions.

### `msgpack-vs-protobuf`

Provides a historical Python comparison of MessagePack and Protocol Buffers
serialized size and serialization performance using waveform-like sample data.

See
[`msgpack-vs-protobuf/README.md`](msgpack-vs-protobuf/README.md)
for the original results, methodology, setup, and usage instructions.

## Generated Files

The checked-in `*_pb2.py` files were generated from the accompanying `.proto`
files. They are retained so the demonstrations can be run without requiring a
local Protocol Buffers compiler.

## Known Limitations

- The performance comparison is a limited Python benchmark rather than a
  comprehensive cross-language or cross-platform evaluation
- Historical timing results depend on the hardware, operating system, Python
  version, package versions, and runtime environment used
- The examples use simplified messages and should not be treated as current
  GEISA API schemas
- Maintenance is limited; broad modernization or dependency upgrades are not
  implied by inclusion in the GEISA Community repository
