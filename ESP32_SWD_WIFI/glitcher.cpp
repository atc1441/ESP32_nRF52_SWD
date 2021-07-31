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

uint32_t delay_max = 30000;
uint32_t width_max = 30;

uint32_t delay_start = 2000;
uint32_t width_start = 0;

uint32_t _delay_us = 0;
uint32_t _delay_us_end = 0;
uint32_t _width = 0;

uint32_t _power_off_delay = 50;
uint32_t _swd_wait_delay = 100;

bool glitcher_enabled = false;

void glitcher_begin()
{
  _delay_us = delay_start;
  _delay_us_end = delay_max;
  _width = width_start;
  pinMode(LED, OUTPUT);
  pinMode(GLITCHER, OUTPUT);
  pinMode(NRF_POWER, OUTPUT);
  digitalWrite(GLITCHER, LOW);
  set_power(HIGH);
}

void set_glitcher(bool new_state)
{
  glitcher_enabled = new_state;
}

bool get_glitcher()
{
  return glitcher_enabled;
}

void set_power(bool state)
{
  digitalWrite(NRF_POWER, state);
  digitalWrite(LED, state);
}

void do_glitcher()
{
  Serial.println("Next glitch");
  Serial.println("Delay: " + String(get_delay()) + " Width: " + String(get_width()));

  digitalWrite(swd_clock_pin, LOW);
  set_power(LOW);
  delay(_power_off_delay);
  set_power(HIGH);
  delayMicroseconds(get_delay());
  digitalWrite(GLITCHER, HIGH);
  delayMicroseconds(get_width());
  digitalWrite(GLITCHER, LOW);

  if (inc_width())
  {
    inc_delay();
  }

  delay(_swd_wait_delay);
  Serial.printf("SWD Id: 0x%08x\r\n", nrf_begin(true));
  uint32_t variant_read = read_register(0x10000100);
  if (variant_read == 0x00052832 || variant_read == 0x00052840 || nrf_read_lock_state() == 1)
  {
    Serial.println("We Have a good glitch");
    glitcher_enabled = false;
    Serial.printf("SWD Id: 0x%08x\r\n", nrf_begin());
  }
}

void set_delay(uint32_t delay_us, uint32_t delay_us_end, uint32_t power_off_delay, uint32_t swd_wait_delay)
{
  _delay_us = delay_us;
  delay_start = _delay_us;

  _delay_us_end = delay_us_end;
  delay_max = _delay_us_end;

  _power_off_delay = power_off_delay;
  _swd_wait_delay = swd_wait_delay;
}

uint32_t get_delay()
{
  return _delay_us;
}

bool inc_delay()
{
  _delay_us += delay_inc_step;
  if (_delay_us > _delay_us_end)
  {
    _delay_us = delay_start;
    return true;
  }
  return false;
}

void set_width(uint32_t width)
{
  _width = width;
}

uint32_t get_width()
{
  return _width;
}

bool inc_width()
{
  _width += width_inc_step;
  if (_width > width_max)
  {
    _width = width_start;
    return true;
  }
  return false;
}

void get_osci_graph(uint16_t graph_buff[], uint32_t size, uint32_t delay_time)
{
  digitalWrite(swd_clock_pin, LOW);
  set_power(LOW);
  delay(_power_off_delay);
  set_power(HIGH);
  long start_micro = micros();
  bool has_fired = 0;
  for (int i = 0; i < size; i++)
  {
    if (!has_fired && micros() - start_micro > delay_time)
    {
      has_fired = 1;
      digitalWrite(GLITCHER, HIGH);
      graph_buff[i++] = analogRead(OSCI_PIN);
      digitalWrite(GLITCHER, LOW);
    }
    graph_buff[i] = analogRead(OSCI_PIN);
  }
}
