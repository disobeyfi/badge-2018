/*
 * This file is part of Disobey Badge 2018 firmware.
 *
 * (based on USB example)
 * Copyright (C) 2010 Gareth McMullin <gareth@blacksphere.co.nz>
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

#include <stdio.h>
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

#include <string.h>

#include "utils.h" 
#include "lights.h"
#include "buttons.h"

#include "debugf.h"


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
	.idProduct = 0xcafe,
	.bcdDevice = 0x0001,
	.iManufacturer = 1,
	.iProduct = 2,
	.iSerialNumber = 3,
	.bNumConfigurations = 1,
};

static const uint8_t hid_report_descriptor[] = {
0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
0x09, 0x06,        // Usage (Keyboard)
0xA1, 0x01,        // Collection (Application)
0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
0x95, 0x08,        //   Report Count (8)
0x75, 0x08,        //   Report Size (8)
0x15, 0x00,        //   Logical Minimum (0)
0x26, 0xFF, 0x00,  //   Logical Maximum (255)
0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
0x19, 0x00,        //   Usage Minimum (0x00)
0x2A, 0xFF, 0x00,  //   Usage Maximum (0xFF)
0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              // End Collection

// 27 bytes
};

typedef struct {
		uint8_t buttons[8];
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

static const struct usb_endpoint_descriptor hid_endpoint = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = 0x81,
	.bmAttributes = USB_ENDPOINT_ATTR_INTERRUPT,
	.wMaxPacketSize = sizeof(hidmessage_t),
	.bInterval = 0x20,
};

static const struct usb_interface_descriptor hid_iface = {
	.bLength = USB_DT_INTERFACE_SIZE,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = USB_CLASS_HID,
//	.bInterfaceSubClass = 1, /* boot */
//	.bInterfaceProtocol = 2, /* mouse */
	.bInterfaceSubClass = 0,
	.bInterfaceProtocol = 0,

	.iInterface = 0,

	.endpoint = &hid_endpoint,

	.extra = &hid_function,
	.extralen = sizeof(hid_function),
};

static const struct usb_interface ifaces[] = {{
	.num_altsetting = 1,
	.altsetting = &hid_iface,
//	.altsetting = iface_sourcesink,
}};

static const struct usb_config_descriptor config = {
	.bLength = USB_DT_CONFIGURATION_SIZE,
	.bDescriptorType = USB_DT_CONFIGURATION,
	.wTotalLength = 0,
	.bNumInterfaces = 1,
	.bConfigurationValue = 2,
	.iConfiguration = 2,
	.bmAttributes = 0x80,
	.bMaxPower = 0x32,

	.interface = ifaces,
};

static char * const usb_strings[] = {
	"Disobey",
	"Badge Keyboard",
	usb_serial,
};

static uint8_t hack_flag = 0;

static int hid_control_request(usbd_device *dev, struct usb_setup_data *req, uint8_t **buf, uint16_t *len,
			void (**complete)(usbd_device *, struct usb_setup_data *))
{
	(void)complete;
	(void)dev;

	//debugf("-hid control req: %X %X %X\r\n", req->bmRequestType, req->bRequest, req->wValue);


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

	//debugf("-hid set config\r\n");

	usbd_ep_setup(dev, 0x81, USB_ENDPOINT_ATTR_INTERRUPT, 4, NULL);

	usbd_register_control_callback(
				dev,
				USB_REQ_TYPE_STANDARD | USB_REQ_TYPE_INTERFACE,
				USB_REQ_TYPE_TYPE | USB_REQ_TYPE_RECIPIENT,
				hid_control_request);

}

static const uint8_t asciitohid[] =  {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x2A, 0x00, 0x00, 0x00, 0x00, 0x28, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
	0x00, 0x00, 0x00, 0x29, 0x00, 0x00, 0x00, 0x00, 
	0x2c, 0x1e, 0x34, 0x20, 0x21, 0x22, 0x24, 0x34, 
	0x26, 0x27, 0x25, 0x2e, 0x36, 0x2d, 0x37, 0x38, 
	0x27, 0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 
	0x25, 0x26, 0x33, 0x33, 0x36, 0x2e, 0x37, 0x38,
	0x1f, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
	0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12,
	0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
	0x1b, 0x1c, 0x1d, 0x2f, 0x31, 0x30, 0x23, 0x2d,
	0x35, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a,
	0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12,
	0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a,
	0x1b, 0x1c, 0x1d, 0x2f, 0x31, 0x30, 0x35, 127
};


static int code_timer = 0;
static int code_pos = 0;

static uint8_t hid_code[80];
static int hid_code_len = 0;

static void make_hid_code(int keybits, const char *keystring)
{

	char viesti[80];
	hid_code_len = sprintf(viesti, "collected keys 0x%X of 0x1FF. %s", keybits, keystring);

	memset(hid_code, 0, hid_code_len);
	for (int i = 0; i < hid_code_len; i++)
	{
		hid_code[i] = asciitohid[(int)viesti[i]];
	}


}

static void device_work(void)
{
	hidmessage_t msg;

	const int hid_rightarrow = 0x4f;
	const int hid_leftarrow = 0x50;
	const int hid_downarrow = 0x51;
	const int hid_uparrow = 0x52;

	if (code_pos < hid_code_len)
	{
		memset(msg.buttons, 0, 8);
		if (code_timer < 14000)
		{

		}
		else if (code_timer < 20000)
		{
			msg.buttons[0] = hid_code[code_pos];
		}
		else
		{
			code_timer = 0;
			code_pos++;
		}

		code_timer++;
	}
	else
	{
		int buts = buttons_read();
		msg.buttons[0] = buts & BUTTON(4) ? hid_uparrow : 0;
		msg.buttons[1] = buts & BUTTON(5) ? hid_downarrow : 0;
		msg.buttons[2] = buts & BUTTON(6) ? hid_leftarrow : 0;
		msg.buttons[3] = buts & BUTTON(7) ? hid_rightarrow : 0;

		msg.buttons[4] = buts & BUTTON(0) ? asciitohid['a'] : 0;
		msg.buttons[5] = buts & BUTTON(1) ? asciitohid['s'] : 0;
		msg.buttons[6] = buts & BUTTON(2) ? asciitohid['z'] : 0;
		msg.buttons[7] = buts & BUTTON(3) ? asciitohid['x'] : 0;
	}

	usbd_ep_write_packet(usbd_dev, 0x81, &msg, sizeof(hidmessage_t));
}


static void rescb(void)
{
	//debugf("-usb reset\r\n");
	hack_flag = 0;
}


void keyboard(int keybits, const char *keystring)
{
	rcc_clock_setup_in_hsi48_out_48mhz();
	crs_autotrim_usb_enable();
	rcc_set_usbclk_source(RCC_HSI48);

	debug_usart_setup();

	debugf("\r\n\r\nhello. CR2: %08X, serial: %s\r\n", RCC_CR2, usb_strings[2]);


	make_hid_code(keybits, keystring);

	rcc_periph_clock_enable(RCC_GPIOA);

	debugf("%s: Entered keyboard mode\r\n", __func__);

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

	buttons_setup();

	while(1)
	{
		usbd_poll(usbd_dev);

		if (hack_flag)
			device_work();

		int powercount;
		for (powercount = 10; powercount > 0 && !gpio_get(GPIOB, GPIO5); powercount--)
		{
			//if (powercount == 10)
			//	debugf("usb power gone?\r\n");
			crapdelay(0x100000);
			//debugf("sp\n");
		}
		if (powercount == 0)
		{
			debugf("yup. usb power gone.\r\n");
			scb_reset_system();
		}
	}
}
