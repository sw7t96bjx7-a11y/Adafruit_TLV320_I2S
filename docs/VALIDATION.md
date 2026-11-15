# Validation record — 2026-11-15

These results were observed for the repair source in this checkout before its commit. The build script records source-file hashes to identify the tested content. They establish compilation and the stated host-test behavior, not physical-device operation.

## Observed checks

| Check | Result |
| --- | --- |
| Host driver tests | Passed 136 assertions with undefined-behavior and float-cast sanitizers |
| GitHub workflow syntax | Passed actionlint 1.7.12 |
| Library formatting | Passed clang-format 18.1.8 (LLVM style) |
| Patch whitespace | Passed `git diff --check` |
| Documentation | Doxygen 1.18.0 exited 0; 18 HTML pages; empty warnings log |

## Actual target builds

Both `examples/basicI2Sconfig` and `tests/api_link` compiled and linked for every target below.

| Target | FQBN | Pinned board core | Result |
| --- | --- | --- | --- |
| uno | `arduino:avr:uno` | `arduino:avr@1.8.8` | Exit 0; both sketches compiled |
| esp32 | `esp32:esp32:featheresp32:FlashFreq=80` | `esp32:esp32@3.3.12` | Exit 0; both sketches compiled |
| esp32s3 | `esp32:esp32:esp32s3` | `esp32:esp32@3.3.12` | Exit 0; both sketches compiled |

All three used Arduino CLI 1.6.0 and Adafruit BusIO 1.17.4. The Uno build emitted unused-parameter warnings from the upstream AVR core, not errors. The repaired driver host tests emitted no warnings.

## Compiled artifact hashes

The following are actual application images produced by these checks. They are compile-test outputs; no device-specific memory, USB, pinout, recovery, or flashing settings were validated.

| Target | Artifact | Bytes | SHA-256 |
| --- | --- | ---: | --- |
| uno | `binaries/api_link/api_link.ino.hex` | 31587 | `8065137812cc3bbed14d6951348d2ca5f4bc86bf83ad9381730008598786eaaa` |
| uno | `binaries/basicI2Sconfig/basicI2Sconfig.ino.hex` | 33727 | `dc7f5ead8b991f5c7dbd83626cc9be87a9d9a1b7cc14202cbd640a67e1475ec2` |
| esp32 | `binaries/api_link/api_link.ino.bin` | 296496 | `7fed1f937cc5d6466d35c87d2a4dc84015d20ae2fa71f6f0085e27dd181f5753` |
| esp32 | `binaries/basicI2Sconfig/basicI2Sconfig.ino.bin` | 304896 | `01fc1b1f372f9fc445157e3b8b21581e2d24df5c00f598b55dc0356135cd35af` |
| esp32s3 | `binaries/api_link/api_link.ino.bin` | 328368 | `f7ad4208a223390fd0c7284d9f52466c78152b47427c03de94f3b52aa4d1c941` |
| esp32s3 | `binaries/basicI2Sconfig/basicI2Sconfig.ino.bin` | 336336 | `1ae7f4992623531ecc1f24dab6383d656b7824866b778b525af8e40a21651300` |

## Limits and remaining checks

- The workflow defines ten target profiles. Only the three profiles listed above were compiled locally for this record; GitHub Actions results are the source of truth for the complete matrix.
- Local documentation validation used Doxygen 1.18.0. The Ubuntu 24.04 workflow installs its packaged Doxygen, so the exact CI documentation environment is checked separately.
- Documentation warnings are retained in `docs-build/warnings.log` as advisory output. A Doxygen execution failure or missing HTML still fails the job.
- No physical device was connected, flashed, or tested. No database file or migration exists in this repository.
- A local parallel-build failure exposed Arduino sketch-cache collisions between board targets. The build script now supplies separate target/sketch work directories; the three successful builds above used that correction.

For a new result, run `python3 tests/run_tests.py` and the commands in the README. Each target generates its own `build.log` and `build-record.json`. Do not use this dated record as evidence that a later revision passes.
