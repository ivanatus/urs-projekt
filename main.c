#define F_CPU 7372800UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include "SSD1306.h"


void writeOLED(uint16_t adc) {
	OLED_Clear();
	OLED_SetCursor(0, 0);
	char adcStr[16];
	itoa(adc, adcStr, 10);
	OLED_Printf(adcStr);
	
	//crtanje grafova 
	if (adc < 45){
		OLED_VerticalGraph(0, 25);
	} else if(adc >= 45 && adc < 50){
		OLED_VerticalGraph(1, 50);
	} else if(adc >= 50){
		OLED_VerticalGraph(2, 75);
		//aktiviranje buzzera
		PORTA = ~_BV(1);
		_delay_ms(1000);
		PORTA = _BV(1);
	}
}

int main(void)
{
	DDRA = _BV(1); //postavljanje izlaznog pina za buzzer
	OLED_Init();

	//tretiranje sound detectora kao obièan ADC
	ADMUX = _BV(REFS0);
	ADCSRA = _BV(ADEN) | _BV(ADPS2) | _BV(ADPS1);

	while (1) {
		ADCSRA |= _BV(ADSC);

		while (!(ADCSRA & _BV(ADIF)));

		writeOLED(ADC);

		_delay_ms(200);
	}
}
