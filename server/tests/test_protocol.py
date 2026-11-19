import numpy as np

from cozmo_server.protocol import (
    HEADER_SIZE,
    MIC_MAGIC,
    SPEAKER_MAGIC,
    beacon_message,
    pack_audio,
    unpack_audio,
)


def test_header_matches_firmware_struct():
    # firmware/src/net/Protocol.h static_asserts sizeof(AudioHeader) == 12
    assert HEADER_SIZE == 12


def test_roundtrip():
    samples = np.arange(-160, 160, dtype=np.int16)
    data = pack_audio(MIC_MAGIC, 42, samples, flags=3)
    assert len(data) == 12 + 2 * len(samples)
    packet = unpack_audio(data)
    assert packet is not None
    assert packet.magic == MIC_MAGIC
    assert packet.seq == 42
    assert packet.flags == 3
    np.testing.assert_array_equal(packet.samples, samples)


def test_little_endian_layout():
    data = pack_audio(SPEAKER_MAGIC, 1, np.array([1], dtype=np.int16))
    assert data[:4] == b"CZSP"
    assert data[4:8] == b"\x01\x00\x00\x00"
    assert data[8:10] == b"\x01\x00"
    assert data[12:14] == b"\x01\x00"


def test_rejects_garbage_and_truncation():
    assert unpack_audio(b"xx") is None
    assert unpack_audio(b"ABCD" + bytes(20)) is None
    data = pack_audio(MIC_MAGIC, 0, np.zeros(10, dtype=np.int16))
    assert len(unpack_audio(data[:-4]).samples) == 8


def test_beacon():
    assert beacon_message(5008) == b"COZMO_SERVER 5008\n"
