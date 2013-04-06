#ifndef HARDCONTROL_H
#define HARDCONTROL_H

#include <QThread>

#include <libusb.h>

class DsoSettings;

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

		libusb_context *context;
		libusb_device_handle *handle;

	public slots:

	protected slots:

	signals:
		void new_event(int type, int value);
};

#endif
