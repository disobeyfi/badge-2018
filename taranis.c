/*
 * This file is part of Disobey Badge 2018 firmware.
 *
 * (based on USB example)
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
 *
 * (also uses one Sbus data structure by CleanFlight team!)
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

#include <stdlib.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/hid.h>
#include <libopencm3/stm32/crs.h>
#include <libopencm3/stm32/f0/syscfg.h>
#include <libopencm3/stm32/usart.h>



#include <libopencmsis/core_cm3.h>

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>

#include <libopencm3/stm32/rtc.h>
#include <libopencm3/stm32/pwr.h>

#include <libopencm3/stm32/exti.h>
#include <libopencm3/stm32/usart.h>

#include <libopencm3/stm32/timer.h>

#include <stdbool.h>

#include <math.h>

#include "utils.h" 
#include "lights.h"


#include "debugf.h"





#define BULK_EP_MAXPACKET (64)


static usbd_device *usbd_dev;

static const struct usb_device_descriptor dev_descr = {
	.bLength = USB_DT_DEVICE_SIZE,
	.bDescriptorType = USB_DT_DEVICE,
	.bcdUSB = 0x0200,
	.bDeviceClass = 0,
	.bDeviceSubClass = 0,
	.bDeviceProtocol = 0,
	.bMaxPacketSize0 = BULK_EP_MAXPACKET,
	.idVendor = 0xdead,
	.idProduct = 0xface,
	.bcdDevice = 0x0001,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

static const uint8_t hid_report_descriptor[] = {
0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
0x09, 0x05,        // Usage (Game Pad)
0xA1, 0x01,        // Collection (Application)
0xA1, 0x00,        //   Collection (Physical)
0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
0x09, 0x30,        //     Usage (X)
0x09, 0x31,        //     Usage (Y)
0x09, 0x32,        //     Usage (Z)
0x09, 0x33,        //     Usage (Rx)
0x15, 0x00,        //     Logical Minimum (0)
0x26, 0xFF, 0x07,  //     Logical Maximum (2047)
0x75, 0x10,        //     Report Size (16)
0x95, 0x04,        //     Report Count (4)
0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              //   End Collection
0xC0,              // End Collection

// 73 bytes
};

typedef struct {
		uint16_t axes[4];
	} __attribute__((packed)) hidmessage_t;

static const struct {
	struct usb_hid_descriptor hid_descriptor;
	struct {
		uint8_t bReportDescriptorType;
		uint16_t wDescriptorLength;
	} __attribute__((packed)) hid_report;
} __attribute__((packed)) hid_function = {
	.hid_descriptor = {
		.bLength = sizeof(hid_function),
		.bDescriptorType = USB_DT_HID,
		.bcdHID = 0x0100,
		.bCountryCode = 0,
		.bNumDescriptors = 1,
	},
	.hid_report = {
		.bReportDescriptorType = USB_DT_REPORT,
		.wDescriptorLength = sizeof(hid_report_descriptor),
	}
};

const struct usb_endpoint_descriptor hid_endpoint = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x81,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = sizeof(hidmessage_t),
	.bInterval = 0x20,
};

const struct usb_interface_descriptor hid_iface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_HID,
//	.bInterfaceSubClass = 1, /* boot */
//	.bInterfaceProtocol = 2, /* mouse */
	.bInterfaceSubClass = 0, /* boot */
	.bInterfaceProtocol = 0, /* mouse */

	.iInterface = 0,

	.endpoint = &hid_endpoint,

	.extra = &hid_function,
	.extralen = sizeof(hid_function),
};

const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = &hid_iface,
//	.altsetting = iface_sourcesink,
}};

const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = 2,
	.iConfiguration = 1,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static const char *usb_strings[] = {
	"Disobey Helicopter Gang",
	"SBUS adapter",
	"GULL",
};

/* Buffer to be used for control requests. */
static uint8_t usbd_control_buffer[BULK_EP_MAXPACKET*5];
uint8_t hack_flag = 0;

static int hid_control_request(usbd_device *dev, struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
			void (**complete)(usbd_device *, struct usb_setup_data *))
{
	(void)complete;
	(void)dev;

	debugf("-hid control req: %X %X %X\r\n", req->bmRequestType, req->bRequest, req->wValue);


	if((req->bmRequestType != 0x81) ||
	   (req->bRequest != USB_REQ_GET_DESCRIPTOR) ||
	   (req->wValue != 0x2200))
		return 0;

	/* Handle the HID report descriptor. */
	*buf = (uint8_t *)hid_report_descriptor;
	*len = sizeof(hid_report_descriptor);

	hack_flag = 1;

	return 1;
}

