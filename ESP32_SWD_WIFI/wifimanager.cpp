/*
   Copyright (c) 2024 Henk.Vergonet @ gmail.com
   SPDX-License-Identifier: GPL-3.0-or-later
*/
#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager/
#include "../secrets.h"  // import for AP password

#include "wifimanager.h"

void init_wifimanager()
{
#ifdef TX_POWER_FIX
  // see https://github.com/luc-github/ESP3D/issues/1009
  int txPower = WiFi.getTxPower();
  WiFi.setTxPower(WIFI_POWER_8_5dBm);

  Serial.print("TX power: ");
  Serial.print(txPower);
  Serial.print(" -> ");
  Serial.println(WiFi.getTxPower());
#endif

  Serial.print("FallbackAP SSID:AutoConnectAP PWD:");
  Serial.println(AP_PASSWORD);

  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  bool res;
  res = wm.autoConnect("AutoConnectAP", AP_PASSWORD);
  if (!res)
    ESP.restart();

#ifdef TX_POWER_FIX
  WiFi.setTxPower(WIFI_POWER_8_5dBm);
#endif
}