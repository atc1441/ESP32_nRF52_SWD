/*
   Copyright (c) 2021 Aaron Christophel ATCnetz.de
   SPDX-License-Identifier: GPL-3.0-or-later
*/
#pragma once

void swd_begin();
uint32_t swd_init();
bool AP_Write(unsigned addr, uint32_t data);
bool AP_Read(unsigned addr, uint32_t &data);
bool DP_Write(unsigned addr, uint32_t data);
bool DP_Read(unsigned addr, uint32_t &data);
bool swd_transfer(unsigned port_address, bool APorDP, bool RorW, uint32_t &data);
bool calculate_parity(uint32_t in_data);
void swd_write(uint32_t data, uint8_t bits);
uint32_t swd_read(uint8_t bits);
void swd_turn(bool WorR);
