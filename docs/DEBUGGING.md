# Driver and build repair notes

This update repairs the Arduino library in this repository. It does not change a
device, install firmware, recover a Meshtastic database, or establish electrical
compatibility with a particular board.

## Repaired behavior

### Headphone short-circuit status

`isHeadphoneShorted()` was declared in the public header but had no implementation.
A sketch calling it could compile its source and then fail to link. The method now
reads the headphone status flag at page 1, register 31, bit D0. It returns `false`
on a failed page selection or register read, consistent with its existing Boolean
API. Therefore `false` alone cannot distinguish an I2C failure from a clear flag;
this API is not a substitute for hardware protection.

### Beep duration and input checks

`configureBeepTone()` previously multiplied duration and sample rate in a 32-bit
integer. At 48 kHz, a 90,000 ms duration wrapped before division and produced
25,032 samples instead of 4,320,000. The multiplication now uses 64 bits, then
retains the existing 24-bit hardware-length limit. Invalid frequencies and a zero
sample rate are rejected before changing registers.

### PLL frequency calculation

The chip's clock relationship is:

```text
PLL output = PLL input × R × (J + D / 10000) / P
```

The former fractional divisor of 2048 and inverse use of R could select the wrong
clock or reject valid clock settings. The configuration and validation functions
now use the datasheet equation and its permitted input/output ranges.

`max_error` is a relative error limit: `0.001` means 0.1%. Requests that cannot be
met within this limit return `false`; the former fallback could silently accept
an error approaching 10%. Callers must check the return value. Direct programming
through `setPLLValues()` remains available with its existing API.

### Volume input validation

Nonfinite volume values now return `false` before conversion to a register byte.
Existing clamping for finite dB values is preserved.

### Headset example reporting

The sticky headset-interrupt bit reports an insertion or removal event. The old
example's removal branch checked the same bit already handled by its insertion
branch, so it was unreachable. The message now describes the combined event; the
separate headset-status query supplies the current state.

Register and clock references: [Texas Instruments TLV320DAC3100 datasheet,
SLAS671C](https://www.ti.com/lit/ds/symlink/tlv320dac3100.pdf), including the PLL
section, the headphone-driver control register, and the headset interrupt tables.

## Validation layers

- `python3 tests/run_tests.py` compiles the actual driver with a small simulated
  I2C/register backend and executes regression checks with undefined-behavior and
  floating-point conversion sanitizers.
- `tests/api_link/api_link.ino` references the repaired public methods so the
  actual Arduino linker checks them on each target.
- `scripts/build.py` compiles both the API-link sketch and the existing
  configuration example. It checks the pinned compiler, board-core, and library
  versions and preserves commands, outcomes, source hashes, and binary hashes.
- The workflow runs the original nine board profiles and a generic ESP32-S3
  profile. Its formatting and documentation jobs are independent of compilation.

Passing these checks establishes the recorded software behavior and compilation
only. No physical board, analog audio path, headset, I2C bus, RF device, USB port,
or wiring is exercised by a host regression test or a cloud build.

## Database boundary

There is no application database, SQL schema, migration, or Meshtastic `.store`
file in the inspected repository. The versioned JSON build-record format is an
evidence log, not a device or user-data database. A database repair requires the
actual database and its owning application's schema/version.

## Documentation publishing

Documentation warnings are advisory and saved in `docs-build/warnings.log`; a
Doxygen failure or missing generated HTML still fails the job.

Documentation generation no longer requires a secret or an existing `gh-pages`
branch. It creates an artifact for inspection. Publishing to the existing
documentation branch is available only through a manual workflow input on the
default branch after all checks pass. The publishing job alone receives write
permission and uses the workflow token. It does not force-push or replace files
outside the generated `html/` directory and `.nojekyll` marker.

GitHub Pages hosting remains a separate repository setting; a branch update made
with `GITHUB_TOKEN` does not itself guarantee a Pages rebuild.

The observed local results are recorded in [VALIDATION.md](VALIDATION.md).
