/*
 * lcd.h
 *
 *  Created on: Nov 5, 2023
 *      Author: ckesp
 */
#ifndef LCD_H
#define LCD_H

#include "stm32g0xx_hal.h"
#include "main.h"

void LCD_Init(void);
void LCD_Command(char cmd);
void LCD_Data(char data);
void LCD_String(char *str);
void LCD_Put_Cursor(int row, int col);
void LCD_Clear(void);

#endif /* LCD_H */
