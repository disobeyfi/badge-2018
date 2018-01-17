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
0x15, 0x80,        //     Logical Minimum (-128)
0x25, 0x7F,        //     Logical Maximum (127)
0x75, 0x08,        //     Report Size (8)
0x95, 0x02,        //     Report Count (2)
0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x05, 0x09,        //     Usage Page (Button)
0x19, 0x01,        //     Usage Minimum (0x01)
0x29, 0x04,        //     Usage Maximum (0x04)
0x15, 0x00,        //     Logical Minimum (0)
0x25, 0x01,        //     Logical Maximum (1)
0x95, 0x04,        //     Report Count (4)
0x75, 0x01,        //     Report Size (1)
0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
0x95, 0x01,        //     Report Count (1)
0x75, 0x04,        //     Report Size (4)
0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
0xC0,              //   End Collection
0xC0,              // End Collection

// 48 bytes
};

typedef struct {
		uint8_t axes[2];
		uint8_t buttons;
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
	"Badge Gamepad",
	usb_serial,
};

/* Buffer to be used for control requests. */
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

static void device_work(void)
{
	hidmessage_t msg;

	int buts = buttons_read();

	uint8_t xx = 0;
	if (buts & BUTTON(6))
		xx = 0x80;
	if (buts & BUTTON(7))
		xx = 0x7F;

	uint8_t yy = 0;
	if (buts & BUTTON(4))
		yy = 0x80;
	if (buts & BUTTON(5))
		yy = 0x7f;

	msg.axes[0] = xx;
	msg.axes[1] = yy;

	uint8_t butdata = 0;
	if (buts & BUTTON(0))
		butdata |= 1;
	if (buts & BUTTON(1))
		butdata |= 2;
	if (buts & BUTTON(2))
		butdata |= 4;
	if (buts & BUTTON(3))
		butdata |= 8;


	msg.buttons = butdata;

	usbd_ep_write_packet(usbd_dev, 0x81, &msg, sizeof(hidmessage_t));
}


static void rescb(void)
{
	//debugf("-usb reset\r\n");

	hack_flag = 0;
}


void gamepad(void)
{
	rcc_clock_setup_in_hsi48_out_48mhz();
	crs_autotrim_usb_enable();
	rcc_set_usbclk_source(RCC_HSI48);

	debug_usart_setup();

	debugf("\r\n\r\nhello. CR2: %08X, serial: %s\r\n", RCC_CR2, usb_strings[2]);

	rcc_periph_clock_enable(RCC_GPIOA);

	debugf("%s: Entered gamepad mode\r\n", __func__);


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
