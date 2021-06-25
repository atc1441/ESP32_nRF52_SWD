/*
   Copyright (c) 2021 Aaron Christophel ATCnetz.de
   SPDX-License-Identifier: GPL-3.0-or-later
*/
#include "Arduino.h"
#include "swd.h"
#include "defines.h"

// Many thanks to scanlime for the work on the ESP8266 SWD Library, parts of this code have inspiration and help from it
// https://github.com/scanlime/esp8266-arm-swd

bool turn_state = 0;

void swd_begin() {
  pinMode(swd_clock_pin, OUTPUT);
  pinMode(swd_data_pin, INPUT_PULLUP);
}

uint32_t swd_init() { //Returns the ID
  swd_write(0xffffffff, 32);
  swd_write(0xffffffff, 32);
  swd_write(0xe79e, 16);
  swd_write(0xffffffff, 32);
  swd_write(0xffffffff, 32);
  swd_write(0, 32);
  swd_write(0, 32);

  uint32_t idcode;
  DP_Read(DP_IDCODE, idcode);
  return idcode;
}


bool AP_Write(unsigned addr, uint32_t data)
{
  uint8_t retry = 15;
  while (retry--) {
    bool state = swd_transfer(addr, 1, 0, data);
    if (state)return true;
  }
  return false;
}

bool AP_Read(unsigned addr, uint32_t &data)
{
  uint8_t retry = 15;
  while (retry--) {
    bool state = swd_transfer(addr, 1, 1, data);
    if (state)return true;
  }
  return false;
}

bool DP_Write(unsigned addr, uint32_t data)
{
  uint8_t retry = 15;
  while (retry--) {
    bool state = swd_transfer(addr, 0, 0, data);
    if (state)return true;
  }
  return false;
}

bool DP_Read(unsigned addr, uint32_t &data)
{
  uint8_t retry = 15;
  while (retry--) {
    bool state = swd_transfer(addr, 0, 1, data);
    if (state)return true;
  }
  return false;
}

bool swd_transfer(unsigned port_address, bool APorDP, bool RorW, uint32_t &data)
{
  bool parity = APorDP ^ RorW ^ ((port_address >> 2) & 1) ^ ((port_address >> 3) & 1);
  uint8_t filled_address = (1 << 0) | (APorDP << 1) | (RorW << 2) | ((port_address & 0xC) << 1) | (parity << 5) | (1 << 7);
  swd_write(filled_address, 8);
  if (swd_read(3) == 1) {
    if (RorW) {// Reading 32 bits from SWD
      data = swd_read(32);
      if (swd_read(1) == calculate_parity(data)) {
        swd_write(0, 1);
        return true;
      }
    } else {// Writing 32bits to SWD
      swd_write(data, 32);
      swd_write(calculate_parity(data), 1);
      swd_write(0, 1);
      return true;
    }
  }
  swd_write(0, 32);
  return false;
}

bool calculate_parity(uint32_t in_data)
{
  in_data = (in_data & 0xFFFF) ^ (in_data >> 16);
  in_data = (in_data & 0xFF) ^ (in_data >> 8);
  in_data = (in_data & 0xF) ^ (in_data >> 4);
  in_data = (in_data & 0x3) ^ (in_data >> 2);
  in_data = (in_data & 0x1) ^ (in_data >> 1);
  return in_data;
}

void swd_write(uint32_t in_data, uint8_t bits) {
  if (turn_state == 0)swd_turn(1);
  while (bits--) {
    digitalWrite(swd_data_pin, in_data & 1);
    digitalWrite(swd_clock_pin, LOW);
    delayMicroseconds(2);
    in_data >>= 1;
    digitalWrite(swd_clock_pin, HIGH);
    delayMicroseconds(2);
  }
}

uint32_t swd_read(uint8_t bits) {
  uint32_t out_data = 0;
  uint32_t input_bit = 1;
  if (turn_state == 1)swd_turn(0);
  while (bits--) {
    if (digitalRead(swd_data_pin)) {
      out_data |= input_bit;
    }
    digitalWrite(swd_clock_pin, LOW);
    delayMicroseconds(2);
    input_bit <<= 1;
    digitalWrite(swd_clock_pin, HIGH);
    delayMicroseconds(2);
  }
  return out_data;
}

void swd_turn(bool WorR) {//1 = Write 0 = Read
  digitalWrite(swd_data_pin, HIGH);
  pinMode(swd_data_pin, INPUT_PULLUP);
  digitalWrite(swd_clock_pin, LOW);
  delayMicroseconds(2);
  digitalWrite(swd_clock_pin, HIGH);
  delayMicroseconds(2);
  if (WorR)
    pinMode(swd_data_pin, OUTPUT);
  turn_state = WorR;
}
