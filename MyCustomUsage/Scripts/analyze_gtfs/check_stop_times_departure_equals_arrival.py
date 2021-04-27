#!/usr/bin/env python3

from pathlib import Path
import sys
import csv
from typing import TextIO, Tuple
import textwrap


def usage_and_exit(return_code: int):
    print(
        textwrap.dedent(
            """
        Parse stop_times.txt to check if arrival_time == departure_time.
        INPUT stop_times.txt is not modified.
        Prints on stdout number of stop_times for which both times are equal (resp. different)
        Usage:         {prog}  INPUT_STOP_TIMES
        For instance : {prog}  stop_times.txt
    """.format(
                prog=sys.argv[0]
            )
        )
    )
    sys.exit(return_code)


def parse_args() -> Path:
    if len(sys.argv) != 2:
        usage_and_exit(1)

    # input stop_times
    stop_times_arg = sys.argv[1]
    try:
        in_stop_times = Path(stop_times_arg).expanduser().resolve()
    except FileNotFoundError:
        print("ERROR with input STOP_TIMES file : {}".format(stop_times_arg))
        usage_and_exit(2)
    return in_stop_times


def analyze(in_stop_times: TextIO) -> Tuple[int, int, int]:
    # SAMPLE :
    # trip_id                          , arrival_time , departure_time , stop_id , stop_sequence , pickup_type , drop_off_type , stop_headsign  # noqa: E501
    # 23358248-2020-TRAM_A1-Lun-Mer-45 , 05:06:26     , 05:06:26       , 7440    , 1             , 0           , 0             ,                # noqa: E501
    # 23358248-2020-TRAM_A1-Lun-Mer-45 , 05:08:26     , 05:08:26       , 7438    , 2             , 0           , 0             ,                # noqa: E501

    reader = csv.DictReader(in_stop_times)
    nb_stop_times = 0
    nb_stop_times_with_equal_fields = 0
    nb_stop_times_with_nonequal_fields = 0
    for row in reader:
        nb_stop_times += 1
        if row["arrival_time"] == row["departure_time"]:
            nb_stop_times_with_equal_fields += 1
        else:
            nb_stop_times_with_nonequal_fields += 1
    return nb_stop_times, nb_stop_times_with_equal_fields, nb_stop_times_with_nonequal_fields


def main():
    in_stop_times = parse_args()

    print("Analyzing INPUT stoptimes file :")
    print(in_stop_times)

    with in_stop_times.open("r") as f_in:
        total, equal, different = analyze(f_in)

    print("Total number of stop_times          = {}".format(total))
    print("stop_times where departure==arrival = {}".format(equal))
    print("stop_times where departure!=arrival = {}".format(different))
    assert total == equal + different


if __name__ == "__main__":
    main()
