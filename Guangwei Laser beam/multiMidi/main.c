/*
 * MultiMidi.c
 *
 * Created: 4/3/2017 10:18:20 PM
 * Author : Yuanyu Huang, Guangwei Chen
 */ 

#define F_CPU 1100000//8Mhz
#define BAUD 31250
#define MYUBRR F_CPU/16/BAUD - 1
#define Sensor_Amount 1

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

//Instrument Button
int button_flag = 0;
int pre_button_flag = 1;
int led_flag = 0;
int change_instrument_flag = 0;
int Instrument_Type = 0;

//Laser Decoder
int beam = 0;
int temp_beam = 0;  //only for debugging
int beam_flag0 = 0;
int beam_flag1 = 0;
int beam_flag2 = 0;
int beam_flag3 = 0;
int beam_flag4 = 0;
int beam_flag5 = 0;
int beam_flag6 = 0;
int half_second_counter = 0; //count 500 for 500ms
int time_mark0 = 0;
int time_mark1 = 0;
int time_mark2 = 0;
int time_mark3 = 0;
int time_mark4 = 0;
int time_mark5 = 0;
int time_mark6 = 0;



int free_mode_flag = 1;
int Autoplay_flag = 1;

//ADC and IR sensor detection
int result_array[7] = {0}; 
int sorted_array[7] = {0};
int compare_value = 0;
int adc_channel = 0;
int adc_sensitive_detect[8] = {0};
int adc_buffer[8] = {0};
int handflag_buffer[8] = {0};
int prehandflag_buffer[8] = {0};


unsigned char note_buffer[21] = {0x48, 0x4a, 0x4c, 0x4d, 0x4f, 0x51, 0x53, 0x3c, 0x3e, 0x40, 0x41, 0x43, 0x45, 0x47, 0x30, 0x32, 0x34, 0x35, 0x37, 0x39, 0x3b}; //HIGH,MID,LOW: CDEFGAB
const char Grand_Piano[6] = {0xC0, 0x01, 0xC1, 0x01, 0xC2, 0x01};
const char Alto_Sax[6] = {0xC0, 0x41, 0xC1, 0x41, 0xC2, 0x41};
const char Guitar[6] = {0xC0, 0x19, 0xC1, 0x19, 0xC2, 0x19};


int tim_cnt = 0;
int adc_flag = 0;
int check = 0;

//This is used for 325
void Clock_Prescalar()
{
	asm volatile("push %0"::"r" ("__sreg__"):);
	asm volatile("cli"::);
	CLKPR = 0x80;
	CLKPR = 0x00;
	asm volatile("pop %0"::"r" ("__sreg__"):);
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
	//in 325, TCCR0A is combined with TCCR0B in 328
	TCCR0A = (1<<WGM01) | (1<<CS02 ); //| (1<<CS00);
	OCR0A = 43;//itr calling every 10ms
	TIMSK0 = (1<<OCIE0A);
}

void adc_init(){
	ADMUX |= (1<<REFS0);
	ADCSRA |= (1<<ADEN) | (1<<ADATE) | (1<<ADIE) | (1<<ADPS2) | (1<<ADPS1);
	ADC_Channel_Select(adc_channel);  //0..6
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
	int current_note;
	int i = 0;
	for(i;i<(ch+1);i++){
		if(adc_sensitive_detect[i]){
			handflag_buffer[i] = 1;                //hand detected
		}else if(!adc_sensitive_detect[i]){
			handflag_buffer[i] = 0;				   //no hand detected
		}
		
		if(handflag_buffer[i] == 1 && prehandflag_buffer[i] == 0){
			if(adc_buffer[i]){
				USART_Transmit(0x90+i);
				USART_Transmit(note_buffer[7*(adc_buffer[i]-1) + i]);    //7*parameter --- change pitch level of one string
				USART_Transmit(0x40);
				current_note = note_buffer[7*(adc_buffer[i]-1) + i];
			}
		}
		if(handflag_buffer[i] == 0 && prehandflag_buffer[i] == 1){
			USART_Transmit(0x90+i);
			USART_Transmit(note_buffer[7*(adc_buffer[i]-1) + i]);	//7*parameter --- change pitch level of one string
			USART_Transmit(0x00);
			adc_buffer[i] = 0;
		}
		prehandflag_buffer[i] = handflag_buffer[i];	
	}
}

void Midi_Switch_Instrument(){
	
	if(change_instrument_flag == 1){
		change_instrument_flag = 0;
		if(Instrument_Type==0){
			USART_Transmit(0XC0);
			USART_Transmit(0X01);
			USART_Transmit(0XC1);
			USART_Transmit(0X01);
			USART_Transmit(0XC2);
			USART_Transmit(0X01);
			//USART_putstring(Grand_Piano);
		}
		else if(Instrument_Type==1){
			USART_Transmit(0XC0);
			USART_Transmit(0X41);
			USART_Transmit(0XC1);
			USART_Transmit(0X41);
			USART_Transmit(0XC2);
			USART_Transmit(0X41);
			//USART_putstring(Alto_Sax);
		}
		else if(Instrument_Type==2){
			USART_Transmit(0XC0);
			USART_Transmit(0X27);
			USART_Transmit(0XC1);
			USART_Transmit(0X27);
			USART_Transmit(0XC2);
			USART_Transmit(0X27);	
			//USART_putstring(Guitar);
		}
	}
	
}

