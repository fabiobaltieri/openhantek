#include <stdio.h>
#include <QMutex>

#include "settings.h"
#include "dso.h"

#include "hardcontrol.h"
#include "requests.h"

#define VENDOR_ID 0x4242
#define PRODUCT_ID 0xe131
#define EP_IN (LIBUSB_ENDPOINT_IN | 1)

#define printf_dbg(a...)

#if 0
static const char *events[PANEL_COUNT] = {
	[PANEL_NONE] = "empty",
	[PANEL_SW_R_RUN_STOP] = "run/stop",
	[PANEL_SW_R_MODE] = "run mode",
	[PANEL_SW_R_DEFAULT] = "default",
	[PANEL_SW_H_TIMEBASE] = "timebase sw",
	[PANEL_SW_H_DELAY] = "delay sw",
	[PANEL_SW_C_POSITION] = "cursor position sw",
	[PANEL_SW_C_MODE] = "cursor mode",
	[PANEL_SW_T_EDGE] = "trigger edge",
	[PANEL_SW_T_SOURCE] = "trigger source",
	[PANEL_SW_T_MANUAL] = "trigger manual",
	[PANEL_SW_T_POSITION] = "trigger position sw",
	[PANEL_SW_CH1_ENABLE] = "ch1 enable",
	[PANEL_SW_CH1_SCALE] = "ch1 scale sw",
	[PANEL_SW_CH1_OFFSET] = "ch1 offset sw",
	[PANEL_SW_CH2_ENABLE] = "ch2 enable",
	[PANEL_SW_CH2_SCALE] = "ch2 scale sw",
	[PANEL_SW_CH2_OFFSET] = "ch2 offset sw",

	[PANEL_ENC_H_TIMEBASE] = "timebase enc",
	[PANEL_ENC_H_DELAY] = "delay enc",
	[PANEL_ENC_C_POSITION] = "cursor position",
	[PANEL_ENC_T_POSITION] = "trigger position",
	[PANEL_ENC_CH1_SCALE] = "ch1 scale",
	[PANEL_ENC_CH1_OFFSET] = "ch1 offset",
	[PANEL_ENC_CH2_SCALE] = "ch2 scale",
	[PANEL_ENC_CH2_OFFSET] = "ch2 offset",
};
#endif

HardControl::HardControl(DsoSettings *settings) : QThread()
{
	int ret;

	this->settings = settings;
	this->usb_lock = new QMutex();
	this->led_value = 0x0000;
	this->handle = NULL;

	ret = libusb_init(&context);
	if (ret < 0) {
		printf("libusb_init: %s\n", libusb_error_name(ret));
	}

	start();
}

HardControl::~HardControl()
{
	libusb_exit(context);
}

int HardControl::send_set_led(int led_mask, int led_value)
{
	int ret;

	usb_lock->lock();

	if (!handle) {
		usb_lock->unlock();
		return -1;
	}

	led_mask &= LED_ALL;
	led_value &= LED_ALL;

	printf_dbg("Sending SET_LED request: mask=%04x, value=%04x\n",
			led_mask, led_value);

	ret = libusb_control_transfer(handle,
			LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE |
			LIBUSB_ENDPOINT_IN,
			CUSTOM_RQ_SET_LED,
			led_mask, led_value, NULL, 0, 1000);

	if (ret < 0) {
		printf("libusb_control_transfer: %s\n", libusb_error_name(ret));
	}

	usb_lock->unlock();

	return ret;
}

