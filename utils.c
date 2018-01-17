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

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/pwr.h>
#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/exti.h>

#include <stdio.h>

#include "utils.h"
#include "debugf.h"

void initrtc()
{
	debugf("rtc setup\r\n");

	rcc_periph_clock_enable(RCC_PWR);
	rcc_periph_reset_pulse(RST_PWR); // not necessary
	pwr_disable_backup_domain_write_protect();

	rcc_osc_on(RCC_LSI);
	rcc_wait_for_osc_ready(RCC_LSI);
	rcc_set_rtc_clock_source(RCC_LSI);
	rcc_enable_rtc_clock();

	rtc_unlock();
	exti_set_trigger(EXTI20, EXTI_TRIGGER_RISING);
	exti_enable_request(EXTI20);
	nvic_enable_irq(NVIC_RTC_IRQ);
	nvic_set_priority(NVIC_RTC_IRQ, 0);
	RTC_CR |= RTC_CR_WUTIE;
	rtc_lock();

	rtc_clear_wakeup_flag();
}

volatile bool g_irq_flag = false;

void rtc_isr(void)
{
	g_irq_flag = true;
	RTC_CR &= ~RTC_CR_WUTE;
	rtc_clear_wakeup_flag();
	exti_reset_request(EXTI20);
}

void wait_for_irq()
{
	#if 1
		__WFI();
	#else
		g_irq_flag = false;
		while(!g_irq_flag)
			continue;
	#endif
}

void sleep_cpu(int time)
{
	rtc_unlock();
	rtc_set_wakeup_time(time, RTC_CR_WUCLKSEL_RTC_DIV8);
	rtc_lock();

	wait_for_irq();

}

void sleep_stop(int time)
{
	rtc_unlock();
	rtc_set_wakeup_time(time, RTC_CR_WUCLKSEL_RTC_DIV8);

	SCB_SCR |= SCB_SCR_SLEEPDEEP;
	pwr_set_stop_mode();
	pwr_voltage_regulator_low_power_in_stop();
	rtc_lock();

	wait_for_irq();
}

void sleep_standby(int time)
{
	rtc_unlock();
	rtc_set_wakeup_time(time, RTC_CR_WUCLKSEL_RTC_DIV16);

	SCB_SCR |= SCB_SCR_SLEEPDEEP;
	pwr_set_standby_mode();
	pwr_clear_wakeup_flag();
	rtc_lock();

	wait_for_irq();

	scb_reset_system();

}



void crapdelay(int aika)
{
	for (int i = 0; i < aika; i++)
	{	
		__asm__("nop");
	}
}


static uint32_t seed = 7;  // 100% random seed value

uint32_t random_getseed()
{
	return seed;
}

void random_setseed(uint32_t newseed)
{
	if (newseed == 0)
		seed = 7;
	else
		seed = newseed;

		debugf("random seed: %lu\r\n", seed);

}

uint32_t random()
{
  seed ^= seed << 13;
  seed ^= seed >> 17;
  seed ^= seed << 5;
  return seed;
}

#define STM32_UDID ((uint32_t *)0x1FFFF7AC)

char usb_serial[9];
void init_usbserial()
{
	uint32_t robbo = 0;
	robbo += STM32_UDID[0];
	robbo += STM32_UDID[1];
	robbo += STM32_UDID[2];

	snprintf(usb_serial, 9, "%08lx", robbo);
	usb_serial[8] = 0;
}

uint8_t usbd_control_buffer[BULK_EP_MAXPACKET*5];
