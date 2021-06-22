/*
   Copyright (c) 2021 Aaron Christophel ATCnetz.de
   SPDX-License-Identifier: GPL-3.0-or-later
*/
#include "Arduino.h"
#include "defines.h"
#include "nrf_swd.h"
#include "swd.h"
#include <FS.h>
#include <SPIFFS.h>
#include "esp_task_wdt.h"

bool is_connected = false;
long last_ack_check = 0;

#define ack_check_interval 1000

bool _task_write_flash = false;
bool _task_read_flash = false;
uint32_t _offset = 0;
uint32_t _file_size = 0;
String _filename = "";
volatile uint8_t _percent = 0;
volatile float _speed = 0;

nrf_info_struct nrf_ufcr;
bool new_info = false;


uint32_t nrf_begin(bool muted) {
  uint32_t temp = swd_init();
  nrf_abort_all();
  if (temp == 0x2ba01477) {//if core id is readable the connection is working
    is_connected = true;
    nrf_ufcr.connected = 1;
    if (nrf_read_lock_state()) {//nRF is unlocked so we can talk to the debugging interface
      nrf_halt();
      nrf_read_ufcr();
      nrf_ufcr.connected = 2;
    }
  } else {
    is_connected = false;
    nrf_ufcr.connected = 0;
  }
  if (!muted)
    new_info = true;
  return temp;
}

void do_nrf_swd() {
  static uint32_t CSW = 0;
  if (is_connected) {
    if (_task_write_flash) {
      flash_file(_offset, _filename);
      _task_write_flash = false;
    } else if (_task_read_flash) {
      dump_flash_to_file(_offset, _file_size, _filename);
      _task_read_flash = false;
    }
  }
}

uint32_t nrf_read_port(bool APorDP, uint8_t address) {
  uint32_t temp = 0;
  if (APorDP)
    AP_Read(address, temp);
  else
    DP_Read(address, temp);
  DP_Read(DP_RDBUFF, temp);
  DP_Read(DP_RDBUFF, temp);
  Serial.printf("%s Read reg: 0x%02x : 0x%08x\r\n", APorDP ? "AP" : "DP", address, temp);
  return temp;
}

void nrf_write_port(bool APorDP, uint8_t address, uint32_t value) {
  uint32_t temp = 0;
  if (APorDP)
    AP_Write(address, value);
  else
    DP_Write(address, value);
  DP_Read(DP_RDBUFF, temp);
  DP_Read(DP_RDBUFF, temp);
  Serial.printf("%s Write reg: 0x%02x : 0x%08x\r\n", APorDP ? "AP" : "DP", address, value);
}

void nrf_abort_all() {
  uint32_t temp = 0;
  nrf_write_port(0, DP_ABORT, 0x1e);
  nrf_write_port(0, DP_CTRLSTAT, 0x50000000);
}

void nrf_halt() {
  AP_Write(AP_CSW, 0xa2000002);
  AP_Write(AP_TAR, 0xe000edf0 );
  uint32_t retry = 500;
  while (retry--) {
    AP_Write(AP_DRW, 0xA05F0003);
  }
}


void nrf_read_ufcr() {
  nrf_ufcr.codepage_size = read_register(0x10000010);
  nrf_ufcr.codesize = read_register(0x10000014);
  nrf_ufcr.flash_size = nrf_ufcr.codepage_size * nrf_ufcr.codesize;
  Serial.printf("Flash size: %i\r\n", nrf_ufcr.flash_size);
  nrf_ufcr.config_id = read_register(0x1000005c);
  nrf_ufcr.device_id0 = read_register(0x10000060);
  nrf_ufcr.device_id1 = read_register(0x10000064);
  nrf_ufcr.info_part = read_register(0x10000100);
  nrf_ufcr.info_variant = read_register(0x10000104);
  nrf_ufcr.info_package = read_register(0x10000108);
  nrf_ufcr.sd_info_area = read_register(0x0000300C) & 0xffff;
  nrf_ufcr.ucir_lock = read_register(0x10001208);
}

