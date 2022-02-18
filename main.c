#define F_CPU 7372800UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include "SSD1306.h"


void displayOLED(uint16_t adc) {
	OLED_Clear();
	OLED_SetCursor(0, 0);
	
	//crtanje grafova 
	if (adc < 45){
		OLED_VerticalGraph(0, 0);
	} else if(adc >= 45 && adc < 48){
		OLED_VerticalGraph(0, 25);
	} else if(adc >= 48 && adc < 51){
		OLED_VerticalGraph(0, 50);
	} else if(adc >= 51 && adc < 53){
		OLED_VerticalGraph(0, 75);
	} else{
		OLED_VerticalGraph(0, 100);
		//aktiviranje buzzera
		PORTA = ~_BV(1);
		_delay_ms(1000);
		PORTA = _BV(1);
	}
}

ISR(ADC_vect) {
	displayOLED(ADC);
}

int main(void)
{
	//DDRA = _BV(1); //postavljanje izlaznog pina za buzzer
	OLED_Init();

	//tretiranje sound detectora kao obi�an ADC
	ADMUX = _BV(REFS0);
	ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1);

	sei();

	 while (1) {
		 ADCSRA |= _BV(ADSC);

		 _delay_ms(200);
	 }
}