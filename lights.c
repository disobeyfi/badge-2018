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

#include "lights.h"

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

/*
	SMILE
	PA1
	PA2
	PA3
	PA8

*/

typedef struct
{
	int rcc;
	uint32_t port;
	uint16_t num;

	int timer;
	int oc;
	uint8_t af;
} led_t;

const int ledcount = 14;

		/*
		0
		1
		2 :)
		3 :)
		4
		5
		6
		7 :)
		8 :)
		9
		10
		11
		12 :)
		13
		*/



// PB17 = dis

led_t leds[14] =
{
 // optimize timer usage
{ RCC_GPIOA, GPIOA, GPIO7,  3,  TIM_OC2, GPIO_AF1}, //3 OK
{ RCC_GPIOB, GPIOB, GPIO4,  3,  TIM_OC1, GPIO_AF1}, //9 ?
{ RCC_GPIOB, GPIOB, GPIO15, 15, TIM_OC2, GPIO_AF1}, //13 OK
{ RCC_GPIOA, GPIOA, GPIO10, 1,  TIM_OC3, GPIO_AF2}, //6 OK
{ RCC_GPIOA, GPIOA, GPIO4,  14, TIM_OC1, GPIO_AF4}, //2 OK
{ RCC_GPIOA, GPIOA, GPIO8,  1,  TIM_OC1, GPIO_AF2}, //4 OK
{ RCC_GPIOA, GPIOA, GPIO15, 2,  TIM_OC1, GPIO_AF2}, //7 OK
{ RCC_GPIOA, GPIOA, GPIO9,  1,  TIM_OC2, GPIO_AF2}, //5 OK
{ RCC_GPIOA, GPIOA, GPIO3,  2,  TIM_OC4, GPIO_AF2}, //1 OK
{ RCC_GPIOB, GPIOB, GPIO9,  17, TIM_OC1, GPIO_AF2}, //11 OK
{ RCC_GPIOA, GPIOA, GPIO2,  15, TIM_OC1, GPIO_AF0}, //0 OK
{ RCC_GPIOB, GPIOB, GPIO10, 2,  TIM_OC3, GPIO_AF2}, //12 OK
{ RCC_GPIOB, GPIOB, GPIO8,  16, TIM_OC1, GPIO_AF2}, //10 OK
{ RCC_GPIOB, GPIOB, GPIO0,  3,  TIM_OC3, GPIO_AF1}, //8 OK
};

int timer_rcc[] = {
	0,
	RCC_TIM1,
	RCC_TIM2,
	RCC_TIM3,
	0,
	0,
	RCC_TIM6,
	RCC_TIM7,
	0,
	0,
	0,
	0,
	0,
	0,
	RCC_TIM14,
	RCC_TIM15,
	RCC_TIM16,
	RCC_TIM17,
};

int timer_rst[] = {
	0,
	RST_TIM1,
	RST_TIM2,
	RST_TIM3,
	0,
	0,
	RST_TIM6,
	RST_TIM7,
	0,
	0,
	0,
	0,
	0,
	0,
	RST_TIM14,
	RST_TIM15,
	RST_TIM16,
	RST_TIM17,
};

uint32_t timer_base[] = {
	0,
	TIM1,
	TIM2,
	TIM3,
	0,
	0,
	TIM6,
	TIM7,
	0,
	0,
	0,
	0,
	0,
	0,
	TIM14,
	TIM15,
	TIM16,
	TIM17,
};

int timer_refmask[18];
/*
	TIM3_CH2 	PA7
	TIM3_CH1	PB4
	TIM15_CH2	PB15
	TIM1_CH3 	PA10
	TIM14_CH1	PA4
	TIM1_CH1 	PA8
	TIM2_CH1	PA15
	TIM1_CH2 	PA9
	TIM2_CH4	PA3
	TIM15_CH1 	PA5
	TIM16_CH1 	PB8
	TIM3_CH3 	PB0
	TIM17_CH1 	PB9
*/

bool timers[18];
static uint32_t p_curlimit = 2047;

#ifdef LEDYELLOW
static uint32_t p_staticlimit = 2047; // yellow
#endif

#ifdef LEDRED
static uint32_t p_staticlimit = 1024; // new red?
#endif

#ifdef LEDGREEN
static uint32_t p_staticlimit = 600; // new green?
#endif

#ifdef LEDPROTO2
static uint32_t p_staticlimit = 1024; // proto2
#endif

void lights_setup(void)
{
	lights_setup_pwm();
	lights_limit(2047);
}

