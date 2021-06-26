/*
   Copyright (c) 2021 Aaron Christophel ATCnetz.de
   SPDX-License-Identifier: GPL-3.0-or-later
*/
#pragma once
#include "swd.h"
#include "defines.h"

uint32_t nrf_begin(bool muted = false);
uint8_t is_nrf_connected();
void do_nrf_swd();
uint32_t nrf_read_port(bool APorDP, uint8_t address);
void nrf_write_port(bool APorDP, uint8_t address, uint32_t value);
void nrf_abort_all();
void nrf_halt();
void nrf_read_ufcr();
uint32_t read_register(uint32_t address, bool muted = 0);
void write_register(uint32_t address, uint32_t value, bool muted = 0);
void write_flash(uint32_t address, uint32_t value);
void erase_flash();
void erase_page(uint32_t page);
uint8_t flash_file(uint32_t offset, String &path);
uint8_t dump_flash_to_file(uint32_t offset, uint32_t read_size, String &path);

uint8_t nrf_write_bank(uint32_t address, uint32_t buffer[], int size);
uint8_t nrf_read_bank(uint32_t address, uint32_t buffer[], int size);

void set_write_flash(uint32_t offset, String &path);
void set_read_flash(uint32_t offset, uint32_t file_size, String &path);
bool get_task_flash(uint8_t *percent);
float get_last_speed();

void set_new_main_info(bool state);
bool get_new_main_info();
void get_new_main_info(nrf_info_struct *_nrf_ufcr);
/*
   NRF custom control port
   For handling low level reset or Protection Bits
*/
void nrf_port_selection(bool new_port);
bool nrf_read_lock_state();
void nrf_soft_reset();
void nrf_erase_all();
/*
  NRF custom control port stuff END
*/
