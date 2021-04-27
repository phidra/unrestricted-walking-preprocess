#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

this_script_parent="$(realpath "$(dirname "$0")" )"

# === Preparing build
BUILD_DIR="$this_script_parent/_build"
CMAKE_ROOT_DIR="$this_script_parent/src"
SCRIPTS_DIR="$this_script_parent/scripts"
DATA_DIR="$this_script_parent/data"
WALKSPEED_KMH=4.7
echo "BUILD_DIR=$BUILD_DIR"
echo "CMAKE_ROOT_DIR=$CMAKE_ROOT_DIR"

echo "To build from scratch :  rm -rf '$BUILD_DIR'"
# rm -rf "$BUILD_DIR"


# === Preparing WORKDIR :
WORKDIR="${this_script_parent}/PREPARED_DATA_BORDEAUX"
echo "Using WORKDIR = $WORKDIR"
[ -e "$WORKDIR" ] && echo "ERROR : already existing workdir :  $WORKDIR" && exit 1
INPUT_POLYGON_FILE="$WORKDIR/INPUT/bordeaux_polygon.geojson"
INPUT_OSM_FILE="$WORKDIR/INPUT/aquitaine-latest.osm.pbf"
INPUT_GTFS_DATA="$WORKDIR/INPUT/gtfs"
mkdir -p "$WORKDIR/INPUT"
mkdir -p "$INPUT_GTFS_DATA"


# === Building :
mkdir -p "$BUILD_DIR"
conan install --install-folder="$BUILD_DIR" "$CMAKE_ROOT_DIR" --profile="$CMAKE_ROOT_DIR/conanprofile.txt"
CXX=$(which clang++) cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo  -B"$BUILD_DIR" -H"$CMAKE_ROOT_DIR"
make -j -C "$BUILD_DIR" download_osm_bordeaux build-preparatory


# === Putting GTFS data in WORKDIR :
# There are two possible sources of data for bordeaux GTFS :
GTFS_DATA_TO_USE="bundled"
if [ "$GTFS_DATA_TO_USE" == "bundled" ]
then
    # 1. use the data bundled with the code (less fresh, but simplify comparison with other intermodal engine like HL-UW) :
    BUNDLED_GTFS_DATA="$DATA_DIR/bordeaux_gtfs/bordeaux_gtfs.tar.7z"
    echo "Using Bordeaux GTFS data bundled with the code : $BUNDLED_GTFS_DATA"
    "$DATA_DIR/bordeaux_gtfs/extract_7z.sh" "$BUNDLED_GTFS_DATA" "$INPUT_GTFS_DATA"
    mv "$INPUT_GTFS_DATA/bordeaux"/*.txt "$INPUT_GTFS_DATA"
    rmdir "$INPUT_GTFS_DATA/bordeaux"
else
    # 2. use downloaded GTFS data (more fresh, but possibly different from the one use in HL-UW) :
    make -j -C "$BUILD_DIR" download_gtfs_bordeaux
    echo "Using downloaded Bordeaux GTFS data"
    cp -R "${this_script_parent}/DOWNLOADED_DATA/gtfs_bordeaux/"* "$INPUT_GTFS_DATA"
fi


# === Putting other data in WORKDIR :
cp "${DATA_DIR}/bordeaux_polygon.geojson" "$INPUT_POLYGON_FILE"
cp "${this_script_parent}/DOWNLOADED_DATA/osm_bordeaux/aquitaine-latest.osm.pbf" "$INPUT_OSM_FILE"
echo "Using data from WORKDIR = $WORKDIR"
echo ""

# === Preprocessing GTFS data to use parent stations :
mv "$INPUT_GTFS_DATA/stops.txt" "$INPUT_GTFS_DATA/original_stops.txt"
mv "$INPUT_GTFS_DATA/stop_times.txt" "$INPUT_GTFS_DATA/original_stop_times.txt"
"${SCRIPTS_DIR}/use_parent_stations.py" \
    "$INPUT_GTFS_DATA/original_stops.txt" \
    "$INPUT_GTFS_DATA/original_stop_times.txt" \
    "$INPUT_GTFS_DATA/stops.txt" \
    "$INPUT_GTFS_DATA/stop_times.txt"

# === building preparatory data :
OUTPUT_DIR="$WORKDIR/OUTPUT"
mkdir "$OUTPUT_DIR"
HLUW_OUTPUT_DIR="$OUTPUT_DIR/HLUW"
mkdir "$HLUW_OUTPUT_DIR"
echo ""
echo "Building preparatory data :"
set -o xtrace
"${BUILD_DIR}/bin/build-preparatory" \
    "$INPUT_GTFS_DATA" \
    "$INPUT_OSM_FILE" \
    "$INPUT_POLYGON_FILE" \
    "$WALKSPEED_KMH" \
    "$OUTPUT_DIR" \
    "$HLUW_OUTPUT_DIR"
set +o xtrace


echo ""
echo "PREPARED DATA are in directory : $OUTPUT_DIR"
