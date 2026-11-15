#pragma once

#include "Arduino.h"
#include "Wire.h"

// Host-only model of paged byte registers, with injectable I2C failures.
namespace mock_i2c {
extern uint8_t registers[256][256];
extern uint8_t page;
extern bool fail_reads;
extern bool fail_writes;
extern unsigned int write_count;
extern uint8_t last_read_page;
extern uint8_t last_read_register;
void clear();
} // namespace mock_i2c

class Adafruit_I2CDevice {
public:
  Adafruit_I2CDevice(uint8_t, TwoWire *) {}
  bool begin() { return true; }
};
