/*
 * MIDItest1.c
 *
 * Created: 2017/2/25 17:56:20
 * Author : Yuanyu
 */ 

#define F_CPU 16000000
#define BAUD 31250
#define MYUBRR F_CPU/16/BAUD - 1

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

unsigned char music_on[] = {0x90,0x3C,0x40};
unsigned char music_off[] = {0x80,0x3C,0x40};

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

int main(void)
{
    USART_Init(MYUBRR);
	sei();
// 	USART_Transmit(0x90);
// 	USART_Transmit(0x3C);
//  	USART_Transmit(0x40);
    while (1) 
    {
		USART_Transmit(0x90);
		USART_Transmit(0x3C);
		USART_Transmit(0x40);
		_delay_ms(500);
		USART_Transmit(0x80);
		USART_Transmit(0x3C);
		USART_Transmit(0x40);
		_delay_ms(500);
// 		USART_putstring(music_on);
// 		_delay_ms(1000);
// 		USART_putstring(music_off);
// 		_delay_ms(1000);
    }
}

