/*
 * BatteryLevelSimulation.c
 *
 * Created: 2017/2/26 14:01:52
 * Author : Yuanyu
 */ 

#define F_CPU 16000000
#define BAUD 9600
#define MYUBRR F_CPU/16/BAUD - 1

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define DataBus PORTD
#define Data_IO DDRD
#define ControlBus PORTB
#define Control_IO DDRB

#define Enable 5
#define RW 3
#define RS 2

#define CLEAR 0x01
#define HOME 0x02
#define FUNCTION 0x38
#define ALL_ON 0x0E
#define FIRSTLINE 0x80
#define SECONDLINE 0xC0

int tim_cnt = 0;
unsigned char fake_battery = 0x00;
int bar = 0;

void timer_init(){
	TCCR0A = (1<<WGM01);
	OCR0A = 156;
	TIMSK0 = (1<<OCIE0A);
	TCCR0B = (1<<CS02) | (1<<CS00);
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

void lcd_init(){
	Control_IO |= 1<<Enable | 1<<RW | 1<<RS;
	_delay_ms(15);

	Send_Command(CLEAR);
	_delay_ms(2);
	Send_Command(FUNCTION);
	_delay_us(50);
	Send_Command(ALL_ON);
	_delay_us(50);
}

void Check_Busy()
{
	Data_IO = 0x00;
	ControlBus |= 1<<RW;
	ControlBus &= ~1<<RS;

	while (DataBus >= 0x80)
	{
		Toggle();
	}
	Data_IO = 0xFF;
}

void Toggle()
{
	ControlBus |= 1<<Enable;
	//DO NOT modify this value unless ur 100% sure
	_delay_us(30);
	ControlBus &= ~1<<Enable;
}

void Send_Command(unsigned char command)
{
	Check_Busy();
	DataBus = command;
	ControlBus &= ~ ((1<<RW)|(1<<RS));
	Toggle();
	DataBus = 0;
}

void Send_Char(unsigned char character)
{
	Check_Busy();
	DataBus = character;
	ControlBus &= ~ (1<<RW);
	ControlBus |= 1<<RS;
	Toggle();
	DataBus = 0;
}

void Send_String(char *string){
	int i = 0;
	for(i;i<strlen(string);i++){
		Send_Char(string[i]);
	}
}

int main(void)
{
	lcd_init();
	USART_Init(MYUBRR);
	timer_init();
	sei();
    Send_String("`abcdefghi");
	Send_Command(0xBD);
	_delay_us(50);
	while (1)
	{
		fake_battery = USART_Receive();
	}
}

ISR(TIMER0_COMPA_vect){
	int bar = fake_battery * 16 / 255;
	int i = 16;
	tim_cnt++;
	if(tim_cnt >= 100){
		tim_cnt = 0;
		for(i;i>0;i--){
			if(bar){
				Send_Char(0xFF);
			}else{
				Send_Char(0x17);
			}
			if(bar){
			    bar--;
			}
		}
		Send_Command(0xBD);
		_delay_us(50);
	}
}

