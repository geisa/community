# msgpack-vs-protobuf
Compare [MessagePack](https://msgpack.org/) (`msgpack`) and [Protocol Buffers](https://protobuf.dev/)
(`protobuf`) for [GEISA](https://lfenergy.org/projects/geisa/).

## Bottom Line Up Front

```bash console
$ ./venv/bin/python main.py
Waveform data shape: (960000, 2)
Length of data in protobuf repeated float: 1920000
Size of serialized protobuf message in kibibytes: 7500.03
Protobuf serialization (1000 trials) took 0.69 seconds.
Size of serialized MessagePack object in kibibytes: 16875.09
MessagePack C-extension is installed.
MessagePack serialization (1000 trials) took 27.06 seconds.
```

While this benchmarking script is not comprehensive and does not
cover languages outside of Python (although keep in mind C extensions
are in play for performance reasons), it would appear that for a
somewhat realistic message that `protobuf` results in a more compact
serialized payload and is significantly more performant during
serialization when compared to `msgpack`.

Additionally, it's worth noting that `protobuf` forces the use of a
strongly-typed schema for both the sending and receiving ends, which
reduces integration errors from sender/receiver schema mismatches.
`msgpack` does not use a schema and results in dynamically typed data
structures.

## Installation/Usage

At the time of writing (2025-06-23), this was only tested with Python 3.12,
although it should work for older (and newer) versions of Python as well.

```bash console
$ cd msgpack-vs-protobuf
$ python -m venv venv
$ ./venv/bin/pip install -r requirements.txt
...
$ ./venv/bin/python main.py
```

## Compiling `protobuf` messages

This can be done "offline" - the resulting Python file (`waveform_pb2.py`) is
included with the repository.

Assuming the present working directory is this repository:

```bash console
$ protoc --version
libprotoc 31.1
$ protoc --proto_path=. --python_out=. ./waveform.proto
```
