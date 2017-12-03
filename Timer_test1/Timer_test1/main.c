/*
 * Timer_test1.c
 *
 * Created: 2017/2/8 18:36:25
 * Author : Yuanyu
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>

#define LED3_ON    PORTB |= (1<<PORTB3)
#define LED3_OFF    PORTB &= ~(1<<PORTB3)
#define LED4_ON    PORTB |= (1<<PORTB4)
#define LED4_OFF    PORTB &= ~(1<<PORTB4)
#define LED5_ON    PORTB |= (1<<PORTB5)
#define LED5_OFF    PORTB &= ~(1<<PORTB5)

int timcnt = 0;//counter
double dist = 0;

void adc_init(){
	ADMUX |= (1<<REFS0) | (1<<MUX0);
	ADCSRA |= (1<<ADEN) | (1<<ADATE) | (1<<ADIE) | (1<<ADPS2) | (1<<ADPS0);
    DIDR0 |= (1<<ADC1D);
	adc_start();
}

void adc_start(){
	ADCSRA |= (1<<ADSC);
}

int main(void)
{
	//IO pins
	DDRB = 0x38;//PB3,4,5 as output

    //timer
	TCCR0A = (1<<WGM01);
	OCR0A = 87;//itr calling every 10ms
	TIMSK0 = (1<<OCIE0A);
	sei();
	TCCR0B = (1<<CS02 ) | (1<<CS00);

    //adc	
	adc_init();

    while (1) 
    { 
	}
	
}

ISR(TIMER0_COMPA_vect){
	timcnt++;
	if (timcnt > 50){//100 * 10ms
		timcnt = 0;
		if((dist <= 200)){
			LED3_ON;
			LED4_OFF;
			LED5_OFF;
			}else if(dist <= 400){
			LED4_ON;
			LED3_OFF;
			LED5_OFF;
			}else{
			LED5_ON;
			LED4_OFF;
			LED3_OFF;
		}
	}
}

ISR(ADC_vect){
	dist = ADC;
	//adc_start();
}

