#!/usr/bin/env python3

from pathlib import Path
import sys
import csv
from typing import TextIO, Tuple, Iterable, cast
import textwrap


def usage_and_exit(return_code: int):
    print(
        textwrap.dedent(
            """
        From an INPUT transfers.txt, only keep transfers to/from an existing stop (i.e. appearing in INPUT stops.txt)

        OUTPUT transfers.txt is identical to INPUT transfers.txt, but without the transfers to/from an inexisting stop.

        Usage:         {prog}  INPUT_TRANSFERS  INPUT_STOPS  OUTPUT_TRANSFERS
        For instance : {prog}  transfers.txt    stops.txt    VALID_transfers.txt
    """.format(
                prog=sys.argv[0]
            )
        )
    )
    sys.exit(return_code)


def parse_args() -> Tuple[Path, Path, Path]:
    if len(sys.argv) != 4:
        usage_and_exit(1)

    # input transfers
    transfers_arg = sys.argv[1]
    try:
        in_transfers = Path(transfers_arg).expanduser().resolve()
    except FileNotFoundError:
        print("ERROR with INPUT transfers file : {}".format(transfers_arg))
        usage_and_exit(2)

    # input stops
    stops_arg = sys.argv[2]
    try:
        in_stops = Path(stops_arg).expanduser().resolve()
    except FileNotFoundError:
        print("ERROR with INPUT stops file : {}".format(stops_arg))
        usage_and_exit(3)

    # output stops
    out_transfers_arg = sys.argv[3]
    out_transfers = Path(out_transfers_arg).expanduser()
    return in_transfers, in_stops, out_transfers


def remove_invalid_transfers(in_io: TextIO, stops_io: TextIO, out_io: TextIO) -> Tuple[int, int]:
    """
    postcondition: out_io only contains transfers between known stops
    """
    # stop_id , stop_name   , stop_lat  , stop_lon  , stop_desc , zone_id , stop_url , stop_code , location_type , parent_station  # noqa: E501
    # 3684    , Lauriers    , 44.879107 , -0.517867 ,           , 33249   ,          , LAURI     , 0             , LAURI           # noqa: E501
    # 3685    , Bois Fleuri , 44.875987 , -0.519344 ,           , 33249   ,          , BFLEA     , 0             , BFLEU           # noqa: E501

    known_stops = set(row["stop_id"] for row in csv.DictReader(stops_io))
    print("Number of known stops = {}".format(len(known_stops)))

    transfers_reader = csv.DictReader(in_io)
    fieldnames = cast(Iterable[str], transfers_reader.fieldnames)
    writer = csv.DictWriter(out_io, fieldnames, dialect="unix", quoting=csv.QUOTE_MINIMAL)
    writer.writeheader()
    nb_transfers_total = 0
    nb_transfers_kept = 0
    for transfer_row in transfers_reader:
        from_stop = transfer_row["from_stop_id"]
        to_stop = transfer_row["to_stop_id"]
        nb_transfers_total += 1
        if from_stop in known_stops and to_stop in known_stops:
            writer.writerow(transfer_row)
            nb_transfers_kept += 1
    nb_transfers_removed = nb_transfers_total - nb_transfers_kept
    return nb_transfers_removed, nb_transfers_total


def main():
    in_transfers, in_stops, out_transfers = parse_args()

    print("Filtering INPUT transfers file :")
    print(in_transfers)
    print("With INPUT stops file :")
    print(in_stops)
    print("Into OUTPUT transfers file :")
    print(out_transfers)

    with in_transfers.open("r") as f_in, in_stops.open("r") as f_stops, out_transfers.open("w") as f_out:
        nb_removed, nb_total = remove_invalid_transfers(f_in, f_stops, f_out)
    print("On {} input transfers in total, {} were removed because invalid".format(nb_total, nb_removed))


if __name__ == "__main__":
    main()
