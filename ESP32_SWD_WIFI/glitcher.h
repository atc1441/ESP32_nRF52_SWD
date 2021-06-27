/*
   Copyright (c) 2021 Aaron Christophel ATCnetz.de
   SPDX-License-Identifier: GPL-3.0-or-later
*/
#pragma once

void glitcher_begin();
void set_glitcher(bool new_state);
bool get_glitcher();
void set_power(bool state);
void do_glitcher();
void set_delay(uint32_t delay_us,uint32_t delay_us_end);
uint32_t get_delay();
bool inc_delay();
void set_width(uint32_t width);
uint32_t get_width();
bool inc_width();

void get_osci_graph(uint16_t graph_buff[],uint32_t size,uint32_t delay_time);