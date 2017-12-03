/*
 * I2CTest2.c
 *
 * Created: 2017/2/28 14:21:28
 * Author : Yuanyu
 */ 

#define F_CPU 8000000

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define SLAVE_ADDR  0x10

#define MESSAGEBUF_SIZE       4

#define TWI_GEN_CALL         0x00  // The General Call address is 0

// Sample TWI transmission commands
#define TWI_CMD_MASTER_WRITE 0x10
#define TWI_CMD_MASTER_READ  0x20

#define DDR_USI             DDRE
#define PORT_USI            PORTE
#define PIN_USI             PINE
#define PORT_USI_SDA        PINE5
#define PORT_USI_SCL        PINE4
#define PIN_USI_SDA         PINE5
#define PIN_USI_SCL         PINE4

#define TWI_READ_BIT  0       // Bit position for R/W bit in "address byte".
#define TWI_ADR_BITS  1       // Bit position for LSB of the slave address bits in the init byte.
#define TWI_NACK_BIT  0       // Bit position for (N)ACK bit.


unsigned char ACK_Rev = 0xAA;
unsigned char tempUSISR_8bit = (1<<USISIF)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)|      // Prepare register value to: Clear flags, and
(0x0<<USICNT0);                                     // set USI to shift 8 bits i.e. count 16 clock edges.
unsigned char tempUSISR_1bit = (1<<USISIF)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)|      // Prepare register value to: Clear flags, and
(0xE<<USICNT0);                                     // set USI to shift 1 bit i.e. count 2 clock edges.

void Clock_Prescale(){
	CLKPR = 0x80;
	CLKPR = 0x00;
}

void USI_TWI_Master_Initialise()
{
  PORT_USI |= (1<<PIN_USI_SDA);           // Enable pullup on SDA, to set high as released state.
  PORT_USI |= (1<<PIN_USI_SCL);           // Enable pullup on SCL, to set high as released state.
  
  DDR_USI  |= (1<<PIN_USI_SCL);           // Enable SCL as output.
  DDR_USI  |= (1<<PIN_USI_SDA);           // Enable SDA as output.
  
  USIDR    =  0xFF;                       // Preload dataregister with "released level" data.
  USICR    =  (0<<USISIE)|(0<<USIOIE)|                            // Disable Interrupts.
              (1<<USIWM1)|(0<<USIWM0)|                            // Set USI in Two-wire mode.
              (1<<USICS1)|(0<<USICS0)|(1<<USICLK)|                // Software stobe as counter clock source
              (0<<USITC);
  USISR   =   (1<<USISIF)|(1<<USIOIF)|(1<<USIPF)|(1<<USIDC)|      // Clear flags,
              (0x0<<USICNT0);                                     // and reset counter.
}

unsigned char USI_TWI_Master_Transfer( unsigned char temp )
{
  USISR = temp;                                     // Set USISR according to temp.
                                                    // Prepare clocking.
  temp  =  (0<<USISIE)|(0<<USIOIE)|                 // Interrupts disabled
           (1<<USIWM1)|(0<<USIWM0)|                 // Set USI in Two-wire mode.
           (1<<USICS1)|(0<<USICS0)|(1<<USICLK)|     // Software clock strobe as source.
           (1<<USITC);                              // Toggle Clock Port.
  do
  {
	_delay_us(25);              
    USICR = temp;                          // Generate positve SCL edge.
    while( !(PIN_USI & (1<<PIN_USI_SCL)) );// Wait for SCL to go high.
    _delay_us(25);         
    USICR = temp;                          // Generate negative SCL edge.
  }while( !(USISR & (1<<USIOIF)) );        // Check for transfer complete.

    _delay_us(25);               
    temp  = USIDR;                           // Read out data.
    USIDR = 0xFF;                            // Release SDA.
    DDR_USI |= (1<<PIN_USI_SDA);             // Enable SDA as output.

    return temp;                             // Return the data from the USIDR
}

 void USI_TWI_Start_Condition(){
	PORT_USI |= (1<<PIN_USI_SCL);                     // Release SCL.
    while( !(PIN_USI & (1<<PIN_USI_SCL)) );          // Verify that SCL becomes high.
	PORT_USI &= ~(1<<PIN_USI_SDA);                    // Force SDA LOW.
    _delay_us(25);                       
    PORT_USI &= ~(1<<PIN_USI_SCL);                    // Pull SCL LOW.
    PORT_USI |= (1<<PIN_USI_SDA);                     // Release SDA.
 }
 
 void USI_TWI_Stop_Condition(){
	 PORT_USI &= ~(1<<PIN_USI_SDA);           // Pull SDA low.
	 PORT_USI |= (1<<PIN_USI_SCL);            // Release SCL.
	 while( !(PIN_USI & (1<<PIN_USI_SCL)) );  // Wait for SCL to go high.
	 _delay_us( 25 );
	 PORT_USI |= (1<<PIN_USI_SDA);            // Release SDA.
	 _delay_us( 25 );
 }

