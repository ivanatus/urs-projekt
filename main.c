#define F_CPU 7372800UL

#include <avr/io.h>
#include <util/delay.h>
#include <stdlib.h>
#include "SSD1306.h"
// flag za prikaz na display-u
uint8_t flag = 0x00;

// pin definitions
#define DDR_SPI         DDRB
#define PORT_SPI        PORTB
#define CS              PINB4
#define MOSI            PINB5
#define MISO            PINB6
#define SCK             PINB7

// macros
#define CS_ENABLE()     PORT_SPI  &= ~(1 << CS)
#define CS_DISABLE()    PORT_SPI |= (1 << CS)

// command definitions
#define CMD0            0
#define CMD0_ARG        0x00000000
#define CMD0_CRC        0x94

// SPI functions
void SPI_init(void);
uint8_t SPI_transfer(uint8_t data);

// SD functions
void SD_powerUpSeq(void);
void SD_command(uint8_t cmd, uint32_t arg, uint8_t crc);
uint8_t SD_readRes1(void);
uint8_t SD_goIdleState(void);

void debounce() {
	_delay_ms(100);
	GIFR = _BV(INTF0) | _BV(INTF1);
}

void displayOLED(uint16_t adc) {
	OLED_Clear();
	OLED_SetCursor(0, 0);
	
	if(!flag){
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
	} else{
		char sd_string[16];
		uint8_t sd_read = SD_readRes1();
		itoa(sd_read, sd_string, 10);
		OLED_Printf(sd_string);
	}
}

ISR(ADC_vect) {
	displayOLED(ADC);
	SD_command(CMD0, ADC, CMD0_CRC);
}

ISR(INT0_vect) {
	flag ^= 0x01;
	
	displayOLED(0);
}

void SD_card(){
	// initialize SPI
	SPI_init();

	// start power up sequence
	SD_powerUpSeq();

	// command card to idle
	SD_goIdleState();
}

int main(void)
{
	DDRA = _BV(1); //postavljanje izlaznog pina za buzzer
	OLED_Init();
	SD_card();

	//tretiranje sound detectora kao obièan ADC
	ADMUX = _BV(REFS0);
	ADCSRA = _BV(ADEN) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1);
	
	//INT0 za oèitavanje sa SD kartice
	MCUCR = _BV(ISC01) | _BV(ISC11);
	GICR = _BV(INT0);

	sei();

	 while (1) {
		 ADCSRA |= _BV(ADSC);

		 _delay_ms(200);
	 }
}

void SPI_init()
{
	// set CS, MOSI and SCK to output
	DDR_SPI |= (1 << CS) | (1 << MOSI) | (1 << SCK);

	// enable pull up resistor in MISO
	DDR_SPI |= (1 << MISO);

	// enable SPI, set as master, and clock to fosc/128
	SPCR = (1 << SPE) | (1 << MSTR) | (1 << SPR1) | (1 << SPR0);
}

uint8_t SPI_transfer(uint8_t data)
{
	// load data into register
	SPDR = data;

	// Wait for transmission complete
	while(!(SPSR & (1 << SPIF)));

	// return SPDR
	return SPDR;
}

void SD_powerUpSeq()
{
	// make sure card is deselected
	CS_DISABLE();

	// give SD card time to power up
	_delay_ms(1);

	// send 80 clock cycles to synchronize
	for(uint8_t i = 0; i < 10; i++)
	SPI_transfer(0xFF);
}

void SD_command(uint8_t cmd, uint32_t arg, uint8_t crc)
{
	// transmit command to sd card
	SPI_transfer(cmd|0x40);

	// transmit argument
	SPI_transfer((uint8_t)(arg >> 24));
	SPI_transfer((uint8_t)(arg >> 16));
	SPI_transfer((uint8_t)(arg >> 8));
	SPI_transfer((uint8_t)(arg));

	// transmit crc
	SPI_transfer(crc|0x01);
}

uint8_t SD_readRes1()
{
	uint8_t i = 0, res1;

	// keep polling until actual data received
	while((res1 = SPI_transfer(0xFF)) == 0xFF)
	{
		i++;

		// if no data received for 8 bytes, break
		if(i > 8) break;
	}

	return res1;
}

uint8_t SD_goIdleState()
{
	// assert chip select
	SPI_transfer(0xFF);
	CS_ENABLE();
	SPI_transfer(0xFF);

	// send CMD0
	SD_command(CMD0, CMD0_ARG, CMD0_CRC);

	// read response
	uint8_t res1 = SD_readRes1();

	// deassert chip select
	SPI_transfer(0xFF);
	CS_DISABLE();
	SPI_transfer(0xFF);

	return res1;
}