uint32_t read_register(uint32_t address, bool muted) {
  uint32_t temp = 0;
  bool state1 = AP_Write(AP_TAR, address);
  bool state2 = AP_Read(AP_DRW, temp);
  bool state3 = DP_Read(DP_RDBUFF, temp);
  bool state4 = DP_Read(DP_RDBUFF, temp);
  if (!muted)Serial.printf("S1:%i,S2:%i,S3:%i,S4:%i Read Register: 0x%08x : 0x%08x\r\n", state1, state2, state3, state4, address, temp);
  return temp;
}

void write_register(uint32_t address, uint32_t value, bool muted) {
  uint32_t temp = 0;
  bool state1 = AP_Write(AP_TAR, address);
  bool state2 = AP_Write(AP_DRW, value);
  bool state3 = DP_Read(DP_RDBUFF, temp);
  if (!muted)Serial.printf("S1:%i,S2:%i,S3:%i Write Register: 0x%08x : 0x%08x\r\n", state1, state2, state3, address, value);
}

void write_flash(uint32_t address, uint32_t value) {
  write_register(0x4001e504, 1);
  while (read_register(0x4001e400) != 1) {}
  write_register(address, value);
  while (read_register(0x4001e400) != 1) {}
  write_register(0x4001e504, 0);
  while (read_register(0x4001e400) != 1) {}
}

void erase_flash() {
  write_register(0x4001e504, 2);
  while (read_register(0x4001e400) != 1) {}
  write_register(0x4001e50c, 1);
  while (read_register(0x4001e400) != 1) {}
  write_register(0x4001e504, 0);
  while (read_register(0x4001e400) != 1) {}
}

void erase_page(uint32_t page) {
  write_register(0x4001e504, 2);
  while (read_register(0x4001e400) != 1) {}
  write_register(0x4001e508, page);
  while (read_register(0x4001e400) != 1) {}
  write_register(0x4001e504, 0);
  while (read_register(0x4001e400) != 1) {}
}

uint8_t flash_file(uint32_t offset, String & path) {
  File file;
  file = SPIFFS.open(path, "rb");
  if (file == 0)
  {
    return 1;
  }
  file.seek(0, SeekEnd);
  uint32_t file_size = file.position();
  file.seek(0, SeekSet);

  Serial.printf("Going to write %i bytes to flash\r\n", file_size);

  uint8_t buffer[4096] = {0x00};
  long millis_start = millis();

  for (int posi = 0; posi < file_size; posi += 4096) {
    uint32_t cur_len = (file_size - posi >= 4096) ? 4096 : file_size - posi;
    file.read(buffer, (size_t)cur_len);
    nrf_write_bank(posi + offset, (uint32_t *)&buffer, cur_len);
    _percent = (uint8_t)(((float)posi / (float)file_size) * 100);
  }

  file.close();
  _speed = (float)((float)(file_size / (float)(millis() - millis_start)));
  Serial.printf("Done flashing file, it took %ims speed: %.4fkbs\r\n", millis() - millis_start, _speed);
  return 0;
}

uint8_t dump_flash_to_file(uint32_t offset, uint32_t read_size, String & path) {

  File file;
  file = SPIFFS.open(path, "wb");
  if (file == 0)
  {
    return 1;
  }

  uint32_t temp;
  Serial.printf("Going to read %i bytes to file\r\n", read_size);
  long millis_start = millis();

  uint8_t buffer[4096] = {0x00};

  for (int posi = 0; posi < read_size; posi += 4096) {
    uint32_t cur_len = (read_size - posi >= 4096) ? 4096 : read_size - posi;
    nrf_read_bank(posi + offset, (uint32_t *)&buffer, cur_len);
    file.write(buffer, (size_t)cur_len);
    _percent = (uint8_t)(((float)posi / (float)read_size) * 100);
  }

  file.close();
  _speed = (float)((float)(read_size / (float)(millis() - millis_start)));
  Serial.printf("Done reading file, it took %ims speed: %.4fkbs\r\n", millis() - millis_start, _speed);
  return 0;
}

