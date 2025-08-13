#include "turboSPOT.h"
#include <Arduino.h>


bool REQUEST_TURBO_SPOT() {
  bool ack = false;
  AM_REGVAL(0x40004034) |= 1;
  while(1) {
    if ((AM_REGVAL(0x40004034) & 0b010) == 0b010) {
      ack = true;
      break;
    } else if ((AM_REGVAL(0x40004034) & 0b010) == 0b000) {
      break;
    }
  }
  return ack;
}

bool STOP_TURBO_SPOT() {
  AM_REGVAL(0x40004034) = 0;
  return false;
}