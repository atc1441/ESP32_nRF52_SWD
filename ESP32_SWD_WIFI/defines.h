/*
   Copyright (c) 2021 Aaron Christophel ATCnetz.de
   SPDX-License-Identifier: GPL-3.0-or-later
*/
#pragma once

#define LED_R 16
#define LED_G 5
#define LED_B 26
#define GLITCHER 23
#define NRF_POWER 19
#define NRF_POWER_5V 18

#define OSCI_PIN 36

#define swd_clock_pin 21
#define swd_data_pin 22

#define AP_NRF_RESET 0x00
#define AP_NRF_ERASEALL 0x04
#define AP_NRF_ERASEALLSTATUS 0x08
#define AP_NRF_ERASEALL 0x04
#define AP_NRF_APPROTECTSTATUS 0x0c
#define AP_NRF_IDR 0xfc

#define AP_CSW 0x00
#define AP_TAR 0x04
#define AP_DRW 0x0c
#define AP_BD0 0x10
#define AP_BD1 0x14
#define AP_BD2 0x18
#define AP_BD3 0x1c
#define AP_DBGDRAR 0xf8
#define AP_IDR 0xfc

#define DP_ABORT 0x00
#define DP_IDCODE 0x00
#define DP_CTRLSTAT 0x04
#define DP_SELECT 0x08
#define DP_RDBUFF 0x0c

struct nrf_info_struct
{
  int flash_size;
  uint32_t connected;
  uint32_t codepage_size;
  uint32_t codesize;
  uint32_t config_id;
  uint32_t device_id0;
  uint32_t device_id1;
  uint32_t info_part;
  uint32_t info_variant;
  uint32_t info_package;
  uint16_t sd_info_area;
  uint32_t ucir_lock;
};
