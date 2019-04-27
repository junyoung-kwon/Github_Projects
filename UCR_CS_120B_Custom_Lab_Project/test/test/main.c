/*
 * test.c
 *
 * Created: 2/26/2019 11:20:17 AM
 * Author : ericj
 */ 

#define F_CPU 8000000UL


#define LCD_RST_set  PORTB |=  (1<<0)    //external reset input
#define LCD_RST_clr  PORTB &=~ (1<<0)

#define LCD_DC_set   PORTB |=  (1<<1)    //data/commande
#define LCD_DC_clr   PORTB &=~ (1<<1)

#define SDIN_set     PORTB |=  (1<<2)    //serial data input
#define SDIN_clr     PORTB &=~ (1<<2)

#define SCLK_set     PORTB |=  (1<<3)    //serial clock input
#define SCLK_clr     PORTB &= ~(1<<3)


#include <avr/io.h>
#include "io.c"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include "english_font.h"
#include "animation.h"
#include "timer.h"

void LCD_write_byte(unsigned char dat, unsigned char command)
{
	unsigned char i;

	if (command == 1)
	LCD_DC_clr;
	else
	LCD_DC_set;

	for(i=0;i<8;i++)
	{
		if(dat&0x80)
		SDIN_set;
		else
		SDIN_clr;
		SCLK_clr;
		dat = dat << 1;
		SCLK_set;
	}
}

void NLCD_init()
{
	LCD_RST_clr;
	_delay_us(1);
	LCD_RST_set;

	_delay_us(1);

	LCD_write_byte(0x21, 1); // set LCD mode
	LCD_write_byte(0xc8, 1); // set bias voltage
	LCD_write_byte(0x06, 1); // temperature correction
	LCD_write_byte(0x13, 1); // 1:48
	LCD_write_byte(0x20, 1); // use bias command, vertical
	LCD_write_byte(0x0c, 1); // set LCD mode,display normally
	LCD_clear();             // clear the LCD
}

void LCD_clear()
{
	unsigned int i;

	LCD_write_byte(0x0c, 1);
	LCD_write_byte(0x80, 1);

	for (i=0; i<504; i++)
	{
		LCD_write_byte(0, 0);
	}
}

void LCD_set_XY(unsigned char X, unsigned char Y)
{
	LCD_write_byte(0x40 | Y, 1); // column
	LCD_write_byte(0x80 | X, 1);    // row
}

void LCD_write_char(unsigned char c)
{
	unsigned char line;

	c -= 32;

	for (line=0; line<6; line++)
	LCD_write_byte(font6x8[c][line], 0);
}

void LCD_write_english_string(unsigned char X,unsigned char Y,char *s)
{
	LCD_set_XY(X,Y);
	while (*s)
	{
		LCD_write_char(*s);
		s++;
	}
}

void adc_init()
{
	// AREF = AVcc
	ADMUX = (1<<REFS0);
	
	// ADC Enable and prescaler of 128
	// 16000000/128 = 125000
	ADCSRA = (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);
}

uint16_t adc_read(uint8_t ch)
{
	// select the corresponding channel 0~7
	// ANDing with ’7? will always keep the value
	// of ‘ch’ between 0 and 7
	ch &= 0b00000111;  // AND operation with 7
	ADMUX = (ADMUX & 0xF8)|ch; // clears the bottom 3 bits before ORing
	
	// start single convertion
	// write ’1? to ADSC
	ADCSRA |= (1<<ADSC);
	
	// wait for conversion to complete
	// ADSC becomes ’0? again
	// till then, run loop continuously
	while(ADCSRA & (1<<ADSC));
	
	return (ADC);
}


void CreateCustomCharacter (unsigned char *Pattern, const char Location)
{ 
	int i=0; 
	LCD_WriteCommand(0x40+(Location * 8));
	for (i=0; i<8; i++)
	LCD_WriteData(Pattern[ i ]); 
}

unsigned char Timerflag = 0;

void Timerisr(){
	Timerflag = 1;
}

enum Flag_States {Start, Display, Choose, Left, Right, DownUp, End} Flag_State;
	



//srand(time(0));

