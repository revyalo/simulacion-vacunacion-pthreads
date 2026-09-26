#!/usr/bin/env sh
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
tmp_dir=$(mktemp -d "${TMPDIR:-/tmp}/vaccination-test.XXXXXX")
trap 'rm -rf "$tmp_dir"' EXIT HUP INT TERM

if [ "$#" -eq 0 ]; then
    set -- "$root/practica2"
fi

index=0
for binary in "$@"; do
    index=$((index + 1))
    "$root/tests/assert_simulation.sh" "$binary" "$root/tests/data/valid-17.txt" "$tmp_dir/result-$index.txt" 17
done

"$root/tests/assert_simulation.sh" "$1" "$root/tests/data/edge-one.txt" "$tmp_dir/edge.txt" 1

if "$1" "$root/tests/data/invalid-extra-field.txt" "$tmp_dir/invalid.txt" >/dev/null 2>&1; then
    echo "se acepto un fichero de entrada con datos extra" >&2
    exit 1
fi

echo "simulation tests: OK"
