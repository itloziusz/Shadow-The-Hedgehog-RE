"""Adversarial packet/data checks without launching a reference emulator."""

import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from capture_dolphin_rsp import RSP


class Socket:
    def __init__(self, data=b""):
        self.data = bytearray(data)
        self.sent = []

    def settimeout(self, value): pass
    def recv(self, count):
        result = bytes(self.data[:count]); del self.data[:count]; return result
    def sendall(self, data): self.sent.append(data)


class PacketTests(unittest.TestCase):
    def test_e_prefixed_instruction_is_data(self):
        rsp = RSP(Socket())
        rsp.send = lambda payload: "E0030000"
        self.assertEqual(rsp.memory(0x80370CFC, 4), bytes.fromhex("E0030000"))

    def test_actual_error_and_short_data_decline(self):
        for response in ("E01", "0000"):
            rsp = RSP(Socket()); rsp.send = lambda payload: response
            with self.assertRaises(ValueError): rsp.memory(0x80370CFC, 4)

    def test_eof_before_and_inside_packet_declines(self):
        for data in (b"", b"+", b"$abc"):
            with self.assertRaises(EOFError): RSP(Socket(data)).packet()

    def test_checksum_and_ack(self):
        sock = Socket(b"+$OK#9a")
        self.assertEqual(RSP(sock).packet(), "OK")
        self.assertEqual(sock.sent, [b"+"])
        bad = Socket(b"$OK#00")
        with self.assertRaises(ValueError): RSP(bad).packet()
        self.assertEqual(bad.sent, [b"-"])

    def test_fragmented_checksum_and_missing_second_digit(self):
        class Fragmented(Socket):
            def recv(self, count): return super().recv(min(count, 1))
        self.assertEqual(RSP(Fragmented(b"$OK#9a")).packet(), "OK")
        with self.assertRaises(EOFError): RSP(Fragmented(b"$OK#9")).packet()


if __name__ == "__main__": unittest.main()
