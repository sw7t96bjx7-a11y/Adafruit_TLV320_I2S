// Compile/link coverage for API methods not used by the basic example.
// This sketch does not validate a board's wiring or audio output.
#include <Adafruit_TLV320DAC3100.h>

Adafruit_TLV320DAC3100 codec;

void setup() {
  if (codec.begin()) {
    (void)codec.isHeadphoneShorted();
    (void)codec.configureBeepTone(1000.0f, 90000, 48000);
    (void)codec.configurePLL(12000000, 96000000);
  }
}

void loop() {}