static void hid_set_config(usbd_device *dev, uint16_t wValue)
{
	(void)wValue;
	(void)dev;

	debugf("-hid set config\r\n");

	usbd_ep_setup(dev, 0x81, USB_ENDPOINT_ATTR_INTERRUPT, 4, NULL);

	usbd_register_control_callback(
				dev,
				USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				hid_control_request);

}

		int sbusChannelData[16];



void device_work(void)
{
	hidmessage_t msg;

#if 0
	int xd = (sbusChannelData[1] - 982) / 40;

	if (xd < -50)
		xd = -50;
	else if (xd > 50)
		xd = 50;


	int yd = -(sbusChannelData[2] - 982) / 40;
	if (yd < -50)
		yd = -50;
	else if (yd > 50)
		yd = 50;

	buf[1] = xd;
	buf[2] = yd;
#endif

	msg.axes[0] = sbusChannelData[0];
	msg.axes[1] = sbusChannelData[3];
	msg.axes[2] = sbusChannelData[1];
	msg.axes[3] = sbusChannelData[2];

	usbd_ep_write_packet(usbd_dev, 0x81, &msg, sizeof(hidmessage_t));
}


void rescb(void)
{
	debugf("usb reset\r\n");
}


int getpwm(float hup)
{
	float arvo = sin(hup*3.14159f*2.f) * 0.2f + 0.3f;
	arvo = arvo*arvo;
	return arvo*2000.f;
}

static int anim2_idx = 0;
void runanim2()
{
		anim2_idx += sbusChannelData[0] + sbusChannelData[1] + sbusChannelData[2] + sbusChannelData[3];
		float fopo = (float)anim2_idx / 500000.f;
		timer_set_oc_value(TIM2, TIM_OC2, getpwm(fopo+0.f));
		timer_set_oc_value(TIM2, TIM_OC3, getpwm(fopo-0.2f));
		timer_set_oc_value(TIM2, TIM_OC4, getpwm(fopo-0.4f));

		if (anim2_idx & 8 || true)
			gpio_set(GPIOA, GPIO8);
		else
			gpio_clear(GPIOA, GPIO8);
}


void anim2()
{
	rcc_periph_clock_enable(RCC_GPIOA);

	// TIM2 for PA1-3

	for (int i = 0; i < 3; i++)
	{
		int hup[3] = {GPIO1, GPIO2, GPIO3};
		gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, hup[i]);
		gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, hup[i]);
		gpio_set_af(GPIOA, GPIO_AF2, hup[i]);
	}

	rcc_periph_clock_enable(RCC_TIM2);
	rcc_periph_reset_pulse(RST_TIM2);

	timer_set_mode(TIM2, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_set_period(TIM2, 2000);
	timer_set_prescaler(TIM2, 0);


	timer_set_oc_mode(TIM2, TIM_OC2, TIM_OCM_PWM1);
	timer_enable_oc_preload(TIM2, TIM_OC2);
	timer_enable_oc_output(TIM2, TIM_OC2);
	timer_set_oc_value(TIM2, TIM_OC2, 0);

	timer_set_oc_mode(TIM2, TIM_OC3, TIM_OCM_PWM1);
	timer_enable_oc_preload(TIM2, TIM_OC3);
	timer_enable_oc_output(TIM2, TIM_OC3);
	timer_set_oc_value(TIM2, TIM_OC3, 0);

	timer_set_oc_mode(TIM2, TIM_OC4, TIM_OCM_PWM1);
	timer_enable_oc_preload(TIM2, TIM_OC4);
	timer_enable_oc_output(TIM2, TIM_OC4);
	timer_set_oc_value(TIM2, TIM_OC4, 0);


	timer_generate_event(TIM2, TIM_EGR_UG);

	timer_enable_preload(TIM2);
	timer_enable_counter(TIM2);







#if 0
	// TIM1 for PA8
	gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO8);
	gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, GPIO8);
	gpio_set_af(GPIOA, GPIO_AF2, GPIO8);

	rcc_periph_clock_enable(RCC_TIM1);
	rcc_periph_reset_pulse(RST_TIM1);

	timer_set_mode(TIM1, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
	timer_set_period(TIM1, 2000);
	timer_set_prescaler(TIM1, 0);


	timer_set_oc_mode(TIM1, TIM_OC1, TIM_OCM_PWM1);
	timer_enable_oc_preload(TIM1, TIM_OC1);
	timer_enable_oc_output(TIM1, TIM_OC1);
	timer_set_oc_value(TIM1, TIM_OC1, 0);

	timer_generate_event(TIM1, TIM_EGR_UG);

	timer_enable_preload(TIM1);
	timer_enable_counter(TIM1);
#else
	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO8);

#endif


}


