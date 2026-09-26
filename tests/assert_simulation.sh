#!/usr/bin/env sh
set -eu

if [ "$#" -ne 4 ]; then
    echo "uso: $0 ejecutable entrada salida habitantes" >&2
    exit 2
fi

binary=$1
input=$2
output=$3
expected=$4

"$binary" "$input" "$output" >/dev/null
grep -q "VACUNACION FINALIZADA" "$output"
grep -q "Habitantes vacunados: $expected de $expected" "$output"

if grep -Eq 'pendientes: [1-9][0-9]*' "$output"; then
    echo "quedaron habitantes pendientes en $binary" >&2
    exit 1
fi
