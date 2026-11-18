#!/usr/bin/env bash

BUILD_TYPE="debug"
BUILD_TESTING="OFF"

for arg in "$@"; do
    case "$arg" in
        r|-r|--release) BUILD_TYPE="release" ;;
        t|-t|--test) BUILD_TESTING="ON" ;;
        *) echo "unknown option: $arg" >&2; exit 1 ;;
    esac
done

BUILD_DIR="build/${BUILD_TYPE}"
mkdir -p "$BUILD_DIR"

cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_TESTING="$BUILD_TESTING"
cmake --build "$BUILD_DIR" -j$(nproc)

if [[ "$BUILD_TESTING" == "ON" ]]; then
    ctest --test-dir "$BUILD_DIR" -L unit --output-on-failure
fi

