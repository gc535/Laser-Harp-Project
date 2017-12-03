/*
 * LCDtest1.c
 *
 * Created: 2017/2/13 10:44:40
 * Author : Yuanyu
 */ 
#define F_CPU 16000000

#include <avr/io.h>
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

void Check_Busy();
void Toggle();
void Send_Command(unsigned char command);
void Send_Char(unsigned char character);
void Send_String(char *string);
void lcd_init();



int main(void)
{
    lcd_init();
	
	Send_String("0123456789");
	Send_Command(SECONDLINE);
	_delay_us(50);
	Send_String("AbCdEfGhIj");
	while(1)
	{
	}
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

