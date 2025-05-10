#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522v2.h>

//-----------------------------------------------------------------------------
// A tiny concrete implementation of MFRC522DriverPin, using a single GPIO.
//-----------------------------------------------------------------------------
class PinDriver : public MFRC522DriverPin {
public:
  explicit PinDriver(uint8_t pin) : _pin(pin) {}
  bool init() override {
    pinMode(_pin, OUTPUT);
    return true;
  }
  void high() override { digitalWrite(_pin, HIGH); }
  void low()  override { digitalWrite(_pin, LOW); }
private:
  uint8_t _pin;
};

//-----------------------------------------------------------------------------
// RFIDAuth: handles RC522 reads + a 10-minute unlock window.
//-----------------------------------------------------------------------------
class RFIDAuth {
public:
  // csPin is the SPI‐CS line for your RC522 module
  explicit RFIDAuth(uint8_t csPin);

  // initialize SPI & card
  void begin();

  // poll the reader once (non-blocking, fast)
  // if a new card is present, extend your unlock window by 10 min
  void update();

  // true if still within 10 min of last valid tap
  bool allowAction() const;

private:
  PinDriver           _csDriver;    ///< drives the RC522 CS line
  MFRC522DriverSPI    _driver;      ///< SPI transport
  MFRC522             _rfid;        ///< high-level RFID logic

  // timestamp (ms) until which actions remain allowed
  uint32_t            _unlockUntil = 0;
};
