/*
   Copyright (c) 2021 Aaron Christophel ATCnetz.de
   SPDX-License-Identifier: GPL-3.0-or-later
*/
#include <Arduino.h>
#include "defines.h"
#include "swd.h"
#include "nrf_swd.h"
#include "glitcher.h"

#define delay_inc_step 1
#define width_inc_step 1

#define delay_max 3000
#define width_max 30

#define delay_start 2000
#define width_start 0


uint32_t _delay_us = 0;
uint32_t _width = 0;

bool glitcher_enabled = false;

void glitcher_begin() {
  _delay_us = delay_start;
  _width = width_start;
  pinMode(LED, OUTPUT);
  pinMode(GLITCHER, OUTPUT);
  pinMode(NRF_POWER, OUTPUT);
  digitalWrite(GLITCHER, LOW);
  set_power(HIGH);
}

void set_glitcher(bool new_state) {
  glitcher_enabled = new_state;
}

bool get_glitcher() {
  return glitcher_enabled;
}

void set_power(bool state) {
  digitalWrite(NRF_POWER, state);
  digitalWrite(LED, state);
}

void do_glitcher() {
  Serial.println("Next glitch");
  Serial.println("Delay: " + String(get_delay()) + " Width: " + String(get_width()));

  set_power(LOW);
  delay(50);
  set_power(HIGH);
  delayMicroseconds(get_delay());
  digitalWrite(GLITCHER, HIGH);
  delayMicroseconds(get_width());
  digitalWrite(GLITCHER, LOW);

  if (inc_width()) {
    inc_delay();
  }

  delay(100);
  Serial.printf("SWD Id: 0x%08x\r\n", nrf_begin(true));
  if (nrf_read_lock_state() == 1) {
    Serial.println("We Have a good glitch");
    glitcher_enabled = false;
    Serial.printf("SWD Id: 0x%08x\r\n", nrf_begin());
  }
}

void set_delay(uint32_t delay_us) {
  _delay_us = delay_us;
}

uint32_t get_delay() {
  return _delay_us;
}

bool inc_delay() {
  _delay_us += delay_inc_step;
  if (_delay_us > delay_max) {
    _delay_us = delay_start;
    return true;
  }
  return false;
}

void set_width(uint32_t width) {
  _width = width;
}

uint32_t get_width() {
  return _width;
}

bool inc_width() {
  _width += width_inc_step;
  if (_width > width_max) {
    _width = width_start;
    return true;
  }
  return false;
}
