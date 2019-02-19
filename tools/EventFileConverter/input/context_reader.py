import struct
import uuid
import ipaddress
import datetime


def _read_ip_address(address_bytes):
    ints = list(struct.unpack('>IIII', address_bytes))
    if 0 == (ints[1] | ints[2] | ints[3]):
        return ipaddress.IPv4Address(ints[0])
    else:
        return ipaddress.IPv6Address(address_bytes)


def _read_context_version_1(context_bytes, offset):
    context_size = 8 + 16 + 16

    timestamp = struct.unpack('<q', context_bytes[offset:offset + 8])[0]
    result = {
        'timestamp': timestamp,
        'timestamp_str': datetime.datetime.utcfromtimestamp(timestamp).strftime('%Y-%m-%d %H:%M:%SZ'),
        'user_id': str(uuid.UUID(bytes=context_bytes[offset + 8:offset + 24])),
        'ip_address': str(_read_ip_address(context_bytes[offset + 24:offset + 40]))
    }
    return result, context_size


context_readers = {
    1: _read_context_version_1
}


def read_context(context_version, context_bytes, offset):
    if context_version in context_readers:
        return context_readers[context_version](context_bytes, offset)

    raise ValueError(f'Unknown context version {context_version}')
