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

#include <libopencmsis/core_cm3.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/pwr.h>

#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/usart.h>

#include <libopencm3/stm32/timer.h>

#include <stdbool.h>

#include <stdio.h>

#include <stdlib.h>
#include <math.h>
#include <string.h>

#include "utils.h" 
#include "debugf.h"
#include "lights.h"
#include "buttons.h"
#include "fondling.h"
#include "animations.h"

#include "keys/badgekey.h"

const bool enable_gp_usb = 1;
const bool enable_kb_usb = 1;

void keyboard(int keybits, const char *keystring);
void gamepad(void);


// key handling integration
extern badgekey_t mykey;
extern uint32_t keycombos[];

badgekey_t key_current;
int key_bits = 0;

void updatekeybits()
{
	uint32_t keyhash = key_gethash(&key_current);

	key_bits = 0;

	for (int i = 0; i < 512; i++)
	{
		if (keyhash == keycombos[i])
		{
			key_bits = i;
		}
	}
}

void loadkey()
{
	uint32_t *key32 = (uint32_t *)key_current.key;

	rtc_unlock();
	key32[0] = RTC_BKPXR(1);
	key32[1] = RTC_BKPXR(2);
	key32[2] = RTC_BKPXR(3);
	key32[3] = RTC_BKPXR(4);
	rtc_lock();

	updatekeybits();

	if (key_bits == 0)
	{
		key_current = mykey;
		updatekeybits();
	}

	debugf("Restored key bits: 0x%X/0x1FF\r\n", key_bits);

}

void savekey()
{
	uint32_t *key32 = (uint32_t *)key_current.key;

	rtc_unlock();
	RTC_BKPXR(1) = key32[0];
	RTC_BKPXR(2) = key32[1];
	RTC_BKPXR(3) = key32[2];
	RTC_BKPXR(4) = key32[3];
	rtc_lock();
}





char* synctext = "HELP ME OBI-WAN, YOU ARE MY ONLY HOPE... ";

void receivecode()
{
	lights_setup();

	debugf("woah code!\r\n");

#if 0
	int flashy = 0xAAAA;
	for (int i = 0; i < 5; i++)
	{
		flashy ^= 0xFFFF;
		lights_set(flashy);
		sleep_cpu(0x500);
	}
#endif

	int synclen = (int)strlen(synctext);
	int syncpos = 0;
	int mode = 0;

	uint8_t keyhex[33];
	keyhex[32] = 0;

	int keyhexidx = 0;
	int keyhexidx2 = 0;

	int payloadcounter = 0;

	debugf("now recving\r\n");

	while(1)
	{
		int wasted = 0;
		while(1)
		{
			bool isdata = fondling_data_avail();

			if (!isdata)
			{
				sleep_cpu(20);
				wasted++;
				if (wasted == 200)
				{
					debugf("woah ok this is enough!\r\n");
					return;
				}
			}
			else
				break;
		}


		uint16_t poo = usart_recv_blocking(USART4);

		if (syncpos < synclen)
		{
			debugf("%c", poo);

			if (poo == synctext[syncpos])
				syncpos++;
			else
			{
				syncpos = 0;
				debugf("\r\n\r\n");
			}

			mode = 0;
			keyhexidx = 0;
			keyhexidx2 = 0;

			payloadcounter = 70; // just some value that's longer than the payload
		}
		else
		{
			debugf("'%c' ", poo);

			if (mode == 0)
			{
				if (poo == 'x')
					mode = 1;
				else if (keyhexidx < 32)
					keyhex[keyhexidx++] = (uint8_t)poo;
				else
					syncpos = 0;
			}
			else if (mode == 1)
			{
				if (poo == '\r')
					mode = 2;
				else if (keyhexidx2 < 32 && keyhex[keyhexidx2++] == poo)
					{}
				else
					syncpos = 0;
			}

			payloadcounter--;
			if (payloadcounter == 0)
				syncpos = 0;

			else if (poo == '\n' && keyhexidx == 32 && keyhexidx2 == 32)
			{
				badgekey_t inputkey;
				key_fromhex(&inputkey, keyhex);

				debugf("key: '%s', key again: '%s', hash %08X\r\n", keyhex, keyhex, key_gethash(&inputkey));

				key_combine(&key_current, &inputkey);
				savekey();
				scb_reset_system();
				// fuck you :DS:D:D:fg,ifd0ih

				syncpos = 0;
			}
		}

		static int ls = 0xAAAA;
		ls ^= 0xFFFF;
		lights_set(ls);
	}
}

