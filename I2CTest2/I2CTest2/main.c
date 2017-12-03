/*
 * I2CTest2.c
 *
 * Created: 2017/2/28 14:21:28
 * Author : Yuanyu
 */ 

/***************************************************
Read Operation of STC3100
-----------------------------------------------------------------------------------
|Start|Device Addr|W|A|Restart|Reg Addr|Device Addr|R|A|Reg Data|A|Reg Data|_|Stop|
|     |  7 Bits   | | |       | 8 Bits |   7 Bits  | | |  8 Bits| |  8 Bits|A|    |
-----------------------------------------------------------------------------------
^                    ^                                                      ^                    
|                    |                                                      |
Master               Slave ACK                                          Master NACK
****************************************************/
#define F_CPU 1100000

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

int check_cnt = 0;
uint8_t dev_add = 0xE0;

void I2CClearFlag_Enable()
{
	TWCR = (1<<TWINT) | (1<<TWEN);//clear the flag and enable
}

void I2CClearFlag_Enable_ACK()
{
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA);//clear the flag and enable
}

void Error_I2C_TWI(){
	for(int i = 0;i<100;i++){
		PORTC ^= (1<<PINC0);
		_delay_ms(20);
	}
	_delay_ms(500);
}

void Pass_I2C_TWI(){
	PORTC |= (1<<PINC0);
	_delay_ms(500);
	PORTC &= ~(1<<PINC0);
	_delay_ms(500);
}

void I2CFlagWait()
{
	while(!(TWCR & (1<<TWINT)))
	//while((TWCR & (1<<TWINT)))
	;
}

void I2CCheckStatus(uint8_t status)
{
	check_cnt++;
	if((TWSR & 0xF8) != status){
		Error_I2C_TWI();
	}else{
		Pass_I2C_TWI();
	}
}

void I2C_Start()
{
	TWCR |= (1<<TWEN) | (1<<TWINT) | (1<<TWSTA);//TWIE not sure
}

int main(void)
{
    uint8_t voltage_low = 0x00;
	
	DDRC |= 0b00000001;
// 	PORTD |= 0b00000100;
	//initialization
	PRR &= ~(1<<PRTWI);
	TWCR &= ~(1<<TWIE);
	//TWCR |= (1<<TWIE);
	TWBR = 0x3C;
	TWSR |= (1<<TWPS0);//actual SCL is 32258Hz
	
	//Start Condition
	I2C_Start();
	//sei();
	I2CFlagWait();//Wait for the flag to be set
	I2CCheckStatus(0x08);
	 
	 _delay_ms(500);
	 
	//Send slave address and W
	TWDR = 0xE0;
	I2CClearFlag_Enable();
	I2CFlagWait();
	I2CCheckStatus(0x18); //check status for SLA+W trans & ack recev

	//Write Register Addr
	TWDR = 0x18;//REG_VOLTAGE_LOW check 8.2 in datasheet
	I2CClearFlag_Enable();
	I2CFlagWait();
	I2CCheckStatus(0x28);
	
	//repeat start
	I2C_Start();
	I2CFlagWait();
	I2CCheckStatus(0x10); //check status for SLA+W trans & ack recev
	
	//send device (slave) address and R
	TWDR = 0xE1;
	I2CClearFlag_Enable();
	I2CFlagWait();
	I2CCheckStatus(0x40); //check status for SLA+W trans & ack recev
	
	//read data (byte1)
	I2CClearFlag_Enable();
	I2CFlagWait();
	I2CCheckStatus(0x58);//check data recev and NACK sent
	
	//retrive
	voltage_low = TWDR;
	
	//read data (byte2)
// 	volatile voltage_high = 0;
// 	voltage_high = TWDR;
// 	I2CClearFlag_Enable();
// 	I2CFlagWait();
// 	I2CCheckStatus(0x58);//check data recev and NACK sent
	
	//stop condition
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO);
	
	//write register address
// 	volatile uint16_t voltage = 0;
// 	voltage = (voltage_high<<8) + voltage_low;
	 
    while (1) 
    {
    }
}