void USI_TWI_Check_ACK()
{
	if( USI_TWI_Master_Transfer( tempUSISR_1bit ) & (1<<TWI_NACK_BIT) ){
		ACK_Rev = 0x00;
	}
	while(!ACK_Rev){
		;//error code
	}
}

void USI_TWI_Write_Addr(unsigned char TWI_targetSlaveAddress)
{
	/* Write a byte */
	PORT_USI &= ~(1<<PIN_USI_SCL);                // Pull SCL LOW.
	USIDR     = TWI_targetSlaveAddress;           // Setup data.
	USI_TWI_Master_Transfer( tempUSISR_8bit );    // Send 8 bits on bus.
	
	/* Clock and verify (N)ACK from slave */
	DDR_USI  &= ~(1<<PIN_USI_SDA);                // Enable SDA as input.
}

unsigned char USI_TWI_Read_Byte()
{
	unsigned char TWI_Read;
	DDR_USI   &= ~(1<<PIN_USI_SDA);               // Enable SDA as input.
	TWI_Read  = USI_TWI_Master_Transfer( tempUSISR_8bit );	
	USIDR = 0xFF;
	USI_TWI_Master_Transfer( tempUSISR_1bit ); // Generate ACK/NACK
	USI_TWI_Stop_Condition();	
	return TWI_Read;
}

void check_pc0(){
	DDRC = 0x01;
	PORTC = 0x01;
}

 /***************************************************
 Read Operation of STC3100
 -----------------------------------------------------------------------------------
   <1>       <2>          <3>        <4>       <5>           <6>                 <7>
 |Start|Device Addr|W|A|Reg Addr|A|Restart|Device Addr|R|A|Reg Data|A|Reg Data|_|Stop|
 |     |  7 Bits   | | | 8 Bits | |       |   7 Bits  | | |  8 Bits| |  8 Bits|A|    |
 -----------------------------------------------------------------------------------
 ^                    ^                                                      ^
 |                    |                                                      |
 Master               Slave ACK                                          Master NACK
 ****************************************************/
 
int main( void )
{
// 	unsigned char messageBuf[MESSAGEBUF_SIZE] = {0};
	unsigned char DeviceAddr_W = 0xE0;
	unsigned char DeviceAddr_R = 0xE1;
	unsigned char RegAddr = 0x18;
	unsigned char TWI_Read = 0x00;
	
	Clock_Prescale();
  
	USI_TWI_Master_Initialise();
	USI_TWI_Start_Condition();//step <1>    
	USI_TWI_Write_Addr(DeviceAddr_W);//step <2>	
	USI_TWI_Check_ACK();//check if ack/nack is received
	USI_TWI_Write_Addr(RegAddr);//step <3>
	USI_TWI_Check_ACK();//check if ack/nack is received
	check_pc0();
	USI_TWI_Start_Condition();//step <4>
	USI_TWI_Write_Addr(DeviceAddr_R);//step <5>
	USI_TWI_Check_ACK();//check if ack/nack is received	
	TWI_Read = USI_TWI_Read_Byte();//step <6> <7>
  
	for(;;)
	{  
		check_pc0();
	}    
}


