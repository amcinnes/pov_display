#include <LUFA/Drivers/USB/USB.h>
#include <stdlib.h>
#include <avr/power.h>
#include <avr/io.h>

uint8_t buf[10];
// Index of the byte currently being sent, or about to be sent
volatile uint8_t buf_sendpos = 0;
volatile uint8_t buf_length = 0;
volatile uint16_t step = 0;

// At 32 carriers per bit, we're sending at about 1200 baud
// and a byte (8N1) takes 320 carriers, which is 640 interrupts.

int main(void) {
	clock_prescale_set(clock_div_1);

	// Set up outputs
	DDRC = 0xc0;

	// Set up timer interrupt
	TCCR0A = 0x02;
	TCCR0B = 0x02; // 2MHz
	TIMSK0 = 0x02;
	OCR0A = 25; // Gives 1000/13 =~ 77 kHz

    sei();

	USB_Init();
    while (1) {
		USB_USBTask();
	}
}

void start_sending(int length) {
	buf_sendpos = 0;
	buf_length = length;
}

void EVENT_USB_Device_ControlRequest(void) {
    if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_VENDOR | REQREC_INTERFACE)) {
		// Set frequency and length
		buf[0] = USB_ControlRequest.bRequest;
		Endpoint_ClearSETUP();
		start_sending(1);
		while (buf_sendpos<1) {}
		Endpoint_ClearIN();
	}
}

ISR(TIMER0_COMPA_vect) {
	if (buf_sendpos==buf_length) {
		PORTC = 0x00;
		return;
	}
	uint8_t bit = 1;
	uint8_t b = buf[buf_sendpos];
	uint8_t i = step/64;
	if (i==0) bit = 0; // start bit
	else if (i>=9) bit = 1; // stop bit
	else bit = ((b>>(i-1))&1);

	if (!bit&&(step&1)) PORTC = 0x80;
	else PORTC = 0x00;

	step++;
	if (step==640) {
		step = 0;
		buf_sendpos++;
	}
}