void codesend()
{
	fondling_usart_setup();

	lights_setup();

	char packet[256];

	char keyhex[33];
	keyhex[32] = 0;
	key_gethex(&mykey, keyhex);

	int packetlen = sprintf(packet, "%s%sx%s\r\n", synctext, keyhex, keyhex);

	for (int t = 0; t < 5; t++)
	{
		for (int idx = 0; idx < packetlen; idx++)
		{
			usart_send_blocking(USART4, packet[idx]);
			debugf("sending: '%c'\r\n", packet[idx]);

			static int ls = 1 << 9;
			ls ^= 1 << 10 | 1 << 12;
			lights_set(ls);
			sleep_cpu(0x10);
		}
	}


	scb_reset_system();
}

void codesend_trigger()
{
	fondling_usart_setup();

	lights_setup();

	char packet[256];
	int packetlen = sprintf(packet, "Amiga users like when their antennas touch");

	for (int t = 0; t < 20; t++)
	{
		for (int idx = 0; idx < packetlen; idx++)
		{
			usart_send_blocking(USART4, packet[idx]);
			debugf("sending: '%c'\r\n", packet[idx]);

			static int ls = 0;
			ls ^= 1 << 9;
			lights_set(ls);
			sleep_cpu(0x50);
		}

		usart_recv(USART4);

		for (int i = 0; i < 10; i++)
		{
			static int ls = 0;
			ls ^= 1 << 10 | 1 << 12;
			lights_set(ls);
			sleep_cpu(0x30);

			bool dada = fondling_data_avail();
			debugf("fondling input: %i (try %i)\r\n", dada ? 1 : 0, i);

			if (dada)
			{

#if 0
				while(1)
				{
					lights_set(0xFFFF);
					sleep_cpu(0x300);
					lights_set(0x0000);
					sleep_cpu(0x300);

				}
#endif

				receivecode();
				return;
			}

		}
	}

}




int code[] =
{
	BUTTON(4),
//	BUTTON(4),
	BUTTON(5),
//	BUTTON(5),

	BUTTON(6),
	BUTTON(7),
	BUTTON(6),
	BUTTON(7),
	
	BUTTON(1),
	BUTTON(0),
};
int codelen = 8;

bool input_konamicode_testfirst()
{
	return buttons_read();
}

bool input_konamicode()
{
	lights_setup();
	int ledstate = 0;

	int timer = 50;

	int flashy = 0;
	int seqpos = 0;

	while(timer--)
	{
		int buts = buttons_read();
		if (buts)
		{
			timer = 100;
			if (seqpos < codelen && buts == code[seqpos])
			{
				flashy |= 1 << seqpos;
				seqpos++;
				if (seqpos == codelen)
					break;
			}
			else if (seqpos >= 1 && buts == code[seqpos-1])
			{
			
			}
			else
			{
				flashy = 0;
				ledstate = 0;
				seqpos = 0;
			}
		}

		ledstate ^= flashy;

		lights_set(ledstate);

		if (gpio_get(GPIOB, GPIO5) && enable_kb_usb)
		{
			debugf("late usb detected!\n");

			lights_set(0);


			char keyhex[33];
			keyhex[32] = 0;
			key_gethex(&key_current, keyhex);
			keyboard(key_bits, keyhex);

			// no return from here
		}


		sleep_cpu(0x50);
	}

	lights_kill();
	if (seqpos == codelen)
	{
		return 1;
	}


	return 0;
}

