/*
 * lcd.c
 *
 *  Created on: Nov 5, 2023
 *      Author: ckesp
 */
#include "lcd.h"
#include "stm32g0xx_hal.h"

// Based off of: https://cdn-shop.adafruit.com/datasheets/TC1602A-01T.pdf

/*
 * This function creates a command to delay in microseconds.
 *
 * In "Write Mode" there is a minimum E (Enable) Pulse Width defined
 * in the datasheet on pg 7. Needed to define a microsecond delay for
 * when the enable pin is used
*/

#define timer htim1

extern TIM_HandleTypeDef timer;

void Delay(uint16_t microseconds)
{
	__HAL_TIM_SET_COUNTER(&timer, 0);
	while(__HAL_TIM_GET_COUNTER(&timer) < microseconds);
}

/*
 * This function will be used to transmit the data to the internal RAM.
 *
 * It first defines the data packet to be transmitted as data or
 * instruction code. Data would be considered a symbol to be displayed,
 * whereas commands are like clearing display, or returning the cursor
 * to its original position. These are outlined on p11 of the datasheet.
 *
 * It then fills writes the data pins high or low. It is not until the
 * enable pin that the data pins' values are actually written onto a
 * register within the control IC.
*/

void LCD_Transmit(char data, int rs)
{
	// Datasheet: RS H:DATA, L:Instruction Code
	HAL_GPIO_WritePin(RS_GPIO_Port, RS_Pin, rs);
	// Write data into internal RAM
	HAL_GPIO_WritePin(DB7_GPIO_Port, DB7_Pin, ((data>>3)&0x01)); //i.e. 0b0000 1000 gets shifted 0b0000 0001 then &ed w/ 0x01 to get 0x01 (HIGH)
	HAL_GPIO_WritePin(DB6_GPIO_Port, DB6_Pin, ((data>>2)&0x01)); //i.e. 0b0000 1000 gets shifted 0b0000 0010 then &ed w/ 0x01 to get 0x00 (LOW)
	HAL_GPIO_WritePin(DB5_GPIO_Port, DB5_Pin, ((data>>1)&0x01));
	HAL_GPIO_WritePin(DB4_GPIO_Port, DB4_Pin, ((data>>0)&0x01));
	//Toggle EN_Pin to send data
	HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, 1);
	Delay(20);
	HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, 0);
	Delay(20);
}

/*
 * This function defines the RS bit at 0 to specify that the
 * following bits are for one of the commands outline on p11.
 *
 * Since we are using 4-bit interfacing, 8-bit data requires two
 * times transferring. p6 of datasheet says to send higher 4-bit data
 * first, then lower 4-bit data.
*/

void LCD_Command(char cmd)
{
	char datasend;

	datasend = ((cmd>>4)&0x0F);
	LCD_Transmit(datasend,0);

	datasend = ((cmd)&0x0F);
	LCD_Transmit(datasend, 0);
}

/*
 * This function defines the RS bit at 1 to specify that the
 * following bits are for data, representing a symbol. This is
 * outlined on p11.
 *
 * Since we are using 4-bit interfacing, 8-bit data requires two
 * times transferring. p6 of datasheet says to send higher 4-bit data
 * first, then lower 4-bit data.
*/

void LCD_Data(char data)
{
	char datasend;

	datasend = ((data>>4)&0x0F);
	LCD_Transmit(datasend,1);

	datasend = ((data)&0x0F);
	LCD_Transmit(datasend, 1);
}

/*
 * This function clears the LCD by sending the appropriate
 * command defined in p11 of the datasheet. 0b000 0001 or 0x01.
*/

void LCD_Clear(void)
{

	LCD_Command(0x01);
	HAL_Delay(2);
}

/*
 * This function places the cursor where you want the text to
 * start. Generally would use before a LCD_String. Its parameters
 * define the location on the 2x16 LCD screen.
 *
 * p14 defines the commands for setting the Digital Display RAM
 * address, DB7 must be 1. p14 defines the location on the 2x16 LCD.
 *
 * 0x8# is associated with the first row's columns
 * 0xC# is associated with the second row's columns
*/

void LCD_Put_Cursor(int row, int col)
{
	switch(row)
	{
	case 0:
		col |= 0x80; //0b1000 0000
		break;
	case 1:
		col |= 0xC0; //0b1100 0000
		break;
	}
	LCD_Command(col);
}

/*
 * This function is the initialization for 4-bit interfacing
 * defined on p13. We decided to use a 4-bit interface so that
 * there are less pins/nets in our design.
 *
 * p11 "In the operation conditions under -20C ~ 75C, the
 * maximum execution time for majority of instruction sets is
 * 100us..."
*/

void LCD_Init(void)
{
	HAL_Delay(50); // Wait time > 15ms after VDD > 4.5V, Wait time > 40ms after VDD > 2.7V (explicitly specified)
	LCD_Command(0x30); // Command explicitly specified on p13
	HAL_Delay(5); // Wait time > 4.1ms (explicitly specified)
	LCD_Command(0x30); // Command explicitly specified on p13
	HAL_Delay(1); // Wait time > 100us (explicitly specified)
	LCD_Command(0x30); // Command explicitly specified on p13
	HAL_Delay(10);
	LCD_Command(0x20); // Command explicitly specified on p13, 0b0010 0000 Set interface to be 4 bits length
	HAL_Delay(10);

	LCD_Command(0x28); // 0b0010 1000 Function Set command from p11 N HIGH = 2-line and still 4 bits length
	HAL_Delay(1);
	LCD_Command(0x08); // Command explicitly specified on p13, Display Off p11
	HAL_Delay(1);
	LCD_Command(0x01); // Command explicitly specified on p13, Clear Display p11
	HAL_Delay(1);
	LCD_Command(0x06); // Command explicitly specified on p13, Entry Mode Set p11
	HAL_Delay(1);
	LCD_Command(0x0C); // Set the Digital Display RAM (DDRAM) Address
}

/*
 * This function send the symbols of a specified string one at a time
 * utilizing LCD_Data until it has gone through the entire string.
*/

void LCD_String(char *str)
{
	while(*str) LCD_Data(*str++);
}

