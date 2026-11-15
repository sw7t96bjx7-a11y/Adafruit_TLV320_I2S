#include "Adafruit_TLV320DAC3100.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>

TwoWire Wire;

namespace mock_i2c {
uint8_t registers[256][256] = {};
uint8_t page = 0;
bool fail_reads = false;
bool fail_writes = false;
unsigned int write_count = 0;
uint8_t last_read_page = 0;
uint8_t last_read_register = 0;

void clear() {
  std::memset(registers, 0, sizeof(registers));
  page = 0;
  fail_reads = false;
  fail_writes = false;
  write_count = 0;
  last_read_page = 0;
  last_read_register = 0;
}
} // namespace mock_i2c

static unsigned int assertions = 0;

static void expect(bool condition, const char *message) {
  ++assertions;
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}

static uint32_t beepSamples() {
  return (uint32_t(mock_i2c::registers[0][0x49]) << 16) |
         (uint32_t(mock_i2c::registers[0][0x4A]) << 8) |
         mock_i2c::registers[0][0x4B];
}

int main() {
  mock_i2c::clear();
  Adafruit_TLV320DAC3100 codec;
  expect(codec.begin(), "mock device initialization");

  // TI SLAS671C, page 1 / register 31: only D0 indicates a headphone short.
  mock_i2c::registers[1][0x1F] = 0xFE;
  mock_i2c::registers[0][0x2C] = 0xC0; // Sticky IRQ flags are independent.
  expect(!codec.isHeadphoneShorted(), "other headphone bits are not short flags");
  expect(mock_i2c::last_read_page == 1 &&
             mock_i2c::last_read_register == 0x1F,
         "headphone detection reads page 1 register 31");
  mock_i2c::registers[1][0x1F] = 0xFF;
  expect(codec.isHeadphoneShorted(), "D0 reports the headphone short");
  expect(mock_i2c::registers[0][0x2C] == 0xC0,
         "headphone query leaves sticky IRQ flags alone");
  mock_i2c::fail_reads = true;
  expect(!codec.isHeadphoneShorted(), "failed register read returns false");
  mock_i2c::fail_reads = false;
  mock_i2c::fail_writes = true;
  expect(!codec.isHeadphoneShorted(), "failed page selection returns false");
  mock_i2c::fail_writes = false;

  expect(codec.configureBeepTone(1000.0f, 1000, 48000), "one second beep");
  expect(beepSamples() == 48000, "one second is 48000 samples");
  expect(codec.configureBeepTone(1000.0f, 90000, 48000), "90 second beep");
  expect(beepSamples() == 4320000,
         "duration multiplication does not overflow after 89 seconds");
  expect(codec.configureBeepTone(1000.0f, UINT32_MAX, UINT32_MAX),
         "maximum integer inputs clamp successfully");
  expect(beepSamples() == 0xFFFFFF, "beep duration saturates at 24 bits");

  const float invalid_frequencies[] = {
      0.0f, -1.0f, 12000.0f, 24000.0f,
      std::numeric_limits<float>::infinity(),
      std::numeric_limits<float>::quiet_NaN()};
  for (float frequency : invalid_frequencies) {
    const unsigned int writes_before = mock_i2c::write_count;
    expect(!codec.configureBeepTone(frequency, 1000, 48000),
           "invalid frequency rejected");
    expect(mock_i2c::write_count == writes_before,
           "invalid frequency does not change device registers");
  }
  const unsigned int writes_before = mock_i2c::write_count;
  expect(!codec.configureBeepTone(1000.0f, 1000, 0), "zero sample rate rejected");
  expect(mock_i2c::write_count == writes_before,
         "invalid sample rate does not change device registers");
  mock_i2c::fail_writes = true;
  expect(!codec.configureBeepTone(1000.0f, 1000, 48000),
         "I2C write failure propagates from beep configuration");
  mock_i2c::fail_writes = false;

  expect(codec.setChannelVolume(false, -100.0f), "finite low volume clamps");
  expect(mock_i2c::registers[0][0x41] == 0x81, "low volume clamps to -63.5 dB");
  expect(codec.setChannelVolume(true, 100.0f), "finite high volume clamps");
  expect(mock_i2c::registers[0][0x42] == 0x30, "high volume clamps to +24 dB");
  const unsigned int volume_writes_before = mock_i2c::write_count;
  expect(!codec.setChannelVolume(false,
                                 std::numeric_limits<float>::quiet_NaN()),
         "NaN volume rejected before integer conversion");
  expect(!codec.setChannelVolume(false,
                                 std::numeric_limits<float>::infinity()),
         "infinite volume rejected");
  expect(mock_i2c::write_count == volume_writes_before,
         "nonfinite volume does not write registers");

  // TI SLAS671C, table 6-28. D is a decimal fraction in ten-thousandths.
  struct PLLExample {
    uint32_t input;
    uint8_t p, r, j;
    uint16_t d;
  };
  const PLLExample examples[] = {
      {2822400, 1, 3, 10, 0}, {5644800, 1, 3, 5, 0},
      {12000000, 1, 1, 7, 560}, {13000000, 1, 1, 6, 3504},
      {16000000, 1, 1, 5, 2920}, {19200000, 1, 1, 4, 4100},
      {48000000, 4, 1, 7, 560}, {2048000, 1, 3, 14, 0},
      {3072000, 1, 4, 7, 0}, {4096000, 1, 3, 7, 0},
      {6144000, 1, 2, 7, 0}, {8192000, 1, 4, 3, 0},
      {12000000, 1, 1, 7, 1680}, {16000000, 1, 1, 5, 3760},
      {19200000, 1, 1, 4, 4800}};
  for (const PLLExample &example : examples) {
    expect(codec.validatePLLConfig(example.p, example.r, example.j, example.d,
                                   example.input),
           "datasheet PLL example is valid");
    const uint32_t desired =
        uint64_t(example.input) * (10000 * example.j + example.d) * example.r /
        (10000 * example.p);
    expect(codec.configurePLL(example.input, desired, 0.001f),
           "PLL search supports datasheet output frequency");
    uint8_t p = 0, r = 0, j = 0;
    uint16_t d = 0;
    expect(codec.getPLLValues(&p, &r, &j, &d), "read programmed PLL registers");
    const double actual = double(example.input) * (j + d / 10000.0) * r / p;
    expect(std::fabs(actual - desired) / desired <= 0.001,
           "programmed hardware frequency satisfies requested tolerance");
    expect(codec.validatePLLConfig(p, r, j, d, example.input),
           "programmed PLL configuration is electrically valid");
  }

  expect(codec.configurePLL(1000000, 96000000, 0.0f),
         "integer PLL search uses R as a multiplier");
  expect(codec.configurePLL(12000000, 86016000, 0.0f),
         "exact fractional PLL frequency satisfies zero tolerance");
  expect(codec.configurePLL(19200000, 110000000, 0.001f),
         "PLL search rounds toward a valid value at the output limit");
  expect(codec.configurePLL(1000000, 95990000, 0.001f),
         "integer mode can approximate within the requested tolerance");
  const unsigned int pll_writes_before = mock_i2c::write_count;
  expect(!codec.configurePLL(1000000, 95990000, 0.00001f),
         "PLL does not silently relax a strict tolerance to 10 percent");
  expect(!codec.configurePLL(0, 96000000), "zero PLL input rejected");
  expect(!codec.configurePLL(12000000, 0), "zero PLL output rejected");
  expect(!codec.configurePLL(12000000, 79000000), "low PLL output rejected");
  expect(!codec.configurePLL(12000000, 111000000), "high PLL output rejected");
  expect(!codec.configurePLL(12000000, 96000000, -0.1f),
         "negative tolerance rejected");
  expect(!codec.configurePLL(12000000, 96000000,
                            std::numeric_limits<float>::quiet_NaN()),
         "NaN tolerance rejected");
  expect(!codec.configurePLL(12000000, 96000000,
                            std::numeric_limits<float>::infinity()),
         "infinite tolerance rejected");
  expect(mock_i2c::write_count == pll_writes_before,
         "invalid or unsatisfiable PLL requests do not write registers");

  expect(!codec.validatePLLConfig(0, 1, 8, 0, 12000000), "P=0 rejected");
  expect(!codec.validatePLLConfig(9, 1, 8, 0, 12000000), "P>8 rejected");
  expect(!codec.validatePLLConfig(1, 0, 8, 0, 12000000), "R=0 rejected");
  expect(!codec.validatePLLConfig(1, 17, 8, 0, 12000000), "R>16 rejected");
  expect(!codec.validatePLLConfig(1, 1, 0, 0, 12000000), "J=0 rejected");
  expect(!codec.validatePLLConfig(1, 1, 64, 0, 12000000), "J>63 rejected");
  expect(!codec.validatePLLConfig(1, 1, 8, 10000, 12000000), "D>9999 rejected");
  expect(!codec.validatePLLConfig(1, 2, 4, 1, 12000000),
         "fractional PLL requires R=1");
  expect(!codec.validatePLLConfig(1, 1, 8, 0, 0), "zero input rejected");
  expect(!codec.validatePLLConfig(1, 1, 8, 0,
                                 std::numeric_limits<float>::quiet_NaN()),
         "NaN input rejected");
  expect(!codec.validatePLLConfig(1, 1, 8, 0,
                                 std::numeric_limits<float>::infinity()),
         "infinite input rejected");
  mock_i2c::fail_writes = true;
  expect(!codec.configurePLL(12000000, 96000000),
         "PLL configuration propagates I2C page-write failure");
  mock_i2c::fail_writes = false;
  mock_i2c::fail_reads = true;
  expect(!codec.configurePLL(12000000, 96000000),
         "PLL configuration propagates I2C register-read failure");

  std::cout << "Driver regression checks passed (" << assertions
            << " assertions).\n";
}