int check_fondling(bool manualreset)
{
	int returncode = 0;

	debugf("Checking for incoming key, keypress and late USB power..\r\n");


	fondling_usart_setup();

	for (int i = 0; i < 50; i++)
	{
		bool dada;
		dada = fondling_data_avail();
//		debugf("fondling input: %i (try %i)\r\n", dada ? 1 : 0, i);
		if (dada)
		{
			//receivecode();
			debugf("incoming uart data detected!\n");

			codesend();
			// no return
		}

		//for (int x = 0; x < 9; x++)
		int curbit = i % 9;
		{
			if (manualreset)
			{
				//int bitmask = (1 << (curbit + 1)) - 1;
				int bitmask = curbit < 4 ? 0 : 0x1FF;

				lights_set((bitmask & key_bits) | (1 << 9));
				sleep_cpu(99);
			}
			else
				sleep_cpu(99);
		}

#if 0
		if (gpio_get(GPIOB, GPIO5) && enable_kb_usb && manualreset)
		{
			debugf("late usb detected!\n");

			lights_set(0);


			char keyhex[33];
			keyhex[32] = 0;
			key_gethex(&key_current, keyhex);
			keyboard(key_bits, keyhex);

			// no return from here
		}
#endif

#if 1
		if (gpio_get(GPIOB, GPIO5) && enable_gp_usb && manualreset)
		{
			lights_kill();
			//debugf("waiting for stabilization..\n");
			//sleep_cpu(10000);
			gamepad();
		}
#endif

		if (manualreset && input_konamicode_testfirst())
		{
			debugf("cheat code detected!\n");

			returncode = 1;
			break;
		}

//		sleep_cpu(0x40);
	}

	fondling_usart_kill();
	return returncode;
}


int main(void)
{
	debug_usart_setup();
	init_usbserial();

#if 0
	{
		gamepad();
	}
#endif

	debugf("\r\n\r\n** Starting up disobey badge ** \r\n");

	initrtc();


	// if device uses too much power, it resets and it might be that
	// the random seed doesn't get updated, leading to same animation again
	// --> reset loop

	rtc_unlock();
	uint32_t seed = RTC_BKPXR(0);
	random_setseed(seed);
	rtc_lock();

	debugf("Advancing the RNG: %lu\r\n", random());

	//goto animations;

	rtc_unlock();
	RTC_BKPXR(0) = random_getseed();
	rtc_lock();

	buttons_setup();

	int bootbuts = buttons_read();
	debugf("Button status on boot: %X\r\n", bootbuts);

	uint32_t resetreason = RCC_CSR & (uint32_t)RCC_CSR_RESET_FLAGS;
	RCC_CSR |= RCC_CSR_RMVF;

	debugf("Reset reason: %08X\r\n", resetreason);

	bool manualreset = resetreason & RCC_CSR_PINRSTF;

	if (manualreset)
	{
		lights_setup();
		lights_set(1 << 9);
	}

	loadkey();

	debugf("Checking for USB power\r\n");
	// TODO: move this to some usb.c
	rcc_periph_clock_enable(RCC_GPIOB);
	gpio_mode_setup(GPIOB, GPIO_MODE_INPUT, GPIO_PUPD_NONE, GPIO5);
	if (gpio_get(GPIOB, GPIO5) && enable_gp_usb)
	{
		lights_kill();
		//debugf("waiting for stabilization..\n");
		//sleep_cpu(10000);
		gamepad();
	}




	int fondling_response = check_fondling(manualreset);


	if (fondling_response)
	{
		int codeid = input_konamicode();

		if (codeid == 1)
		{
			codesend_trigger();
			scb_reset_system();
		}
	}

	buttons_kill();


	animations:
	while (1)
	{
		anim_run();

		int aika = (int)(random() % 8000) + 36000;

		debugf("Storing RNG and going to sleep..\r\n");
		rtc_unlock();
		RTC_BKPXR(0) = random_getseed();
		rtc_lock();

		sleep_standby(aika);
	}

	return 0;
}
