#!/usr/bin/env bash

BUILD_TYPE="debug"
TEST_SCOPE=""

usage() {
    cat <<EOF
usage: $0 [options]

build:
  -r, --release         build Release (default: Debug)

tests (omit to build only):
  -t, --test [SCOPE]    build and run tests, SCOPE = unit | device | all
                        (default: all)
EOF
}

while (( $# )); do
    arg=$1; shift
    case "$arg" in
        r|-r|--release) BUILD_TYPE="release" ;;
        t|-t|--test)
            if [[ ${1:-} && $1 != -* ]]; then TEST_SCOPE="$1"; shift
            else TEST_SCOPE="all"; fi ;;
        *) usage >&2; exit 1 ;;
    esac
done

BUILD_DIR="build/${BUILD_TYPE}"
mkdir -p "$BUILD_DIR"

case "$TEST_SCOPE" in
    ""|all) LABELS='unit|device' ;;
    unit)   LABELS='unit' ;;
    device) LABELS='device' ;;
esac

cmake -S . -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DBUILD_TESTING="$([[ -n $TEST_SCOPE ]] && echo ON || echo OFF)"
cmake --build "$BUILD_DIR" -j$(nproc)

if [[ -n "$TEST_SCOPE" ]]; then
    ctest --test-dir "$BUILD_DIR" -L "$LABELS" --output-on-failure
fi

