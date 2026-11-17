#!/bin/sh
# Usage: sh run-tests.sh [extra CTest arguments]
set -eu

# Resolve paths relative to this script so it works from any directory.
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
build_dir="$project_dir/build"

cmake -S "$project_dir" -B "$build_dir" -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build "$build_dir" --config Debug

cd "$build_dir"
ctest -C Debug --output-on-failure "$@"