static void sbus_setup(void)
{
	rcc_periph_clock_enable(RCC_USART3);

	rcc_periph_clock_enable(RCC_GPIOB);

	rcc_periph_reset_pulse(RST_USART3);



//	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO10);
//	gpio_set_af(GPIOB, GPIO_AF4, GPIO10);

	gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO11);
	gpio_set_af(GPIOB, GPIO_AF4, GPIO11);


	usart_set_baudrate(USART3, 100000);
	usart_set_databits(USART3, 9);
	usart_set_parity(USART3, USART_PARITY_EVEN);
	usart_set_stopbits(USART3, USART_STOPBITS_2);
	usart_set_mode(USART3, USART_MODE_RX);
	usart_set_flow_control(USART3, USART_FLOWCONTROL_NONE);
//	usart_enable_data_inversion(USART3);
	usart_enable_rx_inversion(USART3);

//	USART3_CR1 &= ~USART_CR1_OVER8;

	nvic_enable_irq(NVIC_USART3_4_IRQ);
	USART_CR1(USART3) |= USART_CR1_RXNEIE;
	//USART_CR2(USART3) |= USART_CR2_MSBFIRST;

	usart_enable(USART3);
}



#define SBUS_FRAME_SIZE 25

struct sbusFrame_s {
    uint8_t syncByte;
    // 176 bits of data (11 bits per channel * 16 channels) = 22 bytes.
    unsigned int chan0 : 11;
    unsigned int chan1 : 11;
    unsigned int chan2 : 11;
    unsigned int chan3 : 11;
    unsigned int chan4 : 11;
    unsigned int chan5 : 11;
    unsigned int chan6 : 11;
    unsigned int chan7 : 11;
    unsigned int chan8 : 11;
    unsigned int chan9 : 11;
    unsigned int chan10 : 11;
    unsigned int chan11 : 11;
    unsigned int chan12 : 11;
    unsigned int chan13 : 11;
    unsigned int chan14 : 11;
    unsigned int chan15 : 11;
    uint8_t flags;
    /**
     * The endByte is 0x00 on FrSky and some futaba RX's, on Some SBUS2 RX's the value indicates the telemetry byte that is sent after every 4th sbus frame.
     *
     * See https://github.com/cleanflight/cleanflight/issues/590#issuecomment-101027349
     * and
     * https://github.com/cleanflight/cleanflight/issues/590#issuecomment-101706023
     */
    uint8_t endByte;
} __attribute__ ((__packed__));

typedef union {
    uint8_t bytes[SBUS_FRAME_SIZE];
    struct sbusFrame_s frame;
} sbusFrame_t;

static sbusFrame_t sbusFrame;

void sbus_decode()
{

    sbusChannelData[0] = sbusFrame.frame.chan0;
    sbusChannelData[1] = sbusFrame.frame.chan1;
    sbusChannelData[2] = sbusFrame.frame.chan2;
    sbusChannelData[3] = sbusFrame.frame.chan3;
    sbusChannelData[4] = sbusFrame.frame.chan4;
    sbusChannelData[5] = sbusFrame.frame.chan5;
    sbusChannelData[6] = sbusFrame.frame.chan6;
    sbusChannelData[7] = sbusFrame.frame.chan7;
    sbusChannelData[8] = sbusFrame.frame.chan8;
    sbusChannelData[9] = sbusFrame.frame.chan9;
    sbusChannelData[10] = sbusFrame.frame.chan10;
    sbusChannelData[11] = sbusFrame.frame.chan11;
    sbusChannelData[12] = sbusFrame.frame.chan12;
    sbusChannelData[13] = sbusFrame.frame.chan13;
    sbusChannelData[14] = sbusFrame.frame.chan14;
    sbusChannelData[15] = sbusFrame.frame.chan15;
}

void sbus_dump()
{
		debugf("\e[1;1H");
#if 1
		for (int i = 0; i < 25; i++)
		{
			debugf("%i:%02X ", i,sbusFrame.bytes[i]);
		}
		debugf("\r\n");

		for (int i = 0; i < 25; i++)
		{
			for (int bt = 0; bt < 8; bt++)
			{
				int a = (sbusFrame.bytes[i] << bt) & 128 ? '1' : '0';
				debugf("%c", a);
			}
			debugf(" ");
		}
		debugf("\r\n\r\n");
#endif

#if 0
		for (int i = 0; i < 16; i++)
		{
			int val = 0;
			for (int bt = 0; bt < 11; bt++)
			{
				int bit = i*11+bt;
				int ic = bit / 8;
				int btc = bit & 7;

				int a = (frame[ic+1] >> btc) & 1 ? 1 : 0;
				val |= a << (bt);
			}
//			val -= 1024;
			debugf("\e[6;%iH%i=%i  ", i*9, i, val);
		}
#endif

    debugf("start: %02X, flags %02X, end %02X\n", sbusFrame.frame.syncByte, sbusFrame.frame.flags, sbusFrame.frame.endByte);
		for (int i = 0; i < 16; i++)
		{
			debugf("\e[6;%iH%i=%i  ", i*9, i, sbusChannelData[i]);
		}


		debugf("\r\n\r\n");
}



