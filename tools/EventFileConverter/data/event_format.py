import os
import re

_source_folder = os.path.abspath(os.path.join(os.path.dirname(os.path.realpath(__file__)),
                                              '..', '..', '..',
                                              'src', 'LibForumPersistence'))


def _get_file_path(*args):
    return os.path.abspath(os.path.join(_source_folder, *args))


def _load_event_type_names():
    result = {}
    state = 0
    with open(_get_file_path('PersistenceFormat.h')) as r:
        for line in r:
            line = line.rstrip()
            if 0 == state:
                if 'enum EventType' in line:
                    state = 1
            elif 1 == state:
                if '}' in line:
                    state = 0
                else:
                    name = line.strip(" \t,0={")
                    if name:
                        result[len(result)] = name
    return result


_event_types = _load_event_type_names()

_primitive_unpack_values = {
    'int16_t': ('<h', 2),
    'uint16_t': ('<H', 2),
    'int32_t': ('<i', 4),
    'uint32_t': ('<I', 4),
    'int64_t': ('<q', 8),
    'uint64_t': ('<Q', 8),
    'PersistentPrivilegeEnumType': ('<H', 2),
    'PersistentPrivilegeValueType': ('<h', 2),
    'PersistentPrivilegeDurationType': ('<q', 8)
}


def _get_parser_action(name, params):
    if 'READ_TYPE' == name:
        return name, params[1], _primitive_unpack_values[params[0]]
    return name, params[0], None


def _load_event_data_parser_info():
    result = {}
    begin_importer = re.compile(r'.*BEGIN_DEFAULT_IMPORTER\s*\(\s*([A-Z_]+),\s*(\d+)\s*\).*')
    read_action = re.compile(r'.*(READ_[A-Z_]+)\s*\(([^)]+)\).*')
    state = 0
    event_type = 0
    event_version = 0
    event_actions = []

    with open(_get_file_path('private', 'EventImporter.cpp')) as r:
        for line in r:
            line = line.rstrip()
            if 0 == state:
                match = begin_importer.match(line)
                if match:
                    event_type_name = match[1]
                    event_type = [k for k, v in _event_types.items() if v == event_type_name][0]
                    event_version = int(match[2])
                    event_actions = []
                    state = 1
            elif 1 == state:
                if 'CHECK_READ_ALL_DATA' in line:
                    result[(event_type, event_version)] = event_actions
                    state = 0
                else:
                    match = read_action.match(line)
                    if match:
                        event_actions.append(_get_parser_action(match[1], [v.strip() for v in match[2].split(',')]))

    return result


_event_parser_info = _load_event_data_parser_info()


def get_event_type_name(event_type_int):
    if event_type_int in _event_types:
        return _event_types[event_type_int]
    return 'Unknown'


def get_event_parser_info():
    return _event_parser_info
