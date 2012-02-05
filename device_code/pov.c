#include <stdlib.h>
#include <string.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/sleep.h>
#include <avr/pgmspace.h>
#include "font.c"

#define PIXELS 250
#define PIXELWIDTH 2
#define LENGTH PIXELS*PIXELWIDTH

PROGMEM const int8_t offset[] = { 0, 7, 0, 7, 0, 6, 0, 6, 0, 5, 0, 4, 0, 4, 0, 4 };

#define MAXTEXT 80

char text[MAXTEXT];
const char default_text[] PROGMEM = "Hello, World!";
uint8_t textlen;

volatile uint16_t pattern[LENGTH];
// Current position in pattern
volatile uint16_t pos = 0;

volatile uint8_t need_update = 0;
uint8_t in_packet = 0;

//uint16_t count;

void update_text(void) {
	memset(pattern,0,LENGTH*sizeof(uint16_t));
	uint16_t j = 0;
	uint16_t k;
	uint8_t row;
	uint8_t i,c;
	uint8_t l;
	uint16_t index1, len, pixels;
	int16_t subpixel;
	// For each character in the text
	for (i=0;i<textlen;i++) {
		c = text[i];
		if (c<' '||c>0x7e) c = '_';
		c -= ' ';
		// Get its index and length (# pixel columns) from the font data
		index1 = pgm_read_word(&font_index[c]);
		len = pgm_read_word(&font_index[c+1])-index1;
		// If we've reached the last column of the display, cut off the character at that column
		if (len>PIXELS-j) len = PIXELS-j;
		// For each column in the character
		for (k=0;k<len;k++) {
			// Read pixel data
			pixels = pgm_read_word(&font_data[index1+k]);
			// For each row in this column
			for (row=0;row<16;row++) {
				// Set the appropriate subpixels
				if (pixels&(0x8000>>row)) {
					for (l=0;l<PIXELWIDTH;l++) {
						subpixel = j*PIXELWIDTH-pgm_read_byte(&offset[row])+l;
						if (subpixel<0) subpixel+=LENGTH;
						if (subpixel>=LENGTH) subpixel-=LENGTH;
						pattern[subpixel] |= 1<<row;
					}
				}
			}
			j++;
		}
	}
}

int main(void) {
	clock_prescale_set(clock_div_1);

	// Set up outputs
	DDRB = 0xff;
	DDRC = 0xc0;
	DDRD = 0xcb;
	DDRF = 0x83;
	PORTD = 0x04; // Pull-up resistor for IR receiver
	PORTF = 0x02; // Vcc for IR receiver

	// Set initial text
	textlen = strlen_P(default_text);
	strncpy_P(text,default_text,MAXTEXT);
	update_text();
	textlen = MAXTEXT; // block further input until start of new packet

	// Set up timer interrupt
	TCCR0A = 0x02;
	TCCR0B = 0x05; // 15625 Hz
	TIMSK0 = 0x02;
	OCR0A = 4; // Gives 15625/5 = 3125Hz

	// Set up UART
	UBRR1H = 832>>8;
	UBRR1L = 832&0xff;
	UCSR1B = 0x90;

    sei();

    while (1) {
		sleep_mode();
		if (need_update) {
			need_update = 0;
			update_text();
		}
	}
}

ISR(TIMER0_COMPA_vect) {
	/*
	// Scale frequency down so I can see what it's doing
	count++;
	if (count<3125) return;
	count = 0;
	*/
	// Write pattern[pos] to ports
	uint8_t a = pattern[pos]>>8;
	uint8_t b = pattern[pos]&0xff;
	// From LSB to MSB:
	// B0, B1, B2, B3, B7, D0, D1, F0, D3, C6, C7, D7, B4, B5, B6, F7
	PORTB = (b&0x0f) | ((b&0x10)<<3) | (a&0x70);
	PORTC = (a&0x06)<<5;
	PORTD = ((b&0x60)>>5) | ((a&0x01)<<3) | ((a&0x08)<<4) | 0x04 | (PORTD&0x40);
	PORTF = (a&0x80) | ((b&0x80)>>7) | 0x02;
	pos++;
	if (pos==LENGTH) pos = 0;
}

ISR(USART1_RX_vect) {
	uint8_t byte = UDR1;
	if (byte==0x00) {
		if (in_packet) {
			PORTD &= ~0x40;
			need_update = 1;
			in_packet = 0;
		}
	} else if (byte==0xa5) { // magic number
		in_packet = 1;
		textlen = 0;
		PORTD |= 0x40;
	} else if (textlen<MAXTEXT) {
		text[textlen++] = byte;
	}
}
