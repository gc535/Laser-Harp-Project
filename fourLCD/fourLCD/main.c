#include <avr/io.h>
#include "lcd.h"

int main(void)
{

	lcd_init(LCD_DISP_ON);
	lcd_clrscr();
	lcd_puts("Hola viteh :)\n");
	lcd_puts("Bueno chau :(");
	
	return 0;
}

