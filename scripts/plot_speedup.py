#!/usr/bin/env python3
import csv
import html
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 3:
        print(f"uso: {sys.argv[0]} resultados.csv speedup.svg", file=sys.stderr)
        return 2

    with open(sys.argv[1], newline="", encoding="utf-8") as source:
        rows = list(csv.DictReader(source))
    if not rows:
        print("el CSV no contiene resultados", file=sys.stderr)
        return 1

    values = [(int(row["threads"]), float(row["speedup"])) for row in rows]
    width, height = 720, 420
    left, right, top, bottom = 72, 28, 45, 62
    plot_width = width - left - right
    plot_height = height - top - bottom
    max_x = max(thread for thread, _ in values)
    max_y = max(max(speedup for _, speedup in values), float(max_x), 1.0)

    def point(thread: int, speedup: float) -> tuple[float, float]:
        return (
            left + (thread - 1) / max(max_x - 1, 1) * plot_width,
            top + plot_height - speedup / max_y * plot_height,
        )

    measured = " ".join(f"{x:.1f},{y:.1f}" for x, y in (point(*value) for value in values))
    ideal = " ".join(
        f"{x:.1f},{y:.1f}" for x, y in (point(thread, float(thread)) for thread, _ in values)
    )
    elements = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#0f172a"/>',
        '<text x="36" y="28" fill="#f8fafc" font-family="sans-serif" font-size="20" font-weight="bold">Speedup: secuencial frente a pthreads</text>',
        f'<line x1="{left}" y1="{top}" x2="{left}" y2="{top + plot_height}" stroke="#94a3b8"/>',
        f'<line x1="{left}" y1="{top + plot_height}" x2="{left + plot_width}" y2="{top + plot_height}" stroke="#94a3b8"/>',
        f'<polyline points="{ideal}" fill="none" stroke="#64748b" stroke-width="2" stroke-dasharray="7 6"/>',
        f'<polyline points="{measured}" fill="none" stroke="#38bdf8" stroke-width="3"/>',
    ]
    for thread, speedup in values:
        x, y = point(thread, speedup)
        elements.extend([
            f'<circle cx="{x:.1f}" cy="{y:.1f}" r="5" fill="#38bdf8"/>',
            f'<text x="{x:.1f}" y="{top + plot_height + 23}" text-anchor="middle" fill="#cbd5e1" font-family="sans-serif" font-size="13">{thread}</text>',
            f'<text x="{x:.1f}" y="{y - 10:.1f}" text-anchor="middle" fill="#f8fafc" font-family="sans-serif" font-size="12">{html.escape(f"{speedup:.2f}x")}</text>',
        ])
    for tick in range(0, int(max_y) + 1):
        _, y = point(1, float(tick))
        elements.append(f'<text x="{left - 10}" y="{y + 4:.1f}" text-anchor="end" fill="#94a3b8" font-family="sans-serif" font-size="12">{tick}x</text>')
    elements.extend([
        f'<text x="{left + plot_width / 2:.1f}" y="{height - 18}" text-anchor="middle" fill="#cbd5e1" font-family="sans-serif" font-size="14">Número de hilos</text>',
        f'<text transform="translate(20 {top + plot_height / 2:.1f}) rotate(-90)" text-anchor="middle" fill="#cbd5e1" font-family="sans-serif" font-size="14">Speedup</text>',
        '<text x="500" y="28" fill="#38bdf8" font-family="sans-serif" font-size="12">medido</text>',
        '<text x="570" y="28" fill="#94a3b8" font-family="sans-serif" font-size="12">ideal</text>',
        '</svg>',
    ])
    Path(sys.argv[2]).write_text("\n".join(elements), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
