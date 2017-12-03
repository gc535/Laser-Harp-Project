/*
 * USARTtest1.c
 *
 * Created: 2017/2/13 16:12:26
 * Author : Yuanyu
 */ 

//USART baud rate
#define F_CPU 16000000
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD - 1

//calibrate this value for the sensor
#define UPP 630
#define MIDI_LEN 3

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>


double dist1 = 0;
double dist2 = 0;
//int adcf = 0;
unsigned char String1[]="Hello";
unsigned char String2[]="Hey!!";
	
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

void newline(){
	USART_Transmit(0x0D);//CR
	USART_Transmit(0x0A);//LF
}

void print_range(double x){
	USART_Transmit(0x52);//R
	USART_Transmit(0x61);//a
	USART_Transmit(0x6E);//n
	USART_Transmit(0x67);//g
	USART_Transmit(0x65);//e
	if(x > UPP/2){
		USART_Transmit(0x31);
	}else if(x>UPP/4){
		USART_Transmit(0x32);
	}else{
		USART_Transmit(0x33);
	}
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
// 	ADMUX |= (1<<REFS0);
// 	ADCSRA |= (1<<ADEN) | (1<<ADATE) | (1<<ADIE) | (1<<ADPS2) | (1<<ADPS1) | (1<<ADPS0);
// 	DIDR0 |= (1<<ADC1D);
// 	adc_start();
// 	sei();
// }

// void adc_start(){
// 	ADCSRA |= (1<<ADSC);
// }

// ISR(ADC_vect){
// 	if(adcf){
// 		ADMUX ~= !(1<<MUX0);//channel0
// 		dist1 = ADC;
// 	}else{
// 		ADMUX |= (1<<MUX0);//channel1
// 		dist2 = ADC;
// 	}
// 	adcf = !adcf;
// }

int main(void)
{
    unsigned char strng;
	//Initialization
    USART_Init(MYUBRR);
	//adc_init();
	sei();
	while (1) 
    {
	    USART_putstring(String1);
		newline();
        _delay_ms(50);
 		USART_putstring(String2);
 		newline();
		_delay_ms(50);
	}
}

