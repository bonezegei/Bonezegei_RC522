/*
  This Library is written for Bonezegei RC522
  Author: Bonezegei (Jofel Batutay)
  Date: October 2023 
*/

#ifndef _BONEZEGEI_RC522_H_
#define _BONEZEGEI_RC522_H_
#include <Arduino.h
#include <SPI.h>

static const int RC522_SPISPEED = 1000000;  // 80 MHz
class Bonezegei_RC522 {
public:
  Bonezegei_RC522(int ss, int rst);

  void begin();

  char _ss, _rst;
  SPIClass *vspi = NULL;
};

#endif
