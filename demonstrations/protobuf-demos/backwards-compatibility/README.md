# backward-compatibility

This demonstration illustrates what happens when a `protobuf` message definition
changes. See the `protobuf` [documentation](https://protobuf.dev/best-practices/)
around best practices for more information on how to design messages with backwards
compatibility in mind.

Start by taking a look at the files `v1.proto` and `v2.proto`. Note that they are
nearly identical, but `v2.proto` has one extra, new field: `new_field`.

The idea here is to illustrate what happens if a program using the older message
definition (from `v1.proto`) receives/works with a newer message definition
(from `v2.proto`). Demonstration:

```bash console
$ python --version
Python 3.12.8
$ python -m venv venv
$ ./venv/bin/pip install -r requirements.txt
...
$ ./venv/bin/python v2.py
$ ./venv/bin/python v1.py
Is the sequence_number field present? False
Value of the sequence_number if extracted blindly: 0
timestamp: 42
After loading a 'new' (v2) message with an 'old' (v1) message schema, we don't know about the 'new_field'
```

If you take a look at `v2.py`, you'll notice that it serializes a `v2.proto` message to disk,
and notably omits the `sequence_number` field and includes the `new_field`.

`v1.py` then reads the serialized bytes and illustrates what it looks like to deal with the
missing `sequence_number`. It also proves that despite using the "old" message definition from
`v1.proto` (via `v1_pb2.py`), it's still able to decode the new message just fine. Unsurprisingly,
it is not able to access/use the `new_field` field because it knows nothing about it.

## Compiling `.proto` files

For reference, the files `v1_pb2.py` and `v2_pb2.py` were generated with `protoc`:

```bash console
$ protoc --version
libprotoc 31.1
$ protoc --proto_path=. --python_out=. ./v1.proto
$ protoc --proto_path=. --python_out=. ./v2.proto
```