int index = 0;
uint8_t count = 0;
int acc;
void Choose_Flag(){
	unsigned char val = ~PINA;
	unsigned char rst = ~PINA & 0x08;
	int16_t x,y;
	
	//srand(time(0));

	unsigned char *str, *str2;
	str = (unsigned char*) malloc(sizeof(unsigned char) * 5);
	str2 = (unsigned char*) malloc(sizeof(unsigned char) * 5);
	//str3 = (unsigned char*) malloc(sizeof(unsigned char) * 5);

	unsigned char result = 0x00;
	unsigned char *flag_array [] = {flag_both_sprite, flag_left_sprite, flag_right_sprite};
	unsigned char *ch;
	unsigned char *hs = 0xAA;
	CreateCustomCharacter(Pattern1, 1);
	eeprom_write_byte(hs, eeprom_read_byte(hs));
	

	
	switch(Flag_State){
		case Start:
			count = 0;
			acc = 1;
			//eeprom_write_byte(hs, 0);
			LCD_ClearScreen();
			LCD_WriteCommand(0x86);
			LCD_WriteData(1);
			LCD_DisplayString(2, " Welcome!");
			LCD_WriteCommand(0x8B);
			LCD_WriteData(1);
			if(val != 0x04){
				Flag_State = Start;
			}else if(val == 0x04){
				Flag_State = Display;
			}
			break;
		case Display:
			index = rand() % 3;
			Flag_State = Choose;
			break;
		case Choose:
		//Flag_State = Display;
	//		srand(time(0));
			if(index == 0){
				Flag_State = DownUp;
			}else if(index == 1){
				Flag_State = Left;
			}else if(index == 2){
				Flag_State = Right;
			}
			break;
		case Left:
			if(acc == 1){
				Flag_State = Choose;
			}else{
				Flag_State = End;
			}
			break;
		case Right:
			if(acc == 1){
				Flag_State = Choose;
			}else{
				Flag_State = End;
			}
			break;
		case DownUp:
			if(acc == 1){
				Flag_State = Choose;
			}else{
				Flag_State = End;
			}
			break;
		case End:
			if(!rst){
				Flag_State = End;
			}else if(rst){
				Flag_State = Start;
			}
			break;
		default:
			//Flag_State = Start;
			break;
	}
	switch(Flag_State){
		case Start:
			LCD_clear();
			LCD_write_english_string(0,2,"  Flag Game ");
			LCD_write_english_string(0,4," by Eric Kwon ");
			LCD_write_english_string(0,5," LB to start ");
			break;
		case Display:
			break;
		case Choose:
			LCD_clear();
			for(int n = 0; n < 504; n++)
			{
				LCD_write_byte( flag_array[index][n], 0);
			}
			break;			
		case Left:
			x = adc_read(0);
			y = adc_read(1);
			
			result = 0x00;
			itoa(x, str, 10);
			itoa(y, str2, 10);
			LCD_ClearScreen();
			LCD_DisplayString(1, "Choose a direction");
			
			if(y >= 1000 && flag_array[index] == flag_left_sprite){
				LCD_ClearScreen();
				LCD_DisplayString(1, "CORRECT");
				acc = 1;
				count++;
				index = rand() % 3;
			}else if(y < 100 || x >= 1000 || x < 325){
				LCD_ClearScreen();
				LCD_DisplayString(1, "WRONG");
				acc = 0;
				//index = rand() % 3;
			}
			break;
		case Right:
			x = adc_read(0);
			y = adc_read(1);
			
			result = 0x00;
			itoa(x, str, 10);
			itoa(y, str2, 10);
			LCD_ClearScreen();
			LCD_DisplayString(1, "Choose a direction");
			
			if(y < 100 && flag_array[index] == flag_right_sprite){
				LCD_ClearScreen();
				LCD_DisplayString(1, "CORRECT");
				acc = 1;
				count++;
				index = rand() % 3;
			}else if(y >= 1000 || x >= 1000 || x < 325){
				LCD_ClearScreen();
				LCD_DisplayString(1, "WRONG");
				acc = 0;
				//index = rand() % 3;
			}
			break;
		case DownUp:
			x = adc_read(0);
			y = adc_read(1);
			
			result = 0x00;
			itoa(x, str, 10);
			itoa(y, str2, 10);
			
			LCD_ClearScreen();
			LCD_DisplayString(1, "Choose a direction");
			
			if((x >= 1000 || x < 325) && flag_array[index] == flag_both_sprite){
				LCD_ClearScreen();
				LCD_DisplayString(1, "CORRECT");
				acc = 1;
				count++;
				index = rand() % 3;
			}else if(y < 100 || y >= 1000){
				LCD_ClearScreen();
				LCD_DisplayString(1, "WRONG");
				acc = 0;
				//index = rand() % 3;
			}
			break;
		case End:
			LCD_clear();
			LCD_Cursor(0);
			//unsigned char *score_str, *scorest_str, *highscore_str;
			char score_str[3];
			char highscore_str[3];
			itoa(count, score_str, 10);
			//scorest_str = "Score: ";
			//strcat(scorest_str, score_str);
			LCD_ClearScreen();
			LCD_DisplayString(1, "Score: ");
			LCD_DisplayString(7, score_str);
			if(eeprom_read_byte(hs) < count){
				eeprom_write_byte(hs, count);
			}
			itoa(eeprom_read_byte(hs), highscore_str, 10);
			LCD_DisplayString(17, "High Score: ");
			LCD_DisplayString(28, highscore_str);
			//LCD_clear();
			LCD_write_english_string(0,2,"  RB to reset ");
			//acc = 1;
			break;
		default:
			break;
	}
}



