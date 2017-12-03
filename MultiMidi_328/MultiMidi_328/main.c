/*
 * MultiMidi_328.c
 *
 * Created: 4/4/2017 1:01:18 PM
 * Author : pcadmin
 */ 

#define F_CPU 16000000
#define BAUD 31250
#define MYUBRR F_CPU/16/BAUD - 1

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

int led_flag = 0;
int button_flag = 0;
int pre_button_flag = 1;
int adc_channel = 0;
int adc_buffer[8] = {0};
int handflag_buffer[8] = {0};
int prehandflag_buffer[8] = {0};
unsigned char note_buffer[8] = {0x57,0x45,0x33,0x3C,0x3C,0x3C,0x3C,0x3C};

int tim_cnt = 0;
int adc_flag = 0;
//This is used for 325
void CPU_Clock_Prescalar()
{
	CLKPR = 0x80;
	CLKPR = 0x00;
}

void ADC_Channel_Select(int ch)
{
	ADMUX &= 0xF8;//clear original channel selection
	switch(ch){
		case 1 :
		  ADMUX |= 0x01;
		  break;
		case 2 :
		  ADMUX |= 0x02;
		  break;
		case 3 :
		  ADMUX |= 0x03;
		  break;
		case 4 :
		  ADMUX |= 0x04;
		  break;
		case 5 :
		  ADMUX |= 0x05;
		  break;
		case 6 :
		  ADMUX |= 0x06;
		  break;
		case 7 :
		  ADMUX |= 0x07;
		  break;
	}
}

void timer_init(){
	TCCR0A = (1<<WGM01);
	OCR0A = 156;//itr calling every 10ms
	TIMSK0 = (1<<OCIE0A);
	TCCR0B = (1<<CS02 ) | (1<<CS00);
}

void adc_init(){
	ADMUX |= (1<<REFS0);
	ADCSRA |= (1<<ADEN) | (1<<ADATE) | (1<<ADIE) | (1<<ADPS2) | (1<<ADPS0);
	ADC_Channel_Select(adc_channel);
	adc_start();
}

void adc_start(){
	ADCSRA |= (1<<ADSC);
}

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

void USART_putstring(const char *theString){
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
	UCSR0C = (3<<UCSZ00);//110
}

void Hand_Detection(int ch)
{
	int i = 0;
	for(i;i<(ch+1);i++){
		if(adc_buffer[i]){
			handflag_buffer[i] = 1;
		}else{
			handflag_buffer[i] = 0;
		}
		
		if(handflag_buffer[i] == 1 && prehandflag_buffer[i] == 0){
			USART_Transmit(0x90+i);
			USART_Transmit(note_buffer[i]);
			USART_Transmit(0x40);
		}
		if(handflag_buffer[i] == 0 && prehandflag_buffer[i] == 1){
			USART_Transmit(0x90+i);
			USART_Transmit(note_buffer[i]);
			USART_Transmit(0x00);
		}
		prehandflag_buffer[i] = handflag_buffer[i];	
	}
}

void Midi_Switch_Sax()
{
	USART_Transmit(0xC0);
	USART_Transmit(0x41);
	USART_Transmit(0xC1);
	USART_Transmit(0x41);
	USART_Transmit(0xC2);
	USART_Transmit(0x41);
}

int main(void)
{
	//this must be the first command!!!
	//CPU_Clock_Prescalar();
	DDRB = 0X00;
	DDRD = 0XFF;
	timer_init();
	adc_init();
	USART_Init(MYUBRR);
	sei();
	Midi_Switch_Sax();
	
	while (1)
	{
	}
}

ISR(ADC_vect){
	adc_flag = (adc_flag + 1) % 3;//whit for N cycles where N is # of channel used
	if(adc_flag == 2){
		ADC_Channel_Select(adc_channel);
 		adc_buffer[adc_channel] = (ADC>200);//quantization
		adc_channel = (adc_channel + 1) % 3;//0..2
	}
}

ISR(TIMER0_COMPA_vect){
	Hand_Detection(2);//ch0-2
	if(!(PORTB&(1<<PINB4)) && pre_button_flag == 1){
		led_flag = !led_flag;
	}
	if (led_flag){
		PORTD = 0X04;
	}else{
		PORTD = 0X00;
	   }
	pre_button_flag = (PORTB & (1<<PINB4))>>4;
	
}
