import v1_pb2


def main():
    pb = v1_pb2.WaveformData()
    with open("v2.proto.bin", "rb") as f:
        pb.ParseFromString(f.read())

    print(
        f"Is the sequence_number field present? {pb.HasField('sequence_number')}"
    )
    print(
        f"Value of the sequence_number if extracted blindly: {pb.sequence_number}"
    )
    print(f"timestamp: {pb.timestamp}")
    try:
        print(pb.new_field)
    except AttributeError:
        print(
            "After loading a 'new' (v2) message with an 'old' (v1) message schema, "
            "we don't know about the 'new_field'"
        )

    # TODO: Why does this raise a NotImplementedError?
    # I guess the obvious answer is they haven't implemented it yet...
    # print(pb.UnknownFields())


if __name__ == "__main__":
    main()
