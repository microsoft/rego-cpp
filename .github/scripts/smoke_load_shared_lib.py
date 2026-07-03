#!/usr/bin/env python3
"""Dlopen librego_shared.so; fail if any undefined symbol does not resolve.

Used by the pr_gate musl load test: builds the shared library inside the
musllinux image, then runs this script in a plain python:X-alpine container
to catch glibc-only undefined symbols at load time.
"""

import ctypes
import sys


def main() -> int:
    path = sys.argv[1] if len(sys.argv) > 1 else "/lib_out/librego_shared.so"
    ctypes.CDLL(path)
    print(f"OK: {path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
