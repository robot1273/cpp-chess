"""
Benchmark script for comparing two chess engines using cutechess-cli
used for regression testing
"""

import os
import subprocess

CUTECHESS_CLI_PATH = "cutechess-cli"
local = "/home/bobot/coding/cpp/chess/chess"

# engine names in the dir
BOT_V1_PTH = "/home/bobot/cutechess/engines/engine_binaries/bot_v9"
BOT_V2_PTH = "/home/bobot/coding/cpp/stargaze/bin/stargaze"

BOT_V1 = os.path.basename(BOT_V1_PTH)
BOT_V2 = os.path.basename(BOT_V2_PTH)

TIME_CONTROL = "1+0.08"  # base time + increment
ROUNDS = 2500  # no. rounds to play (max)

# no games to play simultaneously (cores)
CONCURRENCY = os.cpu_count() - 1  # pyright: ignore[reportOptionalOperand]

OPENING_BOOK = "/home/bobot/coding/cpp/chess/book-ply8-unifen-Q-0.0-0.25.pgn"
BOOK_FORMAT = "pgn"

SPRT_SETTINGS = {
    "elo0": "0",  # not useful keep at 0
    "elo1": "10",  # minimum elo difference to determine v2 is better than v1
    "alpha": "0.05",  # false positive probability
    "beta": "0.05",
}  # false negative probability


def run_tournament():
    if not os.path.exists(BOT_V1_PTH):
        print(f"could not find V1 engine at {BOT_V1_PTH}")
        return
    if not os.path.exists(BOT_V2_PTH):
        print(f"could not find V2 engine at {BOT_V2_PTH}")
        return

    cmd = [
        CUTECHESS_CLI_PATH,
        "-engine",
        f"cmd={BOT_V2_PTH}",
        f"name={BOT_V2}",
        "-engine",
        f"cmd={BOT_V1_PTH}",
        f"name={BOT_V1}",
        "-each",
        "proto=uci",
        f"tc={TIME_CONTROL}",
        "-games",
        "2",
        "-rounds",
        str(ROUNDS),
        "-repeat",
        "-concurrency",
        str(CONCURRENCY),
    ]

    if OPENING_BOOK and os.path.exists(OPENING_BOOK):
        cmd.extend(
            [
                "-openings",
                f"file={OPENING_BOOK}",
                f"format={BOOK_FORMAT}",
                "order=random",
            ]
        )
    elif OPENING_BOOK:
        raise FileNotFoundError(f"opening book '{OPENING_BOOK}' not found")

    if SPRT_SETTINGS:
        cmd.extend(
            [
                "-sprt",
                f"elo0={SPRT_SETTINGS['elo0']}",
                f"elo1={SPRT_SETTINGS['elo1']}",
                f"alpha={SPRT_SETTINGS['alpha']}",
                f"beta={SPRT_SETTINGS['beta']}",
            ]
        )

    print("running commmand: \n" + " ".join(cmd) + "\n")

    try:
        subprocess.run(cmd, check=True)
    except KeyboardInterrupt:
        print("\ncancelled.")


if __name__ == "__main__":
    run_tournament()