uint8_t nrf_write_bank(uint32_t address, uint32_t buffer[], int size) {
  if (!is_connected)
    return 1;// not connected to an nRF

  if (size > 4096)
    return 2;// buffer bigger then a bank

  uint32_t temp;

  write_register(0x4001e504, 1);// NVIC Enable writing
  while (read_register(0x4001e400) != 1) {}

  AP_Write(AP_CSW, 0xa2000012);
  AP_Write(AP_TAR, address);

  for (int posi = 0; posi < size; posi += 4) {
    long end_micros = micros() + 400;//wait till writing of nRF memory is done without asking for ready state
    AP_Write(AP_DRW, buffer[posi / 4]);
    while (micros() < end_micros) {};
  }

  AP_Write(AP_CSW, 0xa2000002);
  DP_Read(DP_RDBUFF, temp);
  DP_Read(DP_RDBUFF, temp);

  write_register(0x4001e504, 0);// NVIC Diasble writing
  while (read_register(0x4001e400) != 1) {}

  return 0 ;
}

uint8_t nrf_read_bank(uint32_t address, uint32_t buffer[], int size) {
  if (!is_connected)
    return 1;// not connected to an nRF

  uint32_t temp;

  AP_Write(AP_CSW, 0xa2000012);
  AP_Write(AP_TAR, address);
  AP_Read(AP_DRW, temp);

  uint32_t curr_word = 0;
  for (int posi = 0; posi < size; posi += 4) {
    AP_Read(AP_DRW, curr_word);
    buffer[posi / 4] = curr_word;
  }

  AP_Write(AP_CSW, 0xa2000002);
  DP_Read(DP_RDBUFF, temp);
  DP_Read(DP_RDBUFF, temp);

  return 0 ;
}

void set_write_flash(uint32_t offset, String & path) {
  _offset = offset;
  _filename = path;
  _percent = 0;
  _speed = 0;
  _task_write_flash = true;
}

void set_read_flash(uint32_t offset, uint32_t file_size, String & path) {
  _offset = offset;
  _file_size = file_size;
  _filename = path;
  _percent = 0;
  _speed = 0;
  _task_read_flash = true;
}

bool get_task_flash(uint8_t *percent) {
  if (_task_read_flash || _task_write_flash) {
    *percent = _percent;
    return true;
  }
  return false;
}

float get_last_speed() {
  return _speed;
}

void set_new_main_info(bool state) {
  new_info = state;
}

bool get_new_main_info() {
  bool temp_new_info = new_info;
  new_info = false;
  return temp_new_info;
}

void get_new_main_info(nrf_info_struct *_nrf_ufcr) {
  *_nrf_ufcr = nrf_ufcr;
}

/*
   NRF custom control port
   For handling low level reset or Protection Bits
*/
void nrf_port_selection(bool new_port) {
  DP_Write(DP_SELECT, new_port ? 0x01000000 : 0x00); //Select AP
}

bool nrf_read_lock_state() {
  uint32_t temp;
  nrf_port_selection(1);
  temp = nrf_read_port(1, AP_NRF_APPROTECTSTATUS);
  nrf_port_selection(0);
  return temp & 1;
}

void nrf_soft_reset() {
  nrf_port_selection(1);
  nrf_write_port(1, AP_NRF_RESET, 1);
  delay(100);
  nrf_write_port(1, AP_NRF_RESET, 0);
  nrf_port_selection(0);
}

void nrf_erase_all() {
  uint32_t temp = 0;
  nrf_port_selection(1);
  nrf_write_port(1, AP_NRF_ERASEALL, 1);
  delayMicroseconds(100);
  while (nrf_read_port(1, AP_NRF_ERASEALLSTATUS)) {}
  nrf_write_port(1, AP_NRF_ERASEALL, 0);
  nrf_port_selection(0);
  nrf_soft_reset();
  nrf_begin();
}
/*
   NRF custom control port END
*/
