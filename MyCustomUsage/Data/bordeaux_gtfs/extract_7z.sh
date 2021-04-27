#!/bin/bash

set -o errexit
set -o nounset
set -o pipefail

file_to_extract="$1"
target_dir="${2:-.}"  # by default, extract in current directory

[ ! -d "$target_dir" ] && echo "Inexisting target directory :  $target_dir" && exit 1

echo "Prerequisite = 7z (apt install p7zip-full)"
echo "Extracting : $file_to_extract"
7z x "$file_to_extract" -so | tar -xvf - -C "$target_dir"
