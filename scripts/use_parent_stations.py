#!/usr/bin/env python3

from pathlib import Path
import sys
import csv
from typing import TextIO, Dict, Tuple, cast, Sequence
import textwrap

Stop2Parent = Dict[str, str]


def usage_and_exit(return_code: int):
    print(
        textwrap.dedent(
            """
        Edit input stops/stop_times files to replace stations by their parent.
        OUTPUT stops.txt only contains parent stations from INPUT stops.txt
        OUTPUT stop_times.txt is identical to INPUT stop_times.txt, but with stop_ids replace by parent's station id.
        Usage:         {prog}  INPUT_STOPS  INPUT_STOP_TIMES  OUTPUT_STOPS      OUTPUT_STOP_TIMES
        For instance : {prog}  stops.txt    stop_times.txt    PARENT_stops.txt  PARENT_stop_times.txt
    """.format(
                prog=sys.argv[0]
            )
        )
    )
    sys.exit(return_code)


def parse_args() -> Tuple[Path, Path, Path, Path]:
    if len(sys.argv) != 5:
        usage_and_exit(1)

    # input stops
    stops_arg = sys.argv[1]
    try:
        in_stops = Path(stops_arg).expanduser().resolve()
    except FileNotFoundError:
        print("ERROR with input STOPS file : {}".format(stops_arg))
        usage_and_exit(2)

    # input stop_times
    stop_times_arg = sys.argv[2]
    try:
        in_stop_times = Path(stop_times_arg).expanduser().resolve()
    except FileNotFoundError:
        print("ERROR with input STOP_TIMES file : {}".format(stop_times_arg))
        usage_and_exit(3)

    # output stops
    out_stops_arg = sys.argv[3]
    out_stops = Path(out_stops_arg).expanduser()

    # output stop_times
    out_stop_times_arg = sys.argv[4]
    out_stop_times = Path(out_stop_times_arg).expanduser()

    return in_stops, in_stop_times, out_stops, out_stop_times


def step1_use_parents_in_stops(in_stops: TextIO, out_stops: TextIO) -> Stop2Parent:
    # stop_id , stop_name   , stop_lat  , stop_lon  , stop_desc , zone_id , stop_url , stop_code , location_type , parent_station  # noqa: E501
    # 3684    , Lauriers    , 44.879107 , -0.517867 ,           , 33249   ,          , LAURI     , 0             , LAURI           # noqa: E501
    # 3685    , Bois Fleuri , 44.875987 , -0.519344 ,           , 33249   ,          , BFLEA     , 0             , BFLEU           # noqa: E501

    stop2parent = dict()

    reader = csv.DictReader(in_stops)
    fieldnames = cast(Sequence[str], reader.fieldnames)
    writer = csv.DictWriter(out_stops, fieldnames, dialect="unix", quoting=csv.QUOTE_MINIMAL)
    writer.writeheader()

    for row in reader:
        stop_id = row["stop_id"]
        parsed_parent_station = row["parent_station"]

        # parent stations are the ones that have the "parent_stations" field empty
        is_parent_station = parsed_parent_station == ""

        # a parent station is its own parent, otherwise, use the "real" parent station :
        parent = stop_id if is_parent_station else parsed_parent_station
        stop2parent[stop_id] = parent

        # only dumping parent stations :
        if is_parent_station:
            writer.writerow(row)

    return stop2parent


def step2_use_parents_in_stop_times(in_stop_times: TextIO, out_stop_times: TextIO, stop2parent: Stop2Parent) -> None:
    # trip_id                          , arrival_time , departure_time , stop_id , stop_sequence , pickup_type , drop_off_type , stop_headsign  # noqa: E501
    # 23358248-2020-TRAM_A1-Lun-Mer-45 , 05:06:26     , 05:06:26       , 7440    , 1             , 0           , 0             ,                # noqa: E501
    # 23358248-2020-TRAM_A1-Lun-Mer-45 , 05:08:26     , 05:08:26       , 7438    , 2             , 0           , 0             ,                # noqa: E501

    reader = csv.DictReader(in_stop_times)
    fieldnames = cast(Sequence[str], reader.fieldnames)
    writer = csv.DictWriter(out_stop_times, fieldnames, dialect="unix", quoting=csv.QUOTE_MINIMAL)
    writer.writeheader()

    for row in reader:
        initial_stop_id = row["stop_id"]
        row["stop_id"] = stop2parent[initial_stop_id]
        writer.writerow(row)


def main():
    in_stops, in_stop_times, out_stops, out_stop_times = parse_args()

    print("Using parent stations from INPUT files :")
    print(in_stops)
    print(in_stop_times)
    print("Into OUTPUT files :")
    print(out_stops)
    print(out_stop_times)

    with in_stops.open("r") as f_in, out_stops.open("w") as f_out:
        stop2parent = step1_use_parents_in_stops(f_in, f_out)
    with in_stop_times.open("r") as f_in, out_stop_times.open("w") as f_out:
        step2_use_parents_in_stop_times(f_in, f_out, stop2parent)


if __name__ == "__main__":
    main()
