import struct
import uuid
import re

from data import event_format

_event_parser_info = event_format.get_event_parser_info()


def _read_uuid(_, data_bytes, offset):
    return str(uuid.UUID(bytes=data_bytes[offset:offset + 16])), 16


def _read_string(_, data_bytes, offset):
    length = struct.unpack('<I', data_bytes[offset:offset + 4])[0]
    result = data_bytes[offset + 4:offset + 4 + length].decode('utf-8')
    return result, 4 + length


def _read_type(extra, data_bytes, offset):
    (struct_format, length) = extra
    result = struct.unpack(struct_format, data_bytes[offset:offset + length])[0]
    return result, length


_readers = {
    'READ_UUID': _read_uuid,
    'READ_STRING': _read_string,
    'READ_NONEMPTY_STRING': _read_string,
    'READ_TYPE': _read_type
}


def _read_value(parameter_type, extra, data_bytes, offset):
    if parameter_type in _readers:
        return _readers[parameter_type](extra, data_bytes, offset)

    raise ValueError(f'Unknown data reader {parameter_type}')


_uppercase_regex = re.compile('[A-Z]')


def _get_property_name(parameter_name):
    return _uppercase_regex.sub(lambda match: '_' + match[0].lower(), parameter_name).strip('_')


def read_data(event_type, event_version, data_bytes, offset):
    key = (event_type, event_version)
    if key not in _event_parser_info:
        raise ValueError(f'Unknown data reader for {key}')

    result = {}
    for (parameter_type, parameter_name, extra) in _event_parser_info[key]:
        (value, size) = _read_value(parameter_type, extra, data_bytes, offset)
        result[_get_property_name(parameter_name)] = value
        offset += size

    return result
