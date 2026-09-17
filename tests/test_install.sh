#!/usr/bin/env bash
# test_install.sh -- proves `cmake --install` plus `find_package(hpdf_ua)`
# actually work end-to-end for someone consuming this project as an
# installed package rather than vendoring its source tree (see
# CMakeLists.txt's "installation" section). Not just that install() runs
# without error: this configures and builds a separate, minimal consumer
# project (tests/consume_package/) against the installed tree, and runs
# the resulting binary, so a real regression (a missing header, a broken
# export, a target that doesn't actually link) fails loudly here instead
# of only being discovered by the first real downstream user.
#
# Usage:
#   tests/test_install.sh [build-dir]
#
# Exit status: 0 on success, 1 if any step (install / configure / build /
# run) fails, 2 for a usage/environment error (missing build dir).

set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="${1:-$ROOT_DIR/build}"

if [ ! -d "$BUILD_DIR" ]; then
    echo "error: build directory '$BUILD_DIR' does not exist -- run" >&2
    echo "  cmake -B $BUILD_DIR && cmake --build $BUILD_DIR" >&2
    echo "first, or pass an existing build directory as \$1." >&2
    exit 2
fi

WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT
INSTALL_PREFIX="$WORK_DIR/install"
CONSUMER_BUILD_DIR="$WORK_DIR/consume_build"

echo "== installing to $INSTALL_PREFIX =="
if ! cmake --install "$BUILD_DIR" --prefix "$INSTALL_PREFIX" \
        >"$WORK_DIR/install.log" 2>&1; then
    echo "FAIL: cmake --install failed -- see $WORK_DIR/install.log" >&2
    tail -n 40 "$WORK_DIR/install.log" >&2
    exit 1
fi
echo "install OK."
echo

echo "== configuring tests/consume_package against the installed package =="
if ! cmake -S "$SCRIPT_DIR/consume_package" -B "$CONSUMER_BUILD_DIR" \
        -DCMAKE_PREFIX_PATH="$INSTALL_PREFIX" \
        >"$WORK_DIR/configure.log" 2>&1; then
    echo "FAIL: find_package(hpdf_ua) / consumer configure failed -- see" \
         "$WORK_DIR/configure.log" >&2
    tail -n 40 "$WORK_DIR/configure.log" >&2
    exit 1
fi
echo "configure OK."
echo

echo "== building the consumer =="
if ! cmake --build "$CONSUMER_BUILD_DIR" >"$WORK_DIR/build.log" 2>&1; then
    echo "FAIL: consumer build failed -- see $WORK_DIR/build.log" >&2
    tail -n 40 "$WORK_DIR/build.log" >&2
    exit 1
fi
echo "build OK."
echo

echo "== running the consumer =="
if ! "$CONSUMER_BUILD_DIR/consume_test"; then
    echo "FAIL: consume_test exited non-zero" >&2
    exit 1
fi

echo
echo "== install + find_package(hpdf_ua) verified end-to-end =="
exit 0
