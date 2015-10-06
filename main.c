#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <string.h>
#include "easyavr.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"

#define LED_PIN PINB3
#define LAMP_PIN PINB4

struct BUTTON {
	uint8_t count;
	uint8_t port;
	uint8_t status;
};

#define BUTTONS 2
volatile struct BUTTON buttons[BUTTONS];

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wloop-analysis"
uint16_t get_pad_charge_time(uint8_t pin_bit)
{
	uint16_t tck = 0;

	// Set sns pin to Output
	DDRB |= pin_bit;
	// Set sns pin to 0
	PORTB &= ~pin_bit;

	// Set sns pin to Input
	DDRB &= ~pin_bit;

	while (!(PINB & pin_bit))
		tck++;

	return tck;
}
#pragma clang diagnostic pop

#define MAX_PWM 21
uint8_t log_table[MAX_PWM] = {0, 1, 3, 5, 7, 9, 10, 15, 19, 24, 30, 37, 47, 58, 72, 89, 111, 138, 171, 213, 255};

volatile uint16_t uptime = 0;
volatile uint8_t pwm = 0;
volatile uint8_t pwm_prev = 0;

ISR (TIMER0_OVF_vect) {

	static uint8_t pre_counter = 0;
	static uint8_t buttons_counter = 0;

	pre_counter++;
	if (pre_counter == 60) {

		uptime++;
		pre_counter = 0;
	}

	for (uint8_t i = 0; i < BUTTONS; i++) {


//		if (PINB & buttons[i].port == 0) {
		if (get_pad_charge_time(buttons[i].port) > 10) {

			if (buttons[i].count > 4) {

				if (!buttons[i].status)
					buttons[i].status = 1;
			}
			else {

				buttons[i].count++;
			}
		}
		else {

			if (buttons[i].count == 0) {

				if (buttons[i].status)
					buttons[i].status = 0;
			}
			else {

				buttons[i].count--;
			}
		}
	}

	buttons_counter++;
	if (buttons_counter == 4) {

		buttons_counter = 0;
		uint8_t prev = pwm;

		if ((buttons[0].status || pwm_prev) && pwm < MAX_PWM - 1) {

			OCR1B = log_table[++pwm];

			if (pwm == pwm_prev)
				pwm_prev = 0;
		}

		if (buttons[1].status && pwm > 0)
			OCR1B = log_table[--pwm];

		if (prev != pwm) {

			eeprom_busy_wait();
			eeprom_write_byte((void *) 0, pwm);
		}
	}
}

int main(void)
{
	cli();
	eeprom_busy_wait();
	pwm_prev = eeprom_read_byte((void *)0);
	if (pwm_prev > MAX_PWM)
		pwm_prev = 0;

//	pwm_prev = MAX_PWM;

	memset(&buttons, 0, sizeof(buttons));
	buttons[0].port = _BV(PINB1);
	buttons[1].port = _BV(PINB2);

	PORTB |= _BV(PINB1) | _BV(PINB2);

    DDRB = _BV(LED_PIN) | _BV(LAMP_PIN);

//	PORTB = _BV(LED_PIN) | _BV(LAMP_PIN);
//	while (1) {}

	// Set Timer0 1MHz / 64 / 256 ~ 60 times per second
	TCCR0B = _BV(CS01) | _BV(CS00);
	// Enable Interrupt on Timer0 overflow
	TIMSK = _BV(TOIE0);

	// PWM freq = 1 MHz / 8 / 256 ~ 488 Hz
	TCCR1 = _BV(CS12);
	GTCCR = _BV(PWM1B) | _BV(COM1B1) | _BV(COM1B1);
	OCR1B = 0;
	sei();

    while (1) {

//		if (get_pad_charge_time(_BV(PINB2)) > 16)
		if (buttons[0].status || buttons[1].status)
			PIN_ON(PORTB, LED_PIN);
		else
			PIN_OFF(PORTB, LED_PIN);

		static uint16_t prev = 0;
		if (uptime != prev) {

			prev = uptime;
			PORTB ^= _BV(LAMP_PIN);
		}
    }
}
#pragma clang diagnostic pop