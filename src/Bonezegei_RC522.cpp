/*
    MFRC522 RFID Reader/Writer with MIFARE Classic
    Author: Jofel Batutay (Bonezegei)
    Date: October 14, 2026
*/

#include "Bonezegei_RC522.h"

Bonezegei_RC522::Bonezegei_RC522(uint8_t ssPin, uint8_t rstPin) {
  _ssPin = ssPin;
  _rstPin = rstPin;
  _uidSize = 0;
}

bool Bonezegei_RC522::begin() {
  pinMode(_ssPin, OUTPUT);
  digitalWrite(_ssPin, HIGH);

  if (_rstPin != 0xFF) {
    pinMode(_rstPin, OUTPUT);
    digitalWrite(_rstPin, HIGH);
  }

  SPI.begin();
  reset();

  writeRegister(RC522_TModeReg, 0x8D);
  writeRegister(RC522_TPrescalerReg, 0x3E);
  writeRegister(RC522_TReloadRegL, 30);
  writeRegister(RC522_TReloadRegH, 0);

  writeRegister(RC522_TxASKReg, 0x40);
  writeRegister(RC522_ModeReg, 0x3D);

  antennaOn();

  uint8_t ver = readRegister(RC522_VersionReg);
  return (ver != 0x00 && ver != 0xFF);
}

void Bonezegei_RC522::reset() {
  writeRegister(RC522_CommandReg, PCD_RESETPHASE);
  delay(50);
}

void Bonezegei_RC522::antennaOn() {
  uint8_t temp = readRegister(RC522_TxControlReg);
  if (!(temp & 0x03)) {
    setBitMask(RC522_TxControlReg, 0x03);
  }
}

void Bonezegei_RC522::antennaOff() {
  clearBitMask(RC522_TxControlReg, 0x03);
}

void Bonezegei_RC522::writeRegister(uint8_t reg, uint8_t value) {
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  digitalWrite(_ssPin, LOW);
  SPI.transfer((reg << 1) & 0x7E);
  SPI.transfer(value);
  digitalWrite(_ssPin, HIGH);
  SPI.endTransaction();
}

uint8_t Bonezegei_RC522::readRegister(uint8_t reg) {
  uint8_t value;
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  digitalWrite(_ssPin, LOW);
  SPI.transfer(((reg << 1) & 0x7E) | 0x80);
  value = SPI.transfer(0x00);
  digitalWrite(_ssPin, HIGH);
  SPI.endTransaction();
  return value;
}

void Bonezegei_RC522::setBitMask(uint8_t reg, uint8_t mask) {
  uint8_t current = readRegister(reg);
  writeRegister(reg, current | mask);
}

void Bonezegei_RC522::clearBitMask(uint8_t reg, uint8_t mask) {
  uint8_t current = readRegister(reg);
  writeRegister(reg, current & (~mask));
}

void Bonezegei_RC522::calculateCRC(uint8_t *data, uint8_t len, uint8_t *result) {
  writeRegister(RC522_CommandReg, PCD_IDLE);
  clearBitMask(RC522_DivIrqReg, 0x04);
  setBitMask(RC522_FIFOLevelReg, 0x80);

  for (uint8_t i = 0; i < len; i++) {
    writeRegister(RC522_FIFODataReg, data[i]);
  }

  writeRegister(RC522_CommandReg, PCD_CALCCRC);

  uint16_t i = 0xFFF;
  uint8_t n;
  do {
    n = readRegister(RC522_DivIrqReg);
    i--;
  } while ((i != 0) && !(n & 0x04));

  result[0] = readRegister(RC522_CRCResultRegL);
  result[1] = readRegister(RC522_CRCResultRegH);
}

bool Bonezegei_RC522::isCard() {
  uint8_t tagType[2];
  return (cardRequest(PICC_REQIDL, tagType) == MI_OK);
}

bool Bonezegei_RC522::readCard() {
  uint8_t status;
  uint8_t tagType[2];

  status = cardRequest(PICC_REQIDL, tagType);
  if (status != MI_OK) return false;

  status = anticoll(_uid);
  if (status == MI_OK) {
    _uidSize = 4;
    return true;
  }

  _uidSize = 0;
  return false;
}

uint8_t Bonezegei_RC522::getUIDSize() {
  return _uidSize;
}

void Bonezegei_RC522::getUID(uint8_t *buffer) {
  for (uint8_t i = 0; i < _uidSize; i++) {
    buffer[i] = _uid[i];
  }
}

String Bonezegei_RC522::getUIDString() {
  String str = "";
  for (uint8_t i = 0; i < _uidSize; i++) {
    if (_uid[i] < 0x10) str += "0";
    str += String(_uid[i], HEX);
    if (i < _uidSize - 1) str += ":";
  }
  str.toUpperCase();
  return str;
}

bool Bonezegei_RC522::authenticate(uint8_t authMode, uint8_t blockAddr, uint8_t *key, uint8_t *uid) {
  uint8_t sendData[12];
  sendData[0] = authMode;
  sendData[1] = blockAddr;
  for (uint8_t i = 0; i < 6; i++) sendData[2 + i] = key[i];
  for (uint8_t i = 0; i < 4; i++) sendData[8 + i] = uid[i];

  uint8_t backData[16];
  uint16_t backLen = 0;

  uint8_t status = toCard(PCD_AUTHENT, sendData, 12, backData, &backLen);
  return (status == MI_OK && (readRegister(RC522_Status2Reg) & 0x08));
}

