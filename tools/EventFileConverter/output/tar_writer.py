import tarfile
import io


class TarWriter:
    def __init__(self, file_path):
        self._file_path = file_path

    def __enter__(self):
        self._tar_file = tarfile.open(self._file_path, 'w').__enter__()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._tar_file.__exit__(exc_type, exc_val, exc_tb)

    def _write_file(self, file_name, file_bytes, file_time):
        info = tarfile.TarInfo(name=file_name)
        info.size = len(file_bytes)
        info.mtime = file_time
        data = io.BytesIO(file_bytes)
        self._tar_file.addfile(tarinfo=info, fileobj=data)

    def write_events(self, events):
        for i, event in enumerate(events):
            from output.json_writer import event_to_json
            json_string = event_to_json(event, indent=4)
            json_bytes = bytearray(json_string, encoding='utf-8')

            file_name = f"{(i // 10000):04}/{i:08}.e{event['type']}"
            self._write_file(file_name, json_bytes, event['context']['timestamp'])
