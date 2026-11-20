"""Exercise the ROM's partner selector with a century of daily FNV-1a seeds."""

from collections import Counter
from datetime import date, timedelta
from itertools import groupby
from pathlib import Path
import os
import subprocess
import tempfile


def daily_seed(day):
    seed = 0x811C9DC5
    for byte in f"dailysteven:{day.isoformat()}".encode("ascii"):
        seed = ((seed ^ byte) * 0x01000193) & 0xFFFFFFFF
    return seed or 1


def main():
    first_day = date(2000, 1, 1)
    days = (date(2100, 1, 1) - first_day).days
    seeds = [daily_seed(first_day + timedelta(days=i)) for i in range(days)]
    include = Path(__file__).resolve().parents[1] / "overlay" / "include"

    with tempfile.TemporaryDirectory() as directory:
        source = Path(directory) / "partner.c"
        binary = Path(directory) / "partner"
        source.write_text(
            '#include <inttypes.h>\n'
            '#include <stdio.h>\n'
            '#include "dailymaximo_partner.h"\n'
            'int main(void)\n'
            '{\n'
            '    uint32_t seed;\n'
            '    while (scanf("%" SCNu32, &seed) == 1)\n'
            '        puts(DailyMax_SeedSelectsBlasco(seed) ? "B" : "A");\n'
            '    return 0;\n'
            '}\n',
            encoding="ascii",
        )
        subprocess.run(
            [os.environ.get("CC", "cc"), "-std=c99", "-Wall", "-Wextra",
             "-Werror", "-I", str(include), str(source), "-o", str(binary)],
            check=True,
        )
        inputs = [*seeds, *reversed(seeds), 900002, 900003]
        output = subprocess.run(
            [str(binary)], input="\n".join(map(str, inputs)),
            text=True, capture_output=True, check=True,
        ).stdout.splitlines()

    assert len(output) == len(inputs)
    choices = output[:days]
    assert output[days:-2] == choices[::-1], "Date selection depends on call order"
    assert output[-2:] == ["A", "B"], "Debug victory fixtures changed partners"
    totals = Counter(choices)
    assert 0.48 <= totals["B"] / days <= 0.52, totals
    repeats = sum(a == b for a, b in zip(choices, choices[1:])) / (days - 1)
    assert 0.48 <= repeats <= 0.52, repeats
    print(f"{days} dates: {totals['A']} Aura, {totals['B']} Blasco, {repeats:.2%} repeats")

    runs = [(partner, sum(1 for _ in run)) for partner, run in groupby(choices)]
    for partner in ("A", "B"):
        lengths = [length for current, length in runs if current == partner]
        assert {3, 4, 5}.issubset(lengths) and max(lengths) > 5, lengths
        print(f"{partner}: runs of 3, 4 and 5; maximum {max(lengths)}")
        # After a streak of N, longer runs represent continuations. Omit the
        # last run, which can be cut short by the end of the sample.
        completed = [length for current, length in runs[:-1] if current == partner]
        for streak in range(1, 6):
            continuation = (sum(n > streak for n in completed)
                            / sum(n >= streak for n in completed))
            assert 0.43 <= continuation <= 0.57, (partner, streak, continuation)
            print(f"  After {streak}: {continuation:.2%} continue")


if __name__ == "__main__":
    main()
