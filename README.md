# Adafruit TLV320 I2S Library [![Build Status](https://github.com/sw7t96bjx7-a11y/Adafruit_TLV320_I2S/actions/workflows/githubci.yml/badge.svg?branch=main)](https://github.com/sw7t96bjx7-a11y/Adafruit_TLV320_I2S/actions/workflows/githubci.yml)[![Documentation](https://github.com/adafruit/ci-arduino/blob/master/assets/doxygen_badge.svg)](http://adafruit.github.io/Adafruit_TLV320_I2S/html/index.html)

This is the Library for the Adafruit TLV320DAC3100 I2S DAC

Tested and works great with the Adafruit TLV320DAC3100 Breakout Board

* http://www.adafruit.com/product/6309

This chip uses I2C to initialize the DAC.

Adafruit invests time and resources providing this open source code, please support Adafruit and open-source hardware by purchasing products from Adafruit!

# Dependencies
* [Adafruit_BusIO](https://github.com/adafruit/Adafruit_BusIO)

# Contributing

Contributions are welcome! Please read our [Code of Conduct](https://github.com/adafruit/Adafruit_TLV320_I2S/blob/main/code-of-conduct.md)
before contributing to help this project stay welcoming.

## Documentation and doxygen
Documentation is produced by doxygen. Contributions should include documentation for any new code added.

Some examples of how to use doxygen can be found in these guide pages:

https://learn.adafruit.com/the-well-automated-arduino-library/doxygen

https://learn.adafruit.com/the-well-automated-arduino-library/doxygen-tips

Written by ladyada for Adafruit Industries.

MIT license, check [license.txt](https://github.com/adafruit/Adafruit_TLV320_I2S/blob/main/LICENSE.txt) for more information

All text above must be included in any redistribution

To install, use the Arduino Library Manager and search for "Adafruit TLV320 I2S" and install the library.

## Build and debug this fork

The workflow checks this repository, preserves the original nine compilation
targets, and adds a **generic ESP32-S3 compile check**. A generic target does not
identify the flash, PSRAM, USB settings, or wiring of a particular LilyGo or
Waveshare board. The examples and CI binaries are development checks, not a
validated image for a connected device.

The compiler, board packages, and Adafruit BusIO versions are recorded in
[`scripts/ci-targets.json`](scripts/ci-targets.json). The workflow builds both the
configuration example and an API link-check sketch. It also runs host regression
tests against a simulated I2C register device. These tests do not measure physical
audio, wiring, or electrical protection.

### Run locally

Install Python 3 and the Arduino CLI version listed in the target configuration.
On Linux x86-64, the repository includes a checksum-verified installer:

```sh
bash scripts/install-arduino-cli.sh
```

Run the host regression checks with a GNU C++ compiler supporting sanitizers:

```sh
python3 tests/run_tests.py
```

Compile a target using the build script (it installs the pinned board package and
library dependencies in an isolated build directory):

```sh
python3 scripts/build.py --target esp32s3 --arduino-cli ./build-ci/bin/arduino-cli
python3 scripts/build.py --target uno --arduino-cli ./build-ci/bin/arduino-cli
```

For an existing Arduino setup, pass `--config-file /path/to/arduino-cli.yaml`.
Use `--skip-install` only after installing the versions listed in the target
configuration. Each target writes its command log, a JSON build record, and any
compiled binaries under `build-ci/`. The record identifies the source revision,
local changes, target, tool versions, exit status, and artifact hashes.

### Run on GitHub

Open **Actions → Arduino Library CI → Run workflow**. Compilation, host tests,
formatting, and documentation generation run independently. Build records and
documentation are downloadable workflow artifacts. Documentation publication is
an explicit optional operation on the default branch; it is not needed to compile
or test the library. The workflow does not require a personal `GH_REPO_TOKEN`.
Publishing updates the `gh-pages` branch; a push made with `GITHUB_TOKEN` does not
by itself guarantee that a GitHub Pages site is rebuilt.

### Database scope

This repository is an Arduino audio-driver library. It contains no application
database, Meshtastic `.store` file, database schema, or migration. The JSON build
records track compilation evidence only. Database recovery or migration needs the
specific database file or the separate repository that owns it.

See [`docs/DEBUGGING.md`](docs/DEBUGGING.md) for the repaired defects, regression
coverage, and hardware-validation limits.