int main(void)
{
	PORTA = 0xFF;	DDRA = 0x00;
	PORTB = 0x00;	DDRB = 0xFF;
	PORTC = 0x00;	DDRC = 0xFF;
	PORTD = 0x00;	DDRD = 0xFF;
	//DDRD = 0x0F;

	srand(time(0));
	
	NLCD_init();       //LCD initialization
	
	adc_init();	
	
	LCD_init();

	//eeprom_write_byte(0xAA, 0);

	/*int16_t x,y;
		
	unsigned char *str, *str2;
	str = (unsigned char*) malloc(sizeof(unsigned char) * 5);
	str2 = (unsigned char*) malloc(sizeof(unsigned char) * 5);
	unsigned char result = 0x00;	
	unsigned char *flag_array [] = {flag_both_sprite, flag_left_sprite, flag_right_sprite};
	unsigned char *ch;*/
	
	
	//itoa((int)((double)rand() / ((double)RAND_MAX + 1) * 3), ch, 10);
	//LCD_DisplayString(1, ch);
	//LCD_DisplayString(1, "X-Value: ");
	//LCD_Cursor(17);
	//LCD_DisplayString(17, "Y-Values: ");
	
// 	LCD_write_english_string(0,0," Hello World ! ");
// 	LCD_write_english_string(0,1," bananas ");
// 	LCD_write_english_string(0,2,"dancing potato");
// 	LCD_write_english_string(0,3," tomato");
// 	LCD_write_english_string(0,4,"   with love ");
// 	LCD_write_english_string(0,5,"   from 4a4ik ");

	TimerSet(200);
	TimerOn();
    Flag_State = Start;

	
	
	/* Replace with your application code */
    while (1)
    {		
		Choose_Flag();
		while(!TimerFlag){}
		TimerFlag = 0;
	/*	x = adc_read(0);
		y = adc_read(1);
		
		result = 0x00;
		itoa(x, str, 10);
		if(x >= 1000){
			int index = rand() % 3 ;						
			for(int n = 0; n < 504; n++)
			{
				LCD_write_byte( flag_array[index][n], 0);
			}
			result = 0x01;
		}else if(x < 325){
			for(int n = 0; n < 504; n++)
			{
				LCD_write_byte( flag_both_sprite[ n ], 0);
			}
			result = 0x02;
		}
		//LCD_DisplayString(10, str);
		
		itoa(y, str2, 10);
		if(y >= 1000){
			for(int n = 0; n < 504; n++)
			{
				LCD_write_byte( flag_left_sprite[ n ], 0);
			}
			result = 0x04;
		}else if(y < 100){
			for(int n = 0; n < 504; n++)
			{
				LCD_write_byte( flag_right_sprite[ n ], 0);
			}
			result = 0x08;
		}*/
		//LCD_DisplayString(25, str2);
		//delay_ms(250);
		
	
		//PORTB = result;
	 
		//continue;
    }
}

