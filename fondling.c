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

#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

#include "fondling.h"
/*
	LEFT = PB10 = USART3_TX
	DOWN = PB11 = USART3_RX
*/

void fondling_usart_setup(void)
{
	rcc_periph_clock_enable(RCC_USART4);

	rcc_periph_clock_enable(RCC_GPIOA);


//	/* Setup USART2 TX pin as alternate function. */
//	gpio_set_af(GPIOB, GPIO_AF4, GPIO10);
//	/* Setup GPIO pins for USART2 transmit. */
//	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11);

	/* Setup GPIO pins for USART2 transmit. */
//	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10);
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_PULLUP, GPIO0);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_OD, GPIO_OSPEED_2MHZ, GPIO0);
	/* Setup USART1 TX pin as alternate function. */
	gpio_set_af(GPIOA, GPIO_AF4, GPIO0);


	/* Setup USART2 parameters. */
	usart_set_baudrate(USART4, 9600);
	usart_set_databits(USART4, 8);
	usart_set_parity(USART4, USART_PARITY_NONE);
	usart_set_stopbits(USART4, USART_STOPBITS_1);
	usart_set_mode(USART4, USART_MODE_TX_RX);
	usart_set_flow_control(USART4, USART_FLOWCONTROL_NONE);
	usart_enable_halfduplex(USART4);

	USART4_CR3 |= USART_CR3_OVRDIS;

	/* Finally enable the USART. */
	usart_enable(USART4);
}


void fondling_usart_kill(void)
{
	rcc_periph_clock_disable(RCC_USART4);
}

bool fondling_data_avail(void)
{
	return USART4_ISR & USART_ISR_RXNE;
}
