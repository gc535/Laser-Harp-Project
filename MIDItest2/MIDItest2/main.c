/*
 * MIDItest2.c
 *
 * Created: 3/29/2017 7:34:34 AM
 * Author : Yuanyu
 */ 

#define F_CPU 16000000
#define BAUD 31250
#define MYUBRR F_CPU/16/BAUD - 1

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

unsigned char music_on[] = {0x90,0x60,0x40};
unsigned char music_off[] = {0x90,0x60,0x00};
double dist = 0;

unsigned char USART_Receive(void){
	while(!(UCSR0A & (1<<RXC0)))
	;
	return UDR0;
}

void USART_Transmit(unsigned char data){
	while(!(UCSR0A & (1<<UDRE0)))
	;
	UDR0 = data;
}

void USART_putstring(unsigned char *theString){
	int i = 0;
	for(i;i<strlen(theString);i++){
		USART_Transmit(theString[i]);
	}
}

void USART_Init(unsigned int ubrr){
	//baud rate
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	//enable receiver and transmitter
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	//format: 8 bit data, 1 stop bit
	UCSR0C = (3<<UCSZ00);
}

// void adc_init(){
// 	ADMUX |= (1<<REFS0) | (1<<MUX0);
// 	ADCSRA |= (1<<ADEN) | (1<<ADATE) | (1<<ADIE) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
// 	DIDR0 |= (1<<ADC1D);
// 	adc_start();
// }
// 
// void adc_start(){
// 	ADCSRA |= (1<<ADSC);
// }

// ISR(ADC_vect){
// 	dist = ADC;
// }

int main(void)
{
	USART_Init(MYUBRR);
	sei();
	//adc_init();

	while (1)
	{
        _delay_ms(200);
		if(1){
			USART_Transmit(0x90);
			USART_Transmit(0x60);
			USART_Transmit(0x40);
			_delay_ms(100);
			USART_Transmit(0x90);
			USART_Transmit(0x60);
			USART_Transmit(0x00);
			_delay_ms(100);
		}
	}
}


