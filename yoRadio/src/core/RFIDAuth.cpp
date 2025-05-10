#include "RFIDAuth.h"

RFIDAuth::RFIDAuth(uint8_t csPin)
  : _csDriver(csPin),
    _driver(_csDriver, SPI, SPISettings(4'000'000u, MSBFIRST, SPI_MODE0)),
    _rfid(_driver)
{}

void RFIDAuth::begin() {
  _rfid.PCD_Init();
}

void RFIDAuth::update() {
  // on every new successful read, reset our 10-minute timer
  if (_rfid.PICC_IsNewCardPresent() && _rfid.PICC_ReadCardSerial()) {
    _unlockUntil = millis() + 10ul * 60 * 1000;  // 10 minutes
  }
}

bool RFIDAuth::allowAction() const {
  // remains true while now < unlockUntil
  return (int32_t)(_unlockUntil - millis()) > 0;
}