void HardControl::updateLEDs()
{
	led_value &= ~(LED_MODE | LED_TRIGGER | LED_CH1 | LED_CH2);

	/* Run mode */

	switch (settings->scope.trigger.mode) {
	case Dso::TRIGGERMODE_AUTO:
		led_value |= (1 << LED_R_AUTO);
		break;
	case Dso::TRIGGERMODE_NORMAL:
		led_value |= (1 << LED_R_NORMAL);
		break;
	case Dso::TRIGGERMODE_SINGLE:
		led_value |= (1 << LED_R_SINGLE);
		break;
	default:
		break;
	}

	/* Trigger */

	switch (settings->scope.trigger.slope) {
	case Dso::SLOPE_POSITIVE:
		led_value |= (1 << LED_T_RISING);
		break;
	case Dso::SLOPE_NEGATIVE:
		led_value |= (1 << LED_T_FALLING);
		break;
	default:
		break;
	}

	if (!settings->scope.trigger.special) {
		switch (settings->scope.trigger.source) {
		case 0:
			led_value |= (1 << LED_CH1_TRIG);
			break;
		case 1:
			led_value |= (1 << LED_CH2_TRIG);
			break;
		default:
			break;
		}
	}

	/* Channel 1 */

	if (settings->scope.voltage[0].used)
		led_value |= (1 << LED_CH1_ON);

	switch (settings->scope.voltage[0].misc) {
	case Dso::COUPLING_AC:
		led_value |= (1 << LED_CH1_AC);
		break;
	case Dso::COUPLING_GND:
		led_value |= (1 << LED_CH1_50OHM);
		break;
	}

	/* Channel 2 */

	if (settings->scope.voltage[1].used)
		led_value |= (1 << LED_CH2_ON);

	switch (settings->scope.voltage[1].misc) {
	case Dso::COUPLING_AC:
		led_value |= (1 << LED_CH2_AC);
		break;
	case Dso::COUPLING_GND:
		led_value |= (1 << LED_CH2_50OHM);
		break;
	}

	send_set_led(LED_ALL, led_value);
}

void HardControl::started()
{
	led_value &= ~LED_RUN;
	led_value |= (1 << LED_R_RUN);

	send_set_led(LED_ALL, led_value);
}

void HardControl::stopped()
{
	led_value &= ~LED_RUN;
	led_value |= (1 << LED_R_STOP);

	send_set_led(LED_ALL, led_value);
}

void HardControl::process_event(struct panel_event *evt)
{
	if (evt->type && evt->type < PANEL_COUNT) {
		if (evt->type < PANEL_FIRST_ENC && !evt->value)
			return;

		printf_dbg("[%s: %4hhd]\n", events[evt->type], evt->value);

		emit new_event(evt->type, evt->value);
	}
}

int HardControl::read_intr_in()
{
	int ret;
	int len;
	struct panel_data pdata;

	ret = libusb_interrupt_transfer(handle, EP_IN,
			(unsigned char *)&pdata, sizeof(pdata), &len, 5000);
	if (ret < 0) {
		if (ret != LIBUSB_ERROR_TIMEOUT)
			printf("libusb_interrupt_transfer: %s\n", libusb_error_name(ret));
		return ret;
	}

	process_event(&pdata.event[0]);
	process_event(&pdata.event[1]);
	process_event(&pdata.event[2]);
	process_event(&pdata.event[3]);

	return 0;
}

void HardControl::run()
{
	int ret;

	for (;;) {
		printf_dbg("%s: trying to open\n", __func__);

		usb_lock->lock();
		handle = libusb_open_device_with_vid_pid(context,
				VENDOR_ID, PRODUCT_ID);
		usb_lock->unlock();
		if (!handle) {
			printf_dbg("error: could not find USB device %04x:%04x\n",
					VENDOR_ID, PRODUCT_ID);
			msleep(1000);
			continue;
		}

		updateLEDs();

		printf_dbg("%s: connected\n", __func__);

		libusb_claim_interface(handle, 0);

		do {
			ret = read_intr_in();
		} while (ret == 0 || ret == LIBUSB_ERROR_TIMEOUT);

		libusb_release_interface(handle, 0);

		printf_dbg("%s: closing\n", __func__);

		usb_lock->lock();
		libusb_close(handle);
		handle = NULL;
		usb_lock->unlock();

		msleep(1000);
	}
}
