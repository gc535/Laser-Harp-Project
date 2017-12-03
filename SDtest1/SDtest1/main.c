/*
 * SDtest1.c
 *
 * Created: 3/28/2017 12:36:17 PM
 * Author : Yuanyu
 */ 

 //http://codeandlife.com/2012/04/25/simple-fat-and-sd-tutorial-part-3/
#include <avr/io.h>

#define SS (1<<PB0)
#define SCK (1<<PB1)
#define MOSI (1<<PB2)
#define MISO (1<<PB3)
#define SPI_DDR DDRB

#define SS_ENABLE (PORTB |= SS)
#define SS_DISABLE (PORTB &= ~SS)

void SPI_Master_init(){
	//in Slave mode, only MISO should be output
	SPI_DDR |= SS | SCK | MOSI | MISO;//all output
	//in Slave mode, MSTR should be cleared
	SPCR |= (1<<SPE) | (1<<MSTR) | (1<<SPR1) | (1<<SPR0);//enable, master mode, Fosc/128
}

unsigned char SPI_Master_Write(unsigned char data){
	SPDR = data;
	while(!(SPSR & (1<<SPIF)))
	;
	return SPDR;
}

void SPI_Slave_init(void)
{
	/* Set MISO output, all others input */
	SPI_DDR |= MISO;
	/* Enable SPI */
	SPCR = (1<<SPE) | (1<<SPR1) | (1<<SPR0);//slave mode, Fosc/128
}

char SPI_Slave_Read(void)
{
	/* Wait for reception complete */
	while(!(SPSR & (1<<SPIF)))
	;
	/* Return Data Register */
	return SPDR;
}

void SD_command(unsigned char cmd, unsigned long arg, unsigned char crc, unsigned char read) {
	unsigned char i, buffer[8];
	//arg is a 32 bits (4 bytes) variable
	
	SS_ENABLE;
	SPI_Master_Write(cmd);
	SPI_Master_Write(arg>>24);
	SPI_Master_Write(arg>>16);
	SPI_Master_Write(arg>>8);
	SPI_Master_Write(arg);
	SPI_Master_Write(crc);
	
	for(i=0; i<read; i++)
	buffer[i] = SPI_Master_Write(0xFF);
	SS_DISABLE;
}
/*
each command should have 48 bits
Init and go to SPI mode: [0x40 0x00 0x00 0x00 0x00 0x95]
Initialize card: [0x41 0x00 0x00 0x00 0x00 0xFF]
Set transfer size: [0x50 0x00 0x00 0x02 0x00 0xFF]
Read sector: [0x51 0x00 0x00 0x00 0x00 0xFF]
*/
int main(void)
{
    
    while (1) 
    {
    }
}

