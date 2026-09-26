#!/usr/bin/env sh
set -eu

root=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
items=${1:-3000000}
rounds=${2:-20}
csv=${3:-"$root/benchmarks/results.csv"}
svg=${4:-"$root/benchmarks/speedup.svg"}

mkdir -p "$(dirname "$csv")" "$(dirname "$svg")"
make -C "$root" benchmark >/dev/null
printf 'threads,items,rounds,sequential_seconds,parallel_seconds,speedup,checksum\n' > "$csv"
for threads in 1 2 4 8; do
    "$root/benchmark" --threads "$threads" --items "$items" --rounds "$rounds" --csv >> "$csv"
done
python3 "$root/scripts/plot_speedup.py" "$csv" "$svg"
printf 'Resultados: %s\nGrafica: %s\n' "$csv" "$svg"
