/*
   Copyright (c) 2021 Aaron Christophel ATCnetz.de
   SPDX-License-Identifier: GPL-3.0-or-later
*/

#ifdef ENABLE_OTA
#include <ArduinoOTA.h>
#endif

#include "wifimanager.h"
#include "web.h"
#include "glitcher.h"
#include "nrf_swd.h"
#include "swd.h"

void setup()
{
  Serial.begin(115200);
  delay(2000);
  swd_begin();
  glitcher_begin();
  init_wifimanager();
  init_web();
  Serial.printf("SWD Id: 0x%08x\r\n", nrf_begin());

#ifdef ENABLE_OTA
  ArduinoOTA.begin();
#endif
}

void loop()
{
#ifdef ENABLE_OTA
  ArduinoOTA.handle();
#endif
  if (get_glitcher())
  {
    do_glitcher();
  }
  else
  {
    do_nrf_swd();
  }
}
