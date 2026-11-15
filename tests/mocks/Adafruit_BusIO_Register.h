#pragma once

#include "Adafruit_I2CDevice.h"

class Adafruit_BusIO_Register {
public:
  Adafruit_BusIO_Register(Adafruit_I2CDevice *, uint16_t address)
      : address_(address) {}

  bool read(uint8_t *value) {
    mock_i2c::last_read_page = mock_i2c::page;
    mock_i2c::last_read_register = static_cast<uint8_t>(address_);
    if (mock_i2c::fail_reads) {
      return false;
    }
    *value = address_ == 0 ? mock_i2c::page
                          : mock_i2c::registers[mock_i2c::page][address_];
    return true;
  }

  uint32_t read() {
    uint8_t value = 0;
    return read(&value) ? value : UINT32_MAX;
  }

  bool write(uint32_t value) {
    ++mock_i2c::write_count;
    if (mock_i2c::fail_writes) {
      return false;
    }
    if (address_ == 0) {
      mock_i2c::page = static_cast<uint8_t>(value);
    } else {
      // Model the software-reset bit clearing before reset() reads it back.
      mock_i2c::registers[mock_i2c::page][address_] =
          mock_i2c::page == 0 && address_ == 1
              ? 0
              : static_cast<uint8_t>(value);
    }
    return true;
  }

private:
  uint16_t address_;
};

class Adafruit_BusIO_RegisterBits {
public:
  Adafruit_BusIO_RegisterBits(Adafruit_BusIO_Register *reg, uint8_t width,
                             uint8_t shift)
      : reg_(reg), mask_((1UL << width) - 1), shift_(shift) {}

  uint32_t read() { return (reg_->read() >> shift_) & mask_; }

  bool write(uint32_t value) {
    uint8_t current = 0;
    if (!reg_->read(&current)) {
      return false;
    }
    return reg_->write((current & ~(mask_ << shift_)) |
                       ((value & mask_) << shift_));
  }

private:
  Adafruit_BusIO_Register *reg_;
  uint32_t mask_;
  uint8_t shift_;
};
