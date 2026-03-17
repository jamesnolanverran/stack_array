#!/usr/bin/env bash

set -euo pipefail

usage() {
    cat <<EOF
Usage:
  ./test.sh [compiler]

Compilers:
  cc
  clang
  gcc

Examples:
  ./test.sh
  ./test.sh clang
  ./test.sh gcc
EOF
}

have_cmd() {
    command -v "$1" >/dev/null 2>&1
}

pick_compiler() {
    if [ "$#" -ge 1 ]; then
        case "$1" in
            cc|clang|gcc)
                if have_cmd "$1"; then
                    printf '%s\n' "$1"
                    return 0
                fi
                echo "[build] error: compiler '$1' not found on PATH" >&2
                exit 1
                ;;
            -h|--help)
                usage
                exit 0
                ;;
            *)
                echo "[build] error: unknown compiler '$1'" >&2
                usage >&2
                exit 1
                ;;
        esac
    fi

    for c in cc clang gcc; do
        if have_cmd "$c"; then
            printf '%s\n' "$c"
            return 0
        fi
    done

    echo "[build] error: no supported compiler found on PATH" >&2
    echo "[build] tried: cc, clang, gcc" >&2
    exit 1
}

CC="$(pick_compiler "$@")"

OUT_DIR="build"
OUT_BIN="$OUT_DIR/stack_array_tests"
TMP_WORK_DIR="$OUT_DIR/tmp"
SRC_FILES=(
    "stack_array.c"
    "tests/test_stack_array.c"
    "tests/main.c"
)

COMMON_CFLAGS=(
    "-std=c11"
    "-Wall"
    "-Wextra"
    "-Wpedantic"
    "-I."
)

mkdir -p "$OUT_DIR"
mkdir -p "$TMP_WORK_DIR"
export TMPDIR="$TMP_WORK_DIR"
export TMP="$TMP_WORK_DIR"
export TEMP="$TMP_WORK_DIR"

echo "[build] compiler: $CC"
echo "[build] output:   $OUT_BIN"

"$CC" \
    "${COMMON_CFLAGS[@]}" \
    "${SRC_FILES[@]}" \
    -o "$OUT_BIN"

echo "[build] running tests"
"$OUT_BIN"