void sbus_test()
{
	debugf("\e[2J\e[1;1H");

	while(1)
	{
		lights_set(0xFF);
		int sync = 0;
		while(1)
		{
			int cunt = 0;
			while ((USART3_ISR & USART_ISR_RXNE) == 0)
			{
				cunt++;
				if (cunt > 100000)
				{
					debugf("ERROR: %08X\r\n", USART3_ISR);
					//while(1)
					//	continue;

					//USART3_ICR |= ~0;
					USART3_ICR |= USART_ICR_ORECF;
				}
			}
			int byte = usart_recv(USART3) & 0xFF;
			if (sync == 0 && byte == 0x3e)
				sync = 1;
			else if (sync == 1 && byte == 0xf0)
				break;
			else
				sync = 0;
		}
		lights_set(0x0);


		sbusFrame.bytes[0] = 0xf0;
		for (int i = 1; i < 25; i++)
		{
			if (USART3_ISR & USART_ISR_ORE)
			{
				debugf("ERROR\r\n");
				while(1)
					continue;
			}
			if (USART3_ISR & USART_ISR_PE)
			{
				debugf("PARITY ERROR\r\n");
				while(1)
					continue;
			}


			int byte = usart_recv_blocking(USART3) & 0xff;
			sbusFrame.bytes[i] = byte;
		}


		sbus_dump();

//		crapdelay(1000000);
		usart_recv(USART3);
		USART3_ICR |= USART_ICR_ORECF;

	}
}

volatile int sbus_recv = 0;
volatile int sbus_index = 0;

void usart3_4_isr(void)
{
	if (USART3_ISR & USART_ISR_RXNE)
	{
		int data = usart_recv(USART3);

		if (sbus_recv == 0 && data == 0x0F)
		{
			sbus_recv = 1;
			sbus_index = 0;
		}

		if (sbus_recv == 1)
		{
			sbusFrame.bytes[sbus_index] = data;
			sbus_index++;

			if (sbus_index == 25)
			{
				sbus_recv = 2;
			}
		}
	}
}

int main(void)
{


//	rcc_clock_setup_in_hsi_out_48mhz();

//	rcc_clock_setup_in_hsi48_out_48mhz();
//	rcc_periph_clock_enable(RCC_SYSCFG_COMP);
//	rcc_system_clock_source(RCC_HSI48);

	rcc_clock_setup_in_hsi48_out_48mhz();
	crs_autotrim_usb_enable();
	rcc_set_usbclk_source(RCC_HSI48);

	debug_usart_setup();

	moi:

	debugf("\r\n\r\nhello. CR2: %08X\r\n", RCC_CR2);






	anim2();

	rcc_periph_clock_enable(RCC_GPIOA);
	/*
	 * This is a somewhat common cheap hack to trigger device re-enumeration
	 * on startup.  Assuming a fixed external pullup on D+, (For USB-FS)
	 * setting the pin to output, and driving it explicitly low effectively
	 * "removes" the pullup.  The subsequent USB init will "take over" the
	 * pin, and it will appear as a proper pullup to the host.
	 * The magic delay is somewhat arbitrary, no guarantees on USBIF
	 * compliance here, but "it works" in most places.
	 */
//	gpio_mode_setup(GPIOA, GPIO_OSPEED_LOW,
//		GPIO_PUPD_PULLUP, GPIO11);
////	gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, GPIO11);

//	gpio_clear(GPIOA, GPIO11);
	for (unsigned i = 0; i < 800000; i++) {
		__asm__("nop");
	}

	debugf("uus urhea usbtest\r\n");



	usbd_dev = usbd_init(
		&st_usbfs_v2_usb_driver,
		&dev_descr,
		&config,
		usb_strings, 3,
		usbd_control_buffer,
		sizeof(usbd_control_buffer)
	);


	usbd_register_set_config_callback(usbd_dev, hid_set_config);
	usbd_register_reset_callback(usbd_dev, rescb);

	while(!hack_flag)
		usbd_poll(usbd_dev);


//	lights_setup();
	sbus_setup();


//	usbd_register_sof_callback(usbd_dev, wiggle);

	debugf("\e[2J\e[1;1H");

	while(1)
	{
		usbd_poll(usbd_dev);

//		debugf("moi\n");
		if (sbus_recv == 2)
		{
			sbus_decode();
			sbus_recv = 0;
		//	sbus_dump();
			device_work();
			runanim2();
		}
	}
//	sbus_test();


	while(1)
		wiggle();
}
