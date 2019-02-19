import argparse

from input.events_reader import EventsReader
from output.json_writer import JsonWriter
from output.tar_writer import TarWriter

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert FastForum persisted events')
    parser.add_argument('-i', required=True, help='input file name')
    parser.add_argument('-o', required=True, help='output file name (.tar or .json)')

    args = parser.parse_args()

    writer_type = TarWriter if args.o.lower().endswith('.tar') else JsonWriter

    with EventsReader(args.i) as r:
        with writer_type(args.o) as w:
            w.write_events(r.read_events())
