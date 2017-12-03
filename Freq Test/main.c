/*
 * sensor test
 *
 * Created: 2/3/2017 4:23:09 PM
 * Author : Y.H
 */ 
#define F_CPU 16000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

int main(void)
{
	DDRC = 0xff;	
		
    while (1) 
    {
		PORTC |= 0x01;
		_delay_ms(500);
		PORTC &= 0xFE;
		_delay_ms(500);
    }
}