void lights_limit(uint32_t maximum)
{
	if (maximum > 2047)
		p_curlimit = 2047;
	else
		p_curlimit = maximum;

	p_curlimit = (p_curlimit * p_staticlimit) / 2048;
}

void lights_setup_pwm(void)
{
	for (int i = 0; i < 18; i++)
	{
		timers[i] = false;
		timer_refmask[i] = 0;
	}

	for (int i = 0; i < ledcount; i++)
	{
		rcc_periph_clock_enable(leds[i].rcc);

		gpio_mode_setup(leds[i].port, GPIO_MODE_INPUT, GPIO_PUPD_NONE, leds[i].num);

		timers[leds[i].timer] = true;
	}

	for (int i = 0; i < 18; i++)
	{
		if (timers[i])
		{
			rcc_periph_clock_enable(timer_rcc[i]);
			rcc_periph_reset_pulse(timer_rst[i]);

			timer_set_mode(timer_base[i], TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
			timer_set_period(timer_base[i], 2048);
			timer_set_prescaler(timer_base[i], 0);

//			timer_enable_break_main_output(timer_base[i]); // works only on TIM1

			TIM_BDTR(timer_base[i]) |= TIM_BDTR_MOE;


			timer_enable_preload(timer_base[i]);
 
			timer_generate_event(timer_base[i], TIM_EGR_UG);

			timer_enable_counter(timer_base[i]);

			rcc_periph_clock_disable(timer_rcc[i]);
		}
	}
#if 0
 	TIM_BDTR(TIM1) |= TIM_BDTR_MOE;
	TIM_BDTR(TIM15) |= TIM_BDTR_MOE;
	TIM_BDTR(TIM16) |= TIM_BDTR_MOE;
	TIM_BDTR(TIM17) |= TIM_BDTR_MOE;
#endif

}

void lights_set(int state)
{
	uint32_t bright[14];

	for (int i = 0; i < ledcount; i++)
	{
		if (state & (1 << i))
			bright[i] = 2047;
		else
			bright[i] = 0;
	}

	lights_set_pwm(bright);
}

static void cull_timer(int led)
{
	int tim = leds[led].timer;
	timer_refmask[tim] &= ~(1 << led);
	if (timer_refmask[tim] == 0)
		rcc_periph_clock_disable(timer_rcc[tim]);
}

void lights_set_pwm(uint32_t bright[])
{
	for (int i = 0; i < ledcount; i++)
	{
		uint32_t thisbright = bright[i];
		thisbright = (thisbright * p_curlimit) / 2048;
		thisbright = (thisbright * thisbright) / 2048;

		//int thisbright = bright[i];

		if (thisbright == 0)
		{
			gpio_clear(leds[i].port, leds[i].num);
//			gpio_mode_setup(leds[i].port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, leds[i].num);
			gpio_mode_setup(leds[i].port, GPIO_MODE_INPUT, GPIO_PUPD_NONE, leds[i].num);

			cull_timer(i);
		}
		else if (thisbright == 2047)
		{
			gpio_set(leds[i].port, leds[i].num);
			gpio_mode_setup(leds[i].port, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, leds[i].num);

			cull_timer(i);
		}
		else
		{
			int tim = leds[i].timer;

			uint32_t tb = timer_base[tim];
			rcc_periph_clock_enable(timer_rcc[tim]);
			timer_set_oc_mode(tb, leds[i].oc, TIM_OCM_PWM1);
			timer_enable_oc_preload(tb, leds[i].oc);
			timer_enable_oc_output(tb, leds[i].oc);
//			timer_generate_event(timer_base[i], TIM_EGR_UG);

			timer_set_oc_value(tb, leds[i].oc, thisbright);

			timer_refmask[tim] |= (1 << i);

			gpio_mode_setup(leds[i].port, GPIO_MODE_AF, GPIO_PUPD_NONE, leds[i].num);
			gpio_set_output_options(leds[i].port, GPIO_OTYPE_PP, GPIO_OSPEED_LOW, leds[i].num);
			gpio_set_af(leds[i].port, leds[i].af, leds[i].num);

		}
	}
}


void lights_kill(void)
{
	for (int i = 0; i < ledcount; i++)
	{
		rcc_periph_clock_enable(leds[i].rcc);
		gpio_mode_setup(leds[i].port, GPIO_MODE_INPUT, GPIO_PUPD_NONE, leds[i].num);

//		rcc_periph_clock_disable(leds[i].rcc);
	}

	for (int i = 0; i < 18; i++)
	{
		if (timers[i])
		{
			rcc_periph_clock_disable(timer_rcc[i]);
		}
	}

}
