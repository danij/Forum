import mmap
import struct
import zlib

from input import context_reader, data_reader
from data import event_format


def _decode_event(event_bytes):
    event_type = struct.unpack('<I', event_bytes[0:4])[0]
    event_version = struct.unpack('<H', event_bytes[4:6])[0]
    context_version = struct.unpack('<H', event_bytes[6:8])[0]

    if 0 == event_type:
        return None

    offset = 8
    (context, context_size) = context_reader.read_context(context_version, event_bytes, offset)
    offset += context_size
    data = data_reader.read_data(event_type, event_version, event_bytes, offset)

    return {
        'type': event_type,
        'type_name': event_format.get_event_type_name(event_type),
        'version': event_version,
        'context_version': context_version,
        'context': context,
        'data': data
    }


class EventsReader:
    MAGIC_PREFIX = b'\xff\xff\xff\xff\xff\xff\xff\xff'
    EVENT_LENGTH_SIZE = 4
    EVENT_CHECKSUM_SIZE = 4
    MIN_EVENT_SIZE = len(MAGIC_PREFIX) + EVENT_LENGTH_SIZE + EVENT_CHECKSUM_SIZE
    EVENT_ALIGNMENT = 8

    def __init__(self, file_path):
        self._file_path = file_path

    def __enter__(self):
        self._file_obj = open(self._file_path, 'rb').__enter__()
        self._mmap = mmap.mmap(self._file_obj.fileno(), 0, access=mmap.ACCESS_READ)
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._mmap.close()
        self._file_obj.__exit__(exc_type, exc_val, exc_tb)

    def _padding_required(self, size):
        size_multiple = (size // self.EVENT_ALIGNMENT) * self.EVENT_ALIGNMENT
        if size_multiple < size:
            return self.EVENT_ALIGNMENT - (size - size_multiple)
        return 0

    def read_events(self):
        file_size = self._mmap.size()

        def error(message):
            raise ValueError(f'[Offset {self._mmap.tell()}] {message}')

        while self._mmap.tell() < file_size:
            remaining_bytes = file_size - self._mmap.tell()
            if file_size - self._mmap.tell() < self.MIN_EVENT_SIZE:
                error(f'Event size {remaining_bytes} is less than the minimum event size')

            magic_prefix_bytes = self._mmap.read(len(self.MAGIC_PREFIX))
            if magic_prefix_bytes != self.MAGIC_PREFIX:
                error('Invalid magic prefix ' + str(magic_prefix_bytes))

            (event_length, expected_checksum) = struct.unpack('<II', self._mmap.read(
                self.EVENT_LENGTH_SIZE + self.EVENT_CHECKSUM_SIZE))

            event_bytes = self._mmap.read(event_length)
            if len(event_bytes) < event_length:
                error(f'Not enough bytes remaining for an event of size {event_length}')

            if expected_checksum != 0:
                actual_checksum = zlib.crc32(event_bytes)
                if actual_checksum != expected_checksum:
                    error(f'Checksum mismatch: actual={actual_checksum}, expected={expected_checksum}')

            padding_required = self._padding_required(event_length)
            if padding_required > 0:
                self._mmap.read(padding_required)

            decoded_event = _decode_event(event_bytes)
            if decoded_event:
                yield decoded_event
