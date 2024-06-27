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
#include <LoopbackStream.h>

#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager/tree/feature_asyncwebserver

#include "nrf_swd.h"
#include "glitcher.h"
#include "defines.h"

const char *http_username = "admin";
const char *http_password = "admin";
AsyncWebServer server(80);

unsigned long hstol(String recv)
{
  char c[recv.length() + 1];
  recv.toCharArray(c, recv.length() + 1);
  return strtoul(c, NULL, 16);
}

byte nibble(char c)
{
  if (c >= '0' && c <= '9')
    return c - '0';

  if (c >= 'a' && c <= 'f')
    return c - 'a' + 10;

  if (c >= 'A' && c <= 'F')
    return c - 'A' + 10;

  return 0;
}

int decode_line(byte* buf, String line){
  int data_len = nibble(line[1])*16 + nibble(line[2]);
  int line_len = data_len + 5;

  if (line.length() < line_len*2 + 1){
    return -1;
  }

  for (int i = 0; i < line_len; i++){
    buf[i] = nibble(line[1+i*2])*16 + nibble(line[2+i*2]);
  }

  byte csum = 0;
  for (int i = 0; i < line_len; i++){
    csum += buf[i];
  }
  if (csum){ // should be 0
    return -2;
  }
  return 0;
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
    while (1)
    {
      Serial.println("Error setting up mDNS responder!");
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  MDNS.addService("http", "tcp", 80);
  SPIFFS.begin(true);

  server.addHandler(new SPIFFSEditor(SPIFFS, http_username, http_password));

  server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", String(ESP.getFreeHeap())); });

  server.on("/get_state", HTTP_GET, [](AsyncWebServerRequest *request)
            {
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
              if (get_new_main_info())
              {
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
              }
              else if (get_glitcher())
              {
                answer_state += "Glitcher running";
                answer_state += " Delay: " + String(get_delay());
                answer_state += " Width: " + String(get_width());
              }
              else if (get_task_flash(&percent))
              {
                answer_state += "Flash state ";
                answer_state += String(percent) + "%";
              }
              else
              {
                answer_state += "no task running, last speed " + String(get_last_speed()) + "kbps";
              }
              request->send(200, "text/plain", answer_state);
            });

  server.on("/set_delay", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              if (request->hasParam("delay") && request->hasParam("delay_end") && request->hasParam("power_off") && request->hasParam("swd_wait"))
              {
                int new_delay = request->getParam("delay")->value().toInt();
                int new_delay_end = request->getParam("delay_end")->value().toInt();
                int new_power_off = request->getParam("power_off")->value().toInt();
                int new_swd_wait = request->getParam("swd_wait")->value().toInt();
                request->send(200, "text/plain", "Ok set delay: " + String(new_delay) + " delay_end: " + String(new_delay_end));
                set_delay(new_delay, new_delay_end, new_power_off, new_swd_wait);
                return;
              }
              request->send(200, "text/plain", "Wrong parameter");
            });

  server.on("/set_swd", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              if (get_glitcher())
              {
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
                else
                {

                  if (is_nrf_connected() == 0)
                  {
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
                  }
                  else if (is_nrf_connected() == 1)
                  {
                    request->send(200, "text/plain", "ERROR nRF is locked");
                    return;
                  }
                  else if (swd_cmd == "lock_state")
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
                    sprintf(read_flash_string, "Register read address: 0x%08x value: 0x%08x", (unsigned int)hstol(read_address), read_register(hstol(read_address)));
                    answer = read_flash_string;
                  }
                  else if (swd_cmd == "write_register")
                  {
                    String write_address = "";
                    String write_value = "";
                    if (request->hasParam("address") && request->hasParam("value"))
                    {
                      write_address = request->getParam("address")->value();
                      write_value = request->getParam("value")->value();
                    }
                    else
                    {
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
                    if (request->hasParam("address") && request->hasParam("value"))
                    {
                      write_address = request->getParam("address")->value();
                      write_value = request->getParam("value")->value();
                    }
                    else
                    {
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

  server.on("/flash_cmd", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              if (get_glitcher())
              {
                request->send(200, "text/plain", "ERROR Glitcher is running");
                return;
              }
              if (is_nrf_connected() == 0)
              {
                request->send(200, "text/plain", "ERROR nRF not connected");
                return;
              }
              else if (is_nrf_connected() == 1)
              {
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
                  if (request->hasParam("address"))
                  {
                    write_address = request->getParam("address")->value();
                  }
                  else
                  {
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
                  if (request->hasParam("file"))
                  {
                    filename = request->getParam("file")->value();
                  }
                  else
                  {
                    request->send(200, "text/plain", "Wrong parameter");
                    return;
                  }
                  if (!SPIFFS.exists("/" + filename))
                  {
                    request->send(200, "text/plain", "Error opening file");
                    return;
                  }

                  if (request->hasParam("offset"))
                  {
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
                  if (request->hasParam("file") && request->hasParam("offset") && request->hasParam("size"))
                  {
                    filename = request->getParam("file")->value();
                    offset = hstol(request->getParam("offset")->value());
                    size = hstol(request->getParam("size")->value());
                  }
                  else
                  {
                    request->send(200, "text/plain", "Wrong parameter");
                    return;
                  }
                  if (size > (SPIFFS.totalBytes() - SPIFFS.usedBytes()))
                  {
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

  server.on("/set_glitcher", HTTP_POST, [](AsyncWebServerRequest *request)
            {
              if (request->hasParam("state"))
              {
                String new_state = request->getParam("state")->value();
                if (new_state == "1")
                {
                  request->send(200, "text/plain", "Ok set glitcher: 1");
                  set_glitcher(1);
                }
                else if (new_state == "0")
                {
                  request->send(200, "text/plain", "Ok set glitcher: 0");
                  set_glitcher(0);
                }
                else if (new_state == "dump_full_flash")
                {

                  if (get_glitcher())
                  {
                    request->send(200, "text/plain", "ERROR Glitcher is running");
                    return;
                  }
                  if (is_nrf_connected() == 0)
                  {
                    request->send(200, "text/plain", "ERROR nRF not connected");
                    return;
                  }
                  else if (is_nrf_connected() == 1)
                  {
                    request->send(200, "text/plain", "ERROR nRF is locked");
                    return;
                  }

                  nrf_info_struct nrf_ufcr;
                  get_new_main_info(&nrf_ufcr);

                  if (nrf_ufcr.flash_size > (SPIFFS.totalBytes() - SPIFFS.usedBytes()))
                  {
                    request->send(200, "text/plain", "Not enough free space on the ESP32");
                    return;
                  }

                  String filename = "/full_flash.bin";
                  set_read_flash(0, nrf_ufcr.flash_size, filename);

                  request->send(200, "text/plain", "Ok create task full dump");
                }
                else if (new_state == "dump_full_uicr")
                {

                  if (get_glitcher())
                  {
                    request->send(200, "text/plain", "ERROR Glitcher is running");
                    return;
                  }
                  if (is_nrf_connected() == 0)
                  {
                    request->send(200, "text/plain", "ERROR nRF not connected");
                    return;
                  }
                  else if (is_nrf_connected() == 1)
                  {
                    request->send(200, "text/plain", "ERROR nRF is locked");
                    return;
                  }

                  if (0x1000 > (SPIFFS.totalBytes() - SPIFFS.usedBytes()))
                  {
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

  server.on(
      "/flash_file_direct", HTTP_POST, [](AsyncWebServerRequest *request)
      { request->send(200, "text/plain", "Upload complete!"); },
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        static LoopbackStream buffer(512);
        static uint32_t flash_offset;
        static uint32_t written_bytes;
        static long millis_start;
        static int file_type;
        static bool upload_failed;

        size_t pos = 0;

        Serial.printf("received data file:%s index:%d len:%d final:%d\r\n", filename.c_str(), index, len, final);

        if (!index)
        {
          millis_start = millis();
          written_bytes = 0;
          upload_failed = false;
          buffer.clear();

          if (request->hasParam("flash_up_file_offset"), true)
          {
            flash_offset = hstol(request->getParam("flash_up_file_offset", true)->value());
          } else {
            flash_offset = 0;
          }

          if (filename.endsWith(".hex")){
            file_type = 2;
            flash_offset = 0; //we ignore offset for hex files
          } else {
            file_type = 1;
          }
        }

        if (upload_failed){
          return;
        }

        while (pos < len) {
          buffer.write(data[pos++]);

          if (file_type == 1){
            int chunk_size = 256;
            if ((pos == len) && (final)){
              chunk_size = buffer.available();
            }

            if (buffer.available() >= chunk_size){
              char buf[chunk_size];
              buffer.readBytes(buf, chunk_size);

              Serial.printf("Ok gonna flash bank final: %i, offset:%08x, len: %i len_callback %i\r\n", final, flash_offset + written_bytes, chunk_size, len);
              nrf_write_bank(flash_offset + written_bytes, (uint32_t *)buf, chunk_size);
              written_bytes += chunk_size;
            }
          } else if (file_type == 2){
              if (buffer.contains('\n')){
                String line = buffer.readStringUntil('\n');
                Serial.printf("hex line found: \r\n"); Serial.print(line); Serial.printf("\r\n");

                if (line[0] != ':'){
                  Serial.printf("ERROR: line not starting with ':'\r\n");
                  upload_failed = true;
                  return;
                }
                int data_len = nibble(line[1])*16 + nibble(line[2]);
                int line_len = data_len + 5;
                byte buf[line_len];
                if (int ret = decode_line(buf, line)) {
                  Serial.printf("ERROR: line decode error %d\r\n", ret);
                  upload_failed = true;
                  return;
                }

                if (buf[3] == 0x02){
                  flash_offset = buf[4]*0x1000 + buf[5]*0x10;
                  Serial.printf("Setting flash_offset to %d\r\n", flash_offset);
                } else if (buf[3] == 0x04){
                  flash_offset = (flash_offset & 0xFFFF) + (buf[4] << 24) + (buf[5] << 16);
                  Serial.printf("Setting flash_offset to %d\r\n", flash_offset);
                } else if (buf[3] == 0x00){
                  int off = buf[1]*0x100 + buf[2];
                  Serial.printf("Writing flash_offset: 0x%x off 0x%x\r\n", flash_offset, off);
                  nrf_write_bank(flash_offset + off, (uint32_t *)(buf+4), buf[0]);
                  written_bytes += buf[0];
                }

              }
          }

        }

        if (final)
        {
          set_last_speed((float)((float)(written_bytes) / (float)(millis() - millis_start)));
        }
      });

  server.on("/download_flash", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              nrf_info_struct nrf_ufcr;
              get_new_main_info(&nrf_ufcr);
              int download_len = nrf_ufcr.flash_size;

              static uint32_t offset = 0;

              if (request->hasParam("offset"))
              {
                offset = hstol(request->getParam("offset")->value());

                if (offset > nrf_ufcr.flash_size)
                {
                  request->send(400, "text/html", "ERROR: Invalid offset!");
                  return;
                }
              }

              if (request->hasParam("len"))
              {
                download_len = hstol(request->getParam("len")->value());
                if (offset + download_len > nrf_ufcr.flash_size)
                {
                  request->send(400, "text/html", "ERROR: Too big download length!");
                  return;
                }
              }

              if (offset + download_len > nrf_ufcr.flash_size)
              {
                download_len = nrf_ufcr.flash_size - offset;
              }

              Serial.printf("reading flash: offset:0x%X download_len:%d\n", offset, download_len);

              AsyncWebServerResponse *response = request->beginResponse("application/octet-stream", download_len, [](uint8_t *buffer, size_t maxLen, size_t index) -> size_t
                                                                        {
                                                                          Serial.printf("handle download: index:0x%X maxLen:%d offset:0x%X\n", index, maxLen, offset);

                                                                          if (maxLen > 0)
                                                                          {
                                                                            return nrf_read_bank_bytes(offset + index, buffer, maxLen);
                                                                          }

                                                                          return 0;
                                                                        });

              response->addHeader("Content-Disposition", "attachment; filename=\"flash.bin\"");
              request->send(response);
            });

  server.on("/get_graph", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              if (request->hasParam("size") && request->hasParam("delay"))
              {
                uint32_t size = request->getParam("size")->value().toInt();
                uint32_t delay_time = request->getParam("delay")->value().toInt();
                uint16_t graph_buff[size] = {0};
                get_osci_graph(graph_buff, size, delay_time);
                String answer_state = "";
                for (int i = 0; i < size; i++)
                {
                  answer_state += String(graph_buff[i]) + ",";
                }
                answer_state += "0";
                request->send(200, "text/plain", answer_state);
              }
            });

  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.htm");

  server.onNotFound([](AsyncWebServerRequest *request)
                    {
                      if (request->url() == "/" || request->url() == "index.htm")
                      { // not uploaded the index.htm till now so notify the user about it
                        request->send(200, "text/html", "please use <a href=\"/edit\">/edit</a> with login defined in web.cpp to upload the supplied index.htm to get full useage");
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
