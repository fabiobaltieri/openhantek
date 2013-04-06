#ifndef HARDCONTROL_H
#define HARDCONTROL_H

#include <QThread>

#include <libusb.h>

class DsoSettings;
class QMutex;

class HardControl : public QThread {
	Q_OBJECT

	public:
		HardControl(DsoSettings *settings);
		~HardControl();

	protected:
		DsoSettings *settings;

	private:
		void run();
		int read_intr_in();
		void process_event(struct panel_event *evt);
		int send_set_led(int led_mask, int led_value);

		int led_value;

		libusb_context *context;
		libusb_device_handle *handle;

		QMutex *usb_lock;

	public slots:
		void updateLEDs();
		void started();
		void stopped();

	protected slots:

	signals:
		void new_event(int type, int value);
};

#endif
