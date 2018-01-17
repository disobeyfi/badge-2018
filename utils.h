/*
 * This file is part of Disobey Badge 2018 firmware.
 *
 * Disobey Badge 2018 firmware is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Disobey Badge 2018 firmware is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Disobey Badge 2018 firmware.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>

void sleep_standby(int time) __attribute__ ((noreturn));
void sleep_stop(int time);
void sleep_cpu(int time);
void initrtc();

void crapdelay(int aika);

uint32_t random_getseed();
void random_setseed();
uint32_t random();

void init_usbserial();
extern char usb_serial[9];

#define BULK_EP_MAXPACKET (64)

extern uint8_t usbd_control_buffer[BULK_EP_MAXPACKET*5];
