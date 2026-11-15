#!/usr/bin/env python3
"""Build and run host regression tests; this does not emulate physical audio."""

import os
from pathlib import Path
import shlex
import subprocess
import tempfile


def main():
    root = Path(__file__).resolve().parents[1]
    compiler = shlex.split(os.environ.get("CXX", "g++"))
    with tempfile.TemporaryDirectory(prefix="tlv320-tests-") as output:
        binary = Path(output) / "driver_regressions"
        command = compiler + [
            "-std=c++17",
            "-Wall",
            "-Wextra",
            "-fsanitize=undefined,float-cast-overflow",
            "-fno-sanitize-recover=all",
            "-I", str(root / "tests" / "mocks"),
            "-I", str(root),
            str(root / "Adafruit_TLV320DAC3100.cpp"),
            str(root / "tests" / "driver_regressions.cpp"),
            "-o", str(binary),
        ]
        subprocess.run(command, check=True)
        subprocess.run([str(binary)], check=True)


if __name__ == "__main__":
    main()
