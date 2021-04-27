#!/usr/bin/env python3

from pathlib import Path
from dataclasses import dataclass, field, fields, asdict
import sys
import csv
from typing import TextIO, Tuple, Set, Dict, Any
import textwrap


@dataclass(order=True, frozen=True)
class Trip:
    trip_id: str = field(compare=False)  # e.g. '23556941-2020-TRAM_A1-Lun-Mer-50'
    departure_time_from_src: str = field(compare=True)  # e.g. '17:28:49'
    arrival_time_at_dst: str = field(compare=False)  # e.g. '17:38:42'
    src_sequence_number: int = field(compare=False)  # e.g. 23
    dst_sequence_number: int = field(compare=False)  # e.g. 30


def usage_and_exit(return_code: int):
    print(
        textwrap.dedent(
            """
        Parse stop_times.txt, and list all trips joining a SOURCE stop to a DESTINATION stop
        INPUT stop_times.txt is not modified.
        Dumping joining trips in OUTPUT CSV file.
        Also printing for information the first and last joining trips on stdout.
        Usage:         {prog}  INPUT_STOP_TIMES  SRC_STOP  DST_STOP  OUTPUT_CSV
        For instance : {prog}  stop_times.txt    POLIC     ARLAC     /tmp/trips_from_POLIC_to_ARLAC.csv
    """.format(
                prog=sys.argv[0]
            )
        )
    )
    sys.exit(return_code)


def parse_args() -> Tuple[Path, str, str, Path]:
    if len(sys.argv) != 5:
        usage_and_exit(1)

    # input stop_times
    stop_times_arg = sys.argv[1]
    try:
        in_stop_times = Path(stop_times_arg).expanduser().resolve()
    except FileNotFoundError:
        print("ERROR with input STOP_TIMES file : {}".format(stop_times_arg))
        usage_and_exit(2)

    src_stop = sys.argv[2]
    dst_stop = sys.argv[3]

    # output trips csv :
    out_joining_trips_arg = sys.argv[4]
    out_joining_trips = Path(out_joining_trips_arg).expanduser()

    return in_stop_times, src_stop, dst_stop, out_joining_trips


def analyze(in_stop_times: TextIO, src_stop: str, dst_stop: str) -> Any:
    # SAMPLE :
    # trip_id                          , arrival_time , departure_time , stop_id , stop_sequence , pickup_type , drop_off_type , stop_headsign  # noqa: E501
    # 23358248-2020-TRAM_A1-Lun-Mer-45 , 05:06:26     , 05:06:26       , 7440    , 1             , 0           , 0             ,                # noqa: E501
    # 23358248-2020-TRAM_A1-Lun-Mer-45 , 05:08:26     , 05:08:26       , 7438    , 2             , 0           , 0             ,                # noqa: E501

    trips_passing_by_src: Set[str] = set()
    trips_passing_by_dst: Set[str] = set()

    RowType = Dict[str, str]
    maybe_interesting_lines: Dict[str, RowType] = {}

    reader = csv.DictReader(in_stop_times)
    for row in reader:
        if row["stop_id"] == src_stop:
            trips_passing_by_src.add(row["trip_id"])
            maybe_interesting_lines[row["trip_id"] + "_SRC"] = row
        if row["stop_id"] == dst_stop:
            trips_passing_by_dst.add(row["trip_id"])
            maybe_interesting_lines[row["trip_id"] + "_DST"] = row

    # quels sont les trips qui passent par les deux stops ?
    trips_joining_both_stop = trips_passing_by_src.intersection(trips_passing_by_dst)

    # parmi ceux-ci, quels sont les trips qui passent par dst APRÈS src ?
    proper_order_trips = []
    for trip_id in trips_joining_both_stop:
        src_row = maybe_interesting_lines[trip_id + "_SRC"]
        dst_row = maybe_interesting_lines[trip_id + "_DST"]
        assert src_row["trip_id"] == dst_row["trip_id"] == trip_id
        if src_row["departure_time"] < dst_row["arrival_time"]:
            proper_order_trips.append(
                Trip(
                    trip_id=trip_id,
                    departure_time_from_src=src_row["departure_time"],
                    arrival_time_at_dst=dst_row["arrival_time"],
                    src_sequence_number=int(src_row["stop_sequence"]),
                    dst_sequence_number=int(dst_row["stop_sequence"]),
                )
            )

    return proper_order_trips


def main():
    in_stop_times, src_stop, dst_stop, out_joining_trips = parse_args()

    print("Analyzing INPUT stoptimes file :")
    print(in_stop_times)
    print("Listing trips joining these stops :")
    print("{} ->  {}".format(src_stop, dst_stop))
    print("Dumping trips joining SRC to DST in OUTPUT file :")
    print(out_joining_trips)

    with in_stop_times.open("r") as f_in:
        trips = analyze(f_in, src_stop, dst_stop)

    print("There are {} trips joining SRC to DST".format(len(trips)))
    trips.sort()

    # dumping trips in CSV file :
    fieldnames = [field.name for field in fields(Trip)]
    with out_joining_trips.open("w") as f_out:
        writer = csv.DictWriter(f_out, fieldnames, dialect="unix", quoting=csv.QUOTE_MINIMAL)
        writer.writeheader()
        for trip in trips:
            writer.writerow(asdict(trip))

    print("First ones are :")
    for trip in trips[0:5]:
        print("\t{}".format(trip))
    print("Last ones are :")
    for trip in trips[-5:]:
        print("\t{}".format(trip))


if __name__ == "__main__":
    main()
