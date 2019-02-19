import json


def event_to_json(event, indent=None):
    return json.dumps(event, sort_keys=False, indent=indent)


class JsonWriter:
    def __init__(self, file_path):
        self._file_path = file_path

    def __enter__(self):
        self._file_obj = open(self._file_path, 'w').__enter__()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._file_obj.__exit__(exc_type, exc_val, exc_tb)

    def write_events(self, events):
        for event in events:
            self._file_obj.write(event_to_json(event) + '\n')