void laser_Finder(unsigned char note_played){
	int reminder;
	reminder = note_played % 0x0C;
	if(reminder == 0 || reminder ==1 ){
		beam = 0;   //do & do#
	}else if(reminder == 2 || reminder ==3 ){
		beam = 1;   //re & re#
	}else if(reminder == 4 ){
		beam = 2;   //mi
	}else if(reminder == 5 || reminder == 6 ){
		beam = 3;   //fa & fa#
	}else if(reminder == 7 || reminder ==8 ){
		beam = 4;   //so & so#
	}else if(reminder == 9 || reminder ==10 ){
		beam = 5;   //la & la#
	}else if(reminder == 11 ){
		beam = 6;   //si 
	}else{
		beam = 7;
	}
}


void Demutiplexer_Output(int time_point){
	//turn on logic
	if(beam==0 && beam_flag0==0){
		beam = 7;
		PORTC |= 0X01;
		beam_flag0 = 1;
		time_mark0 = time_point;
	}
	if(beam==1 && beam_flag1==0){
		beam = 7;
		PORTC |= 0X02;
		beam_flag1 = 1;
		time_mark1 = time_point;
	}
	if(beam==2 && beam_flag2==0){
		beam = 7;
		PORTC |= 0X04;
		beam_flag2 = 1;
		time_mark2 = time_point;
	}
	if(beam==3 && beam_flag3==0){
		beam = 7;
		PORTC |= 0X08;
		beam_flag3 = 1;
		time_mark3 = time_point;
	}
	if(beam==4  && beam_flag4==0){
		beam = 7;
		PORTC |= 0X10;
		beam_flag4 = 1;
		time_mark4 = time_point;
	}
	if(beam==5 && beam_flag5==0){
		beam = 7;
		PORTC |= 0X20;
		beam_flag5 = 1;
		time_mark5 = time_point;
	}
	if(beam==6 && beam_flag6==0){
		beam = 7;
		PORTC |= 0X40;
		beam_flag6 = 1;
		time_mark6 = time_point;
	}

	//turn off logic
	if(beam_flag0 = 1 && time_mark0 == (time_point+1)){
		beam_flag0 = 0;
		PORTC &= 0xFE;
	}
	if(beam_flag1 = 1 && time_mark1 == (time_point+1)){
		beam_flag1 = 0;
		PORTC &= 0xFD;
	}
	if(beam_flag2 = 1 && time_mark2 == (time_point+1)){
		beam_flag2 = 0;
		PORTC &= 0xFB;
	}
	if(beam_flag3 = 1 && time_mark3 == (time_point+1)){
		beam_flag3 = 0;
		PORTC &= 0xF7;
	}
	if(beam_flag4 = 1 && time_mark4 == (time_point+1)){
		beam_flag4 = 0;
		PORTC &= 0xEF;
	}
	if(beam_flag5 = 1 && time_mark5 == (time_point+1)){
		beam_flag5 = 0;
		PORTC &= 0xDF;
	}
	if(beam_flag6 = 1 && time_mark6 == (time_point+1)){
		beam_flag6 = 0;
		PORTC &= 0xBF;
	}
}


void array_sorter(){
	 memcpy(sorted_array, result_array, sizeof(result_array));
	 int temp = 0;
	 for(int i=1; i<7; i++){
		 temp = sorted_array[i];
		 for(int j=i; j>=1 && temp<sorted_array[j-1]; j--){
			 sorted_array[j] = sorted_array[j-1];
			 sorted_array[j-1] = temp; 
		 }
	 }
}

void ADC_quantization(){
	compare_value = sorted_array[0];   //value position in buffer
	adc_sensitive_detect[adc_channel] = (compare_value>190);
	if((compare_value>220 && compare_value<300)){
		adc_buffer[adc_channel] = 1;
	}else if(compare_value>340){
		adc_buffer[adc_channel] = 2;
	}
	
}


int main(void)
{
	
	DDRB &= 0X0F;      //reserved for push button
	DDRB |= 0X60;
	DDRC = 0xFF;	   //reserved for laser beams
	beam = 7;          //no beam active (beam from 0-6),
	timer_init();
	adc_init();
	USART_Init(MYUBRR);
	sei();
	//Midi_Switch_Sax();
    /*Clock_Prescalar();*/
	
	while (1)
	{
	}
}

ISR(ADC_vect){
	check = 1;
	result_array[adc_flag] = ADC; 
	adc_flag = (adc_flag + 1) % 7;//Sensor_Amount;
	if(adc_flag == 6){//Sensor_Amount-1){
		result_array[adc_flag] = ADC;
		array_sorter();
		ADC_quantization();
		adc_channel = (adc_channel + 1) % Sensor_Amount;//0..6
		ADC_Channel_Select(adc_channel);
	}
}

ISR(TIMER0_COMP_vect){
	check = 1;
	//count 500ms
	half_second_counter = (half_second_counter + 1) % 100;
	
	
	
	//free mode
	if(free_mode_flag){
		Hand_Detection(Sensor_Amount - 1);//ch0-2
		//change instrument button
		if(!(PINB>>7) && pre_button_flag == 1){
			led_flag = !led_flag;
			change_instrument_flag = 1;
			Instrument_Type = (Instrument_Type + 1)  % 3;
			temp_beam = (temp_beam + 1) % 7;  //button debug laser decoder
			beam = temp_beam;
		}
		if (led_flag){
			PORTB |= 0x20;
			}else if(!led_flag){
			PORTB &= 0XDF;
		}
		if (half_second_counter == 49){
			PORTB |= 0x40;
			}else if(half_second_counter == 99){
			PORTB &= 0XBF;
		}
		pre_button_flag = (PINB)>>7;
		Midi_Switch_Instrument();
	}
	
	
	//Autoplay mode
	if(Autoplay_flag){
		Demutiplexer_Output(half_second_counter);
		
		
	}
		
		
		
}

