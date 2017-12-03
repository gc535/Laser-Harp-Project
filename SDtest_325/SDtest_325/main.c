/*
 * SDtest1.c
 *
 * Created: 3/28/2017 12:36:17 PM
 * Author : Yuanyu
 */ 

 /* http://codeandlife.com/2012/04/25/simple-fat-and-sd-tutorial-part-3/ */
#define F_CPU 8000000

#include <avr/io.h>
#include <util/delay.h>

#define SS (1<<PB0)
#define SCK (1<<PB1)
#define MOSI (1<<PB2)
#define MISO (1<<PB3)
#define SPI_DDR DDRB

#define SS_DISABLE() (PORTB |= SS)
#define SS_ENABLE() (PORTB &= ~SS)

#define CMD0 0
#define CMD8 8
#define CMD12 12
#define CMD17 17
#define CMD55 55
#define ACMD41 41

unsigned char sd_response[6] = {0};
unsigned char sd_addr[4] = {0};
unsigned char sd_data[100] = {0};
	
int timeout[3] = {0};


void Clock_Prescale(){
	CLKPR = 0x80;
	CLKPR = 0x00;
}

void SPI_Clock_Change(int divide){
	//refer to Table 19-5
	SPCR &= ~(1<<SPR0 | 1<<SPR1);
	SPSR &= ~(1<<SPI2X);
	
	switch(divide){
		case 2 :
		SPSR |= (1<<SPI2X);
		break;
		case 4 :
		;
		break;
		case 8 :
		SPSR |= (1<<SPI2X);
		SPCR |= (1<<SPR0);
		break;
		case 16 :
		SPCR |= (1<<SPR0);
		break;
		case 32 :
		SPSR |= (1<<SPI2X);
		SPCR |= (1<<SPR1);
		break;
		case 64 :
		SPSR |= (1<<SPI2X);
		SPCR |= (1<<SPR0);
		SPCR |= (1<<SPR1);
		break;
		case 128 :
		SPCR |= (1<<SPR0);
		SPCR |= (1<<SPR1);
		break;
	}
}

unsigned long FAT32_Addr(unsigned long part_set, unsigned long off_set){
	unsigned long fat32_addr;
	fat32_addr = (part_set + off_set) * 0x200;
	return fat32_addr;
}

unsigned char* Address_Byte(unsigned long addr)
{
	sd_addr[0] = addr >> 24;
	sd_addr[1] = (addr << 8) >> 24;
	sd_addr[2] = (addr << 16) >> 24;
	sd_addr[3] = (addr << 24) >> 24;
	return sd_addr;
}

void SPI_Master_init(){
	//in Slave mode, only MISO should be output
	SPI_DDR |= SS | SCK | MOSI;//all output
	//in Slave mode, MSTR should be cleared
	SPCR |= (1<<SPE) | (1<<MSTR);//enable, master mode, Fosc/128
}

unsigned char SPI_Master_Write(unsigned char data){
	SPDR = data;
	while(!(SPSR & (1<<SPIF)))
	;
	return SPDR;
}

//This function should be called before the init command
void Clock_Pulse(){
	SS_DISABLE();
	int i = 0;
	for(i;i<11;i++){//88 clock cycles (minimum 74 )
		SPI_Master_Write(0xFF);//keeping MOSI high
	}
}

int SD_command(unsigned char cmd, unsigned long arg, unsigned char crc, unsigned char read) {
	
	unsigned char CMD = 0x40 | cmd;
	int i = 0;
	int check = 0;
	SS_ENABLE();

	SPI_Master_Write(CMD);
	SPI_Master_Write(arg>>24);
	SPI_Master_Write(arg>>16);
	SPI_Master_Write(arg>>8);
	SPI_Master_Write(arg);
	SPI_Master_Write(crc);
	//adjustment
	SPI_Master_Write(0xFF);	
	for(i; i<read; i++){
		sd_response[i] = SPI_Master_Write(0xFF);
	}
    SS_DISABLE();
	
	if(cmd == CMD8){
		check = Check_Response(arg,4,0);//CMD8 check pattern
	}
	return check;
}

int Check_Response(unsigned long arg1, int index, int offset){//compare one byte in the command argument with one byte in response
	int i;
	unsigned char byte2 = (arg1>> (offset*8) ) & 0xFF;
	sd_response[index] == byte2? (i = 0): (i = 1);
	return i; 

}

/*
each command should have 48 bits
Init and go to SPI mode: [0x40 0x00 0x00 0x00 0x00 0x95]
Initialize card: [0x41 0x00 0x00 0x00 0x00 0xFF]
Set transfer size: [0x50 0x00 0x00 0x02 0x00 0xFF]
Read sector: [0x51 0x00 0x00 0x00 0x00 0xFF]
*/

void SD_init(){
	SPI_Master_init();
	Clock_Pulse();
	do{
		SD_command(CMD0,0x00000000,0x95,6);
		//LCD message here
	}while(sd_response[0] != 0x01);
	
	while(SD_command(CMD8,0x000001AA,0x87,6)){
		//LCD message here
	}
	
	do{
		SD_command(CMD55,0x00000000,0xFF,6);
		SD_command(ACMD41,0x00000000,0xFF,6);
		//LCD message here
	}while(sd_response[0] != 0x00);
	PORTC = 0x01;
}

void SD_Read_Single(unsigned long address, unsigned short offset, unsigned short length, unsigned char* buffer){
	unsigned short k = offset+length;
	SS_ENABLE();
	unsigned char* addr = Address_Byte(address);
	/************CMD17****************/
	SPI_Master_Write(0x51);//CMD17??? Is 0x57 ok?
	SPI_Master_Write(addr[0]);
	SPI_Master_Write(addr[1]);
	SPI_Master_Write(addr[2]);
	SPI_Master_Write(addr[3]);
	SPI_Master_Write(0xFF);
	
	do{
		timeout[0]++;
		SPI_Master_Write(0xFF);
		//lcd message here
	}while(timeout[0] < 30 && SPDR != 0x00);
	
	if(timeout[0] == 30){
		;//lcd message
	}
		
	do{
		timeout[1]++;
		SPI_Master_Write(0xFF);
	}while(timeout[1] < 30 && SPDR != 0xFE);
	
	if(timeout[1] == 30){
		;//lcd message
	}
	
	 for(int i=0; i<offset; i++){
		 SPI_Master_Write(0xFF);
	 }//skip bytes
	 
	 for(int j=0; j<length; j++){
		 buffer[j] = SPI_Master_Write(0xFF);
	 }//read part within length only
	 
	 for(k; k<512; k++){
		 SPI_Master_Write(0xFF);
	 }
	 //skip checksum
	 SPI_Master_Write(0xFF);
	 SPI_Master_Write(0xFF);
	 
	 SS_DISABLE();
	 
	 do{
		 timeout[2]++;
		 SD_command(CMD12,0x00000000,0xFF,6);
	 } while (timeout[2] < 30 && sd_response[0] != 0x00);
	 
	 if(timeout[2] == 30){
		 ;//lcd message
	 }
}

void Check_SPI_Ouput()
{
	SPI_Master_Write(0x55);
	SPI_Master_Write(0xCA);
}

int main(void)
{
	DDRC = 0x01;
	Clock_Prescale();
	SPI_Clock_Change(32);//Fcpu/32
	SD_init();
	SD_Read_Single(FAT32_Addr(144,16385),0x0000,0x0064,sd_data);//0x64 = 100
	PORTC = 0x01;
    while (1) 
    {
		Check_SPI_Ouput();		
    }
}
