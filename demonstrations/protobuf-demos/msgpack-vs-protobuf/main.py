from dataclasses import asdict, dataclass
from time import time_ns
from timeit import timeit

import msgpack  # type: ignore[import-untyped]

# Determine if C-extension for msgpack is installed.
# https://github.com/msgpack/msgpack-python/blob/v1.1.1/benchmark/benchmark.py
try:
    from msgpack import _cmsgpack

    has_ext = True
except ImportError:
    from msgpack import fallback

    has_ext = False

import numpy as np

import waveform_pb2

SEQUENCE_NUMBER = 11_512
TIMESTAMP = time_ns()
SAMPLE_RATE = 16_000
SAMPLE_COUNT = 16_000 * 60
CHANNEL_COUNT = 2

# How many times to serialize objects for timing.
SERIALIZE_COUNT = 1_000


@dataclass
class WaveformData:
    data: list[float]
    sequence_number: int = SEQUENCE_NUMBER
    timestamp: int = TIMESTAMP
    sample_rate: int = SAMPLE_RATE
    sample_count: int = SAMPLE_COUNT
    channel_count: int = CHANNEL_COUNT


def main():
    rng = np.random.default_rng()
    noise = rng.standard_normal((SAMPLE_COUNT, CHANNEL_COUNT))
    time = np.arange(SAMPLE_COUNT).reshape((SAMPLE_COUNT, 1)) / SAMPLE_RATE
    # Sorta voltage-like
    wave = (
        120 * np.sin(2 * np.pi * 60 * np.repeat(time, CHANNEL_COUNT, axis=1))
        + noise
    )

    print(f"Waveform data shape: {wave.shape}")
    # Consumer of data is expected to grab CHANNEL_COUNT at a time
    # values.
    wave_list = wave.flatten().tolist()

    # pb: protobuf
    pb = waveform_pb2.WaveformData()
    pb.sequence_number = SEQUENCE_NUMBER
    pb.timestamp = TIMESTAMP
    pb.sample_rate = SAMPLE_RATE
    pb.sample_count = SAMPLE_COUNT
    pb.channel_count = CHANNEL_COUNT
    pb.data.extend(wave_list)
    # print(f"Length of data in protobuf repeated float: {len(pb.data)}")
    print(
        "Size of serialized protobuf message in kibibytes: "
        f"{pb.ByteSize() / 1024:.2f}"
    )

    # Setup code for serialization test. Doing this in case there is some
    # caching that happens under the hood with protobuf if we just
    # serialized the same object every time.
    setup = """
pb = waveform_pb2.WaveformData()
pb.sequence_number = SEQUENCE_NUMBER
pb.timestamp = TIMESTAMP
pb.sample_rate = SAMPLE_RATE
pb.sample_count = SAMPLE_COUNT
pb.channel_count = CHANNEL_COUNT
pb.data.extend(wave_list)
    """
    t_pb = timeit(
        "pb.SerializeToString()",
        number=SERIALIZE_COUNT,
        setup=setup,
        globals=globals() | {"wave_list": wave_list},
    )
    print(
        f"Protobuf serialization ({SERIALIZE_COUNT} trials) took "
        f"{t_pb:.2f} seconds."
    )

    # dc: dataclass
    dc = WaveformData(data=wave_list)
    dc_dict = asdict(dc)

    # TODO: Why do we get a wildly different serialization size here
    # than when we use the Packer instance later?
    mp_bytes = msgpack.packb(dc_dict)
    print(
        "Size of serialized MessagePack object in kibibytes: "
        f"{len(mp_bytes) / 1024:.2f}"
    )
    # Initialize Packer instance for MessagePack.
    #
    if has_ext:
        print("MessagePack C-extension is installed.")
        packer = _cmsgpack.Packer(
            use_single_float=True, buf_size=len(mp_bytes)
        )
    else:
        print("MessagePack does NOT have its C-extenstion installed.")
        packer = fallback.Packer(use_single_float=True, buf_size=len(mp_bytes))

    t_mp = timeit(
        "packer.pack(dc_dict)",
        number=SERIALIZE_COUNT,
        globals={"dc_dict": dc_dict, "packer": packer},
    )
    print(
        f"MessagePack serialization ({SERIALIZE_COUNT} trials) took "
        f"{t_mp:.2f} seconds."
    )


if __name__ == "__main__":
    main()
