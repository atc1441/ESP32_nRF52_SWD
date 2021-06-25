/*
   Copyright (c) 2021 Aaron Christophel ATCnetz.de
   SPDX-License-Identifier: GPL-3.0-or-later
*/
#include <Arduino.h>
#include "web.h"
#include <FS.h>
#include "SPIFFS.h"
#include <ESPmDNS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager/tree/feature_asyncwebserver

#include "nrf_swd.h"
#include "glitcher.h"
#include "defines.h"


const char *http_username = "admin";
const char *http_password = "admin";
AsyncWebServer server(80);

unsigned long hstol(String recv) {
  char c[recv.length() + 1];
  recv.toCharArray(c, recv.length() + 1);
  return strtoul(c, NULL, 16);
}

void init_web()
{
  WiFi.mode(WIFI_STA);
  WiFiManager wm;
  bool res;
  res = wm.autoConnect("AutoConnectAP");
  if (!res)
  {
    Serial.println("Failed to connect");
    ESP.restart();
  }
  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  // Make accessible via http://swd.local using mDNS responder
  if (!MDNS.begin("swd"))
  {
    while (1) {
      Serial.println("Error setting up mDNS responder!");
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  MDNS.addService("http", "tcp", 80);
  SPIFFS.begin(true);

  server.addHandler(new SPIFFSEditor(SPIFFS, http_username, http_password));

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", String(ESP.getFreeHeap()));
  });

  server.on("/get_state", HTTP_GET, [](AsyncWebServerRequest * request) {
    String new_cmd = "";
    if (request->hasParam("cmd"))
    {
      new_cmd = request->getParam("cmd")->value();
    }
    String answer_state = "";
    uint8_t percent = 0;
    if (new_cmd == "1")
      set_new_main_info(false);
    else if (new_cmd == "2")
      set_new_main_info(true);
    if (get_new_main_info()) {
      nrf_info_struct nrf_ufcr;
      get_new_main_info(&nrf_ufcr);
      answer_state += ";info;";
      answer_state += String(nrf_ufcr.connected) + ";";
      answer_state += String(nrf_ufcr.flash_size) + ";";
      answer_state += String(nrf_ufcr.config_id) + ";";
      answer_state += String(nrf_ufcr.device_id0) + ";";
      answer_state += String(nrf_ufcr.device_id1) + ";";
      answer_state += String(nrf_ufcr.info_part) + ";";
      answer_state += String(nrf_ufcr.info_variant) + ";";
      answer_state += String(nrf_ufcr.info_package) + ";";
      answer_state += String(SPIFFS.totalBytes()) + ";";
      answer_state += String(SPIFFS.usedBytes()) + ";";
      answer_state += String(SPIFFS.totalBytes() - SPIFFS.usedBytes()) + ";";
      answer_state += String(nrf_ufcr.sd_info_area) + ";";
      answer_state += String(nrf_ufcr.ucir_lock) + ";";
    } else if (get_glitcher()) {
      answer_state += "Glitcher running";
      answer_state += " Delay: " + String(get_delay());
      answer_state += " Width: " + String(get_width());
    } else if (get_task_flash(&percent)) {
      answer_state += "Flash state ";
      answer_state += String(percent) + "%";
    } else {
      answer_state += "no task running, last speed " + String(get_last_speed()) + "kbps";
    }
    request->send(200, "text/plain", answer_state);
  });

  server.on("/set_delay", HTTP_POST, [](AsyncWebServerRequest * request) {
    if (request->hasParam("delay"))
    {
      int new_delay = request->getParam("delay")->value().toInt();
      request->send(200, "text/plain", "Ok set delay: " + String(new_delay));
      set_delay(new_delay);
      return;
    }
    request->send(200, "text/plain", "Wrong parameter");
  });

  server.on("/set_swd", HTTP_POST, [](AsyncWebServerRequest * request) {

    if (get_glitcher()) {
      request->send(200, "text/plain", "ERROR Glitcher is running");
      return;
    }

    if (request->hasParam("cmd"))
    {
      String swd_cmd = request->getParam("cmd")->value();
      String answer = "";

      if (swd_cmd == "init")
      {
        char init_string[100] = {0};
        sprintf(init_string, "Init of SWD ID: 0x%08x", nrf_begin());
        answer = init_string;
      }
      else if (swd_cmd == "power_on")
      {
        set_power(HIGH);
        answer = "Power on";
      }
      else if (swd_cmd == "power_off")
      {
        set_power(LOW);
        answer = "Power off";
      }
      else {

        if (is_nrf_connected() == 0) {
          request->send(200, "text/plain", "ERROR nRF not connected");
          return;
        }

        if (swd_cmd == "set_reset")
        {
          nrf_soft_reset();
          answer = "Ok reset";
        }
        else if (swd_cmd == "erase_all")
        {
          nrf_erase_all();
          answer = "nRF erased";
        } else if (is_nrf_connected() == 1) {
          request->send(200, "text/plain", "ERROR nRF is locked");
          return;
        } else if (swd_cmd == "lock_state")
        {
          answer = "the nRF is " + String(nrf_read_lock_state() ? "unlocked" : "locked");
        }
        else if (swd_cmd == "set_lock")
        {
          write_flash(0x10001208, 0x00);
          nrf_soft_reset();
          nrf_begin();
          answer = "the nRF is now " + String(nrf_read_lock_state() ? "unlocked" : "locked");
        }
        else if (swd_cmd == "read_register")
        {
          String read_address = "";
          if (request->hasParam("address"))
            read_address = request->getParam("address")->value();
          char read_flash_string[100] = {0};
          sprintf(read_flash_string, "Register read address: 0x%08x value: 0x%08x", hstol(read_address), read_register(hstol(read_address)));
          answer = read_flash_string;
        }
        else if (swd_cmd == "write_register")
        {
          String write_address = "";
          String write_value = "";
          if (request->hasParam("address") && request->hasParam("value")) {
            write_address = request->getParam("address")->value();
            write_value = request->getParam("value")->value();
          } else {
            request->send(200, "text/plain", "Wrong parameter");
            return;
          }
          write_register(hstol(write_address), hstol(write_value));
          answer = "Register write address: 0x" + String(write_address) + " value: 0x" + String(write_value);
        }
        else if (swd_cmd == "write_flash")
        {
          String write_address = "";
          String write_value = "";
          if (request->hasParam("address") && request->hasParam("value")) {
            write_address = request->getParam("address")->value();
            write_value = request->getParam("value")->value();
          } else {
            request->send(200, "text/plain", "Wrong parameter");
            return;
          }
          write_flash(hstol(write_address), hstol(write_value));
          answer = "Flash write address: 0x" + String(write_address) + " value: 0x" + String(write_value);
        }
        else
        {
          request->send(200, "text/plain", "Wrong parameter");
          return;
        }
      }
      request->send(200, "text/plain", "Ok: " + String(answer));
      return;
    }
    request->send(200, "text/plain", "Wrong parameter");
  });

  server.on("/flash_cmd", HTTP_POST, [](AsyncWebServerRequest * request) {

    if (get_glitcher()) {
      request->send(200, "text/plain", "ERROR Glitcher is running");
      return;
    }
    if (is_nrf_connected() == 0) {
      request->send(200, "text/plain", "ERROR nRF not connected");
      return;
    } else if (is_nrf_connected() == 1) {
      request->send(200, "text/plain", "ERROR nRF is locked");
      return;
    }

    if (request->hasParam("cmd"))
    {
      String swd_cmd = request->getParam("cmd")->value();
      String answer = "";

      if (swd_cmd == "erase_all")
      {
        erase_flash();
        answer = "Everything erased";
      }
      else if (swd_cmd == "page_erase")
      {
        String write_address = "";
        if (request->hasParam("address")) {
          write_address = request->getParam("address")->value();
        } else {
          request->send(200, "text/plain", "Wrong parameter");
          return;
        }
        erase_page(hstol(write_address));
        answer = "Page erased: 0x" + String(write_address);
      }
      else if (swd_cmd == "flash_file")
      {
        String filename = "";
        uint32_t offset = 0;
        if (request->hasParam("file")) {
          filename = request->getParam("file")->value();
        } else {
          request->send(200, "text/plain", "Wrong parameter");
          return;
        }
        if (!SPIFFS.exists("/" + filename))
        {
          request->send(200, "text/plain", "Error opening file");
          return;
        }

        if (request->hasParam("offset")) {
          offset = hstol(request->getParam("offset")->value());
        }

        set_write_flash(offset, "/" + filename);
        answer = "file flash task created";
      }
      else if (swd_cmd == "dump_flash")
      {
        String filename = "";
        uint32_t offset = 0;
        uint32_t size = 0;
        if (request->hasParam("file") && request->hasParam("offset") && request->hasParam("size")) {
          filename = request->getParam("file")->value();
          offset = hstol(request->getParam("offset")->value());
          size = hstol(request->getParam("size")->value());
        } else {
          request->send(200, "text/plain", "Wrong parameter");
          return;
        }
        if (size > (SPIFFS.totalBytes() - SPIFFS.usedBytes())) {
          request->send(200, "text/plain", "Not enough free space on the ESP32");
          return;
        }
        set_read_flash(offset, size, "/" + filename);
        answer = "file read task created";
      }
      else
      {
        request->send(200, "text/plain", "Wrong parameter");
        return;
      }
      request->send(200, "text/plain", "Ok: " + String(answer));
      return;
    }
    request->send(200, "text/plain", "Wrong parameter");
  });

  server.on("/set_glitcher", HTTP_POST, [](AsyncWebServerRequest * request) {
    if (request->hasParam("state"))
    {
      String new_state = request->getParam("state")->value();
      if (new_state == "1") {
        request->send(200, "text/plain", "Ok set glitcher: 1");
        set_glitcher(1);
      } else if (new_state == "0") {
        request->send(200, "text/plain", "Ok set glitcher: 0");
        set_glitcher(0);
      } else if (new_state == "dump_full_flash") {

        if (get_glitcher()) {
          request->send(200, "text/plain", "ERROR Glitcher is running");
          return;
        }
        if (is_nrf_connected() == 0) {
          request->send(200, "text/plain", "ERROR nRF not connected");
          return;
        } else if (is_nrf_connected() == 1) {
          request->send(200, "text/plain", "ERROR nRF is locked");
          return;
        }

        nrf_info_struct nrf_ufcr;
        get_new_main_info(&nrf_ufcr);

        if (nrf_ufcr.flash_size > (SPIFFS.totalBytes() - SPIFFS.usedBytes())) {
          request->send(200, "text/plain", "Not enough free space on the ESP32");
          return;
        }

        String filename = "/full_flash.bin";
        set_read_flash(0, nrf_ufcr.flash_size, filename);

        request->send(200, "text/plain", "Ok create task full dump");
      } else if (new_state == "dump_full_uicr") {

        if (get_glitcher()) {
          request->send(200, "text/plain", "ERROR Glitcher is running");
          return;
        }
        if (is_nrf_connected() == 0) {
          request->send(200, "text/plain", "ERROR nRF not connected");
          return;
        } else if (is_nrf_connected() == 1) {
          request->send(200, "text/plain", "ERROR nRF is locked");
          return;
        }

        if (0x1000 > (SPIFFS.totalBytes() - SPIFFS.usedBytes())) {
          request->send(200, "text/plain", "Not enough free space on the ESP32");
          return;
        }
        String filename = "/full_uicr.bin";
        set_read_flash(0x10001000, 0x1000, filename);

        request->send(200, "text/plain", "Ok create UICR dump");
      }
      return;
    }
    request->send(200, "text/plain", "Wrong parameter");
  });

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");

  server.onNotFound([](AsyncWebServerRequest * request) {
    if (request->url() == "/" || request->url() == "index.htm")
    { // not uploaded the index.htm till now so notify the user about it
      request->send(200, "text/html", "please use <a href=\"/edit\">/edit</a> with login defined in web.cpp to uplaod the supplied index.htm to get full useage");
      return;
    }
    Serial.printf("NOT_FOUND: ");
    if (request->method() == HTTP_GET)
      Serial.printf("GET");
    else if (request->method() == HTTP_POST)
      Serial.printf("POST");
    else if (request->method() == HTTP_DELETE)
      Serial.printf("DELETE");
    else if (request->method() == HTTP_PUT)
      Serial.printf("PUT");
    else if (request->method() == HTTP_PATCH)
      Serial.printf("PATCH");
    else if (request->method() == HTTP_HEAD)
      Serial.printf("HEAD");
    else if (request->method() == HTTP_OPTIONS)
      Serial.printf("OPTIONS");
    else
      Serial.printf("UNKNOWN");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());

    if (request->contentLength())
    {
      Serial.printf("_CONTENT_TYPE: %s\n", request->contentType().c_str());
      Serial.printf("_CONTENT_LENGTH: %u\n", request->contentLength());
    }
    int headers = request->headers();
    int i;
    for (i = 0; i < headers; i++)
    {
      AsyncWebHeader *h = request->getHeader(i);
      Serial.printf("_HEADER[%s]: %s\n", h->name().c_str(), h->value().c_str());
    }
    int params = request->params();
    for (i = 0; i < params; i++)
    {
      AsyncWebParameter *p = request->getParam(i);
      if (p->isFile())
      {
        Serial.printf("_FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
      }
      else if (p->isPost())
      {
        Serial.printf("_POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
      else
      {
        Serial.printf("_GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
      }
    }
    request->send(404);
  });

  server.begin();
}
