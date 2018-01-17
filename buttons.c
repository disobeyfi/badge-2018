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

#include "buttons.h"
#include "utils.h"
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

typedef struct
{
	int rcc;
	int port;
	int num;
} button_t;

const int buttoncount = 8;
button_t buttons[8] =
{

{ RCC_GPIOB, GPIOB, GPIO1 }, // a
{ RCC_GPIOB, GPIOB, GPIO14 }, // b
{ RCC_GPIOB, GPIOB, GPIO3 }, // select
{ RCC_GPIOC, GPIOC, GPIO13 }, // start

{ RCC_GPIOB, GPIOB, GPIO2 }, // up
{ RCC_GPIOB, GPIOB, GPIO12 }, // down
{ RCC_GPIOB, GPIOB, GPIO11 }, // left
{ RCC_GPIOB, GPIOB, GPIO13 }, // right

};

void buttons_setup(void)
{
	for (int i = 0; i < buttoncount; i++)
	{
		rcc_periph_clock_enable(buttons[i].rcc);
		gpio_mode_setup(buttons[i].port, GPIO_MODE_INPUT, GPIO_PUPD_PULLUP, buttons[i].num);
	}
}

static int buttons_read_lower(void)
{
	int out = 0;

	for (int i = 0; i < buttoncount; i++)
	{
		if (!gpio_get(buttons[i].port, buttons[i].num))
			out |= BUTTON(i);
	}

	return out;
}

int buttons_read(void)
{
	// very simple debouncing. might or might not work
	int state = buttons_read_lower();
	crapdelay(100);
	state &= buttons_read_lower();

	return state;
}

void buttons_kill(void)
{
	for (int i = 0; i < buttoncount; i++)
	{
		gpio_mode_setup(buttons[i].port, GPIO_MODE_INPUT, GPIO_PUPD_NONE, buttons[i].num);
	}
}