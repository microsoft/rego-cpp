#!/usr/bin/env python3
"""Smoke test the installed regopy wheel on musl.

Runs a simple query on the main thread and on a worker thread; the worker
forces snmalloc's pthread-key thread-teardown to run at thread exit.
"""

import sys
import threading

from regopy import Interpreter


def run() -> None:
    out = Interpreter().query("x = 1 + 1")
    got = out.binding("x").json()
    if got != "2":
        raise AssertionError(f"expected '2', got {got!r}")


def main() -> int:
    run()
    t = threading.Thread(target=run)
    t.start()
    t.join()
    print("OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
