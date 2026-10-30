#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
test_dir=$(mktemp -d)
trap 'rm -f "$test_dir"/*; rmdir "$test_dir"' EXIT HUP INT TERM
: "${CC:=cc}"
: "${CFLAGS:=-std=gnu99 -O2 -Wall -Wextra -Werror}"
for source in charset crc db encode fuzzy help inet mail strutil secure xmit bit_array; do
    $CC $CFLAGS -I. -c "$source.c" -o "$test_dir/$source.o"
done
$CC $CFLAGS -I. tests/regression.c "$test_dir"/*.o ${LDFLAGS:-} -o "$test_dir/regression"
"$test_dir/regression"