bool Bonezegei_RC522::readBlock(uint8_t blockAddr, uint8_t *buffer) {
  uint8_t sendData[4];
  sendData[0] = PICC_READ;
  sendData[1] = blockAddr;
  calculateCRC(sendData, 2, &sendData[2]);

  uint16_t backLen = 0;
  uint8_t status = toCard(PCD_TRANSCEIVE, sendData, 4, buffer, &backLen);
  return (status == MI_OK && backLen == 144);
}

bool Bonezegei_RC522::writeBlock(uint8_t blockAddr, uint8_t *buffer) {
  uint8_t cmdData[4];
  cmdData[0] = PICC_WRITE;
  cmdData[1] = blockAddr;
  calculateCRC(cmdData, 2, &cmdData[2]);

  uint8_t backData[16];
  uint16_t backLen = 0;
  uint8_t status = toCard(PCD_TRANSCEIVE, cmdData, 4, backData, &backLen);

  if ((status != MI_OK) || (backLen != 4) || ((backData[0] & 0x0F) != 0x0A)) {
    return false;
  }

  uint8_t sendData[18];
  for (uint8_t i = 0; i < 16; i++) sendData[i] = buffer[i];
  calculateCRC(sendData, 16, &sendData[16]);

  status = toCard(PCD_TRANSCEIVE, sendData, 18, backData, &backLen);
  return (status == MI_OK && backLen == 4 && (backData[0] & 0x0F) == 0x0A);
}

void Bonezegei_RC522::stopCrypto() {
  clearBitMask(RC522_Status2Reg, 0x08);
}

void Bonezegei_RC522::halt() {
  uint8_t sendData[4];
  sendData[0] = PICC_HALT;
  sendData[1] = 0x00;
  calculateCRC(sendData, 2, &sendData[2]);

  uint8_t backData[16];
  uint16_t backLen = 0;
  toCard(PCD_TRANSCEIVE, sendData, 4, backData, &backLen);
}

uint8_t Bonezegei_RC522::cardRequest(uint8_t reqMode, uint8_t *tagType) {
  uint8_t status;
  uint16_t backBits;

  writeRegister(RC522_BitFramingReg, 0x07);
  tagType[0] = reqMode;

  status = toCard(PCD_TRANSCEIVE, tagType, 1, tagType, &backBits);
  if ((status != MI_OK) || (backBits != 0x10)) {
    status = MI_ERR;
  }
  return status;
}

uint8_t Bonezegei_RC522::anticoll(uint8_t *serNum) {
  uint8_t status;
  uint8_t i;
  uint8_t serNumCheck = 0;
  uint16_t unLen;

  writeRegister(RC522_BitFramingReg, 0x00);
  serNum[0] = PICC_ANTICOLL;
  serNum[1] = 0x20;

  status = toCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

  if (status == MI_OK) {
    for (i = 0; i < 4; i++) serNumCheck ^= serNum[i];
    if (serNumCheck != serNum[i]) status = MI_ERR;
  }
  return status;
}

uint8_t Bonezegei_RC522::toCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint16_t *backLen) {
  uint8_t status = MI_ERR;
  uint8_t irqEn = 0x00;
  uint8_t waitIRq = 0x00;
  uint8_t lastBits;
  uint8_t n;
  uint16_t i;

  if (command == PCD_TRANSCEIVE) {
    irqEn = 0x77;
    waitIRq = 0x30;
  } else if (command == PCD_AUTHENT) {
    irqEn = 0x12;
    waitIRq = 0x10;
  }

  writeRegister(RC522_ComIEnReg, irqEn | 0x80);
  clearBitMask(RC522_ComIrqReg, 0x80);
  setBitMask(RC522_FIFOLevelReg, 0x80);

  writeRegister(RC522_CommandReg, PCD_IDLE);

  for (i = 0; i < sendLen; i++) {
    writeRegister(RC522_FIFODataReg, sendData[i]);
  }

  writeRegister(RC522_CommandReg, command);
  if (command == PCD_TRANSCEIVE) {
    setBitMask(RC522_BitFramingReg, 0x80);
  }

  i = 2000;
  do {
    n = readRegister(RC522_ComIrqReg);
    i--;
  } while ((i != 0) && !(n & 0x01) && !(n & waitIRq));

  clearBitMask(RC522_BitFramingReg, 0x80);

  if (i != 0) {
    if (!(readRegister(RC522_ErrorReg) & 0x1B)) {
      status = MI_OK;
      if (n & irqEn & 0x01) status = MI_NOTAGERR;

      if (command == PCD_TRANSCEIVE) {
        n = readRegister(RC522_FIFOLevelReg);
        lastBits = readRegister(RC522_ControlReg) & 0x07;
        if (lastBits) *backLen = (n - 1) * 8 + lastBits;
        else *backLen = n * 8;

        if (n == 0) n = 1;
        if (n > 16) n = 16;

        for (i = 0; i < n; i++) backData[i] = readRegister(RC522_FIFODataReg);
      }
    } else {
      status = MI_ERR;
    }
  }

  return status;
}
