#ifndef __LCD_H
#define __LCD_H
#include "stm32f10x.h"

///////////////////////////////////////////////////////////////////////////////
//  文 件 名   : lcd.h
//  版 本 号   : v3.0
//  功能描述   : LCD接口
///////////////////////////////////////////////////////////////////////////////

#define RED  	  0xf800
#define GREEN	  0x07e0
#define BLUE 	  0x001f
#define YELLOW  0xFFE0
#define WHITE	  0xffff
#define BLACK	  0x0000

#define GRAY0   0xEF7D   	//灰色0 3165 00110 001011 00101
#define GRAY1   0x8410    //灰色1      00000 000000 00000
#define GRAY2   0x4208    //灰色2      11111 111110 11111

#define X_MAX_PIXEL	  160
#define Y_MAX_PIXEL	  80
#define LCD_ROW_OFFSET 0x19

//#define LCD_BACKLIGHT GPIO_Pin_0
//#define	LCD_BL_SET  	GPIOB->BSRR=LCD_BACKLIGHT 
//#define	LCD_BL_CLR  	GPIOB->BRR=LCD_BACKLIGHT 

void Lcd_Init(void);
void Lcd_SetRotation(u8 rotation);

void Lcd_Clear(u16 color);

void Lcd_SetPoint(u8 x, u8 y);
void Lcd_SetRect(u8 x_start, u8 y_start, u8 x_end, u8 y_end);
void Lcd_Draw(u16 color);

void Lcd_DrawPoint(u8 x, u8 y, u16 color);

struct Image {
	u8 width, height;
	const u16 *colors;
};
void Lcd_DrawImage(u8 x, u8 y, struct Image image);

#endif
