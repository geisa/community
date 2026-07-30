import v2_pb2


def main():
    pb = v2_pb2.WaveformData()
    # Intentionally omit the sequence number.
    # pb.sequence_number = 12
    pb.timestamp = 42
    pb.sample_rate = 16_000
    pb.sample_count = 128
    pb.channel_count = 2
    pb.data.extend([12.4, 86.3, 19.8])
    pb.new_field = 18

    with open("v2.proto.bin", "wb") as f:
        f.write(pb.SerializeToString())


if __name__ == "__main__":
    main()
