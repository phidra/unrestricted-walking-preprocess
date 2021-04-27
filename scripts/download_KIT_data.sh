#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

# this script downloads partial/complete data from KIT's ULTRA server :

SWITZERLAND_BASE_URL="https://i11www.iti.kit.edu/PublicTransitData/Switzerland/binaryFiles"
SWITZERLAND_PARTIAL_URL="${SWITZERLAND_BASE_URL}/partial"
SWITZERLAND_COMPLETE_URL="${SWITZERLAND_BASE_URL}/complete"

files_to_download="""
raptor.binary
raptor.binary.graph.beginOut
raptor.binary.graph.coordinates
raptor.binary.graph.statistics.txt
raptor.binary.graph.toVertex
raptor.binary.graph.travelTime
"""
# note : there are also some files for CSA on the server...

this_script_parent="$(realpath "$(dirname "$0")" )"
GIT_ROOT="$(dirname "${this_script_parent}" )"
PARTIAL_DIR="${GIT_ROOT}/DOWNLOADED_DATA/partial"
COMPLETE_DIR="${GIT_ROOT}/DOWNLOADED_DATA/complete"

# partial :
if [ ! -d "${PARTIAL_DIR}" ]
then
    mkdir -p "${PARTIAL_DIR}"
    pushd "${PARTIAL_DIR}"
    for f in $files_to_download
    do
        echo "About to download PARTIAL file : ${f}"
        wget "${SWITZERLAND_PARTIAL_URL}/$f"
    done
    popd
else
    echo "Not downloading PARTIAL data : already existing target dir : ${PARTIAL_DIR}"
fi


# complete :
if [ ! -d "${COMPLETE_DIR}" ]
then
    mkdir -p "${COMPLETE_DIR}"
    pushd "${COMPLETE_DIR}"
    for f in $files_to_download
    do
        echo "About to download COMPLETE file : ${f}"
        wget "${SWITZERLAND_COMPLETE_URL}/$f"
    done
    popd
else
    echo "Not downloading COMPLETE data : already existing target dir : ${COMPLETE_DIR}"
fi
