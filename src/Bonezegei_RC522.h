/*
    MFRC522 RFID Reader/Writer with MIFARE Classic
    Author: Jofel Batutay (Bonezegei)
    Date: October 14, 2026
*/

#ifndef BONEZEGEI_RC522_H
#define BONEZEGEI_RC522_H

#include <Arduino.h>
#include <SPI.h>

// MFRC522 Registers
#define RC522_CommandReg       0x01
#define RC522_ComIEnReg        0x02
#define RC522_DivIEnReg        0x03
#define RC522_ComIrqReg        0x04
#define RC522_DivIrqReg        0x05
#define RC522_ErrorReg         0x06
#define RC522_Status1Reg       0x07
#define RC522_Status2Reg       0x08
#define RC522_FIFODataReg      0x09
#define RC522_FIFOLevelReg     0x0A
#define RC522_WaterLevelReg    0x0B
#define RC522_ControlReg       0x0C
#define RC522_BitFramingReg    0x0D
#define RC522_CollReg          0x0E
#define RC522_ModeReg          0x11
#define RC522_TxModeReg        0x12
#define RC522_RxModeReg        0x13
#define RC522_TxControlReg     0x14
#define RC522_TxASKReg         0x15
#define RC522_CRCResultRegH    0x21
#define RC522_CRCResultRegL    0x22
#define RC522_RFCfgReg         0x26
#define RC522_TModeReg         0x2A
#define RC522_TPrescalerReg    0x2B
#define RC522_TReloadRegH      0x2C
#define RC522_TReloadRegL      0x2D
#define RC522_VersionReg       0x37

// PCD Commands
#define PCD_IDLE              0x00
#define PCD_CALCCRC           0x03
#define PCD_TRANSCEIVE        0x0C
#define PCD_AUTHENT           0x0E
#define PCD_RESETPHASE        0x0F

// PICC Commands
#define PICC_REQIDL           0x26
#define PICC_ANTICOLL         0x93
#define PICC_AUTHENT1A        0x60
#define PICC_AUTHENT1B        0x61
#define PICC_READ             0x30
#define PICC_WRITE            0xA0
#define PICC_HALT             0x50

// Status Codes
#define MI_OK                 0
#define MI_NOTAGERR           1
#define MI_ERR                2

class Bonezegei_RC522 {
  public:
    Bonezegei_RC522(uint8_t ssPin, uint8_t rstPin);

    bool begin();
    void reset();
    void antennaOn();
    void antennaOff();

    bool isCard();
    bool readCard();

    uint8_t getUIDSize();
    void getUID(uint8_t *buffer);
    String getUIDString();

    // MIFARE Data & Auth Functions
    bool authenticate(uint8_t authMode, uint8_t blockAddr, uint8_t *key, uint8_t *uid);
    bool readBlock(uint8_t blockAddr, uint8_t *buffer);
    bool writeBlock(uint8_t blockAddr, uint8_t *buffer);
    void stopCrypto();
    void halt();

    // Low Level SPI Register Operations
    void writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    void setBitMask(uint8_t reg, uint8_t mask);
    void clearBitMask(uint8_t reg, uint8_t mask);
    void calculateCRC(uint8_t *data, uint8_t len, uint8_t *result);

  private:
    uint8_t _ssPin;
    uint8_t _rstPin;
    uint8_t _uid[10];
    uint8_t _uidSize;

    uint8_t cardRequest(uint8_t reqMode, uint8_t *tagType);
    uint8_t anticoll(uint8_t *serNum);
    uint8_t toCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint16_t *backLen);
};

#endif