#include <stm32f10x.h>
#include "lcd.h"
#include "ui.h"
#include "util.h"
#include "font.h"
#include "input.h"

u16 Gui_BgColor = GRAY0;

void Gui_Circle(u8 X, u8 Y, u8 R, u16 fc) 
{//Bresenham算法 
    unsigned short  a,b; 
    int c; 
    a=0; 
    b=R; 
    c=3-2*R; 
    while (a<b) 
    { 
        Lcd_DrawPoint(X+a,Y+b,fc);     //        7 
        Lcd_DrawPoint(X-a,Y+b,fc);     //        6 
        Lcd_DrawPoint(X+a,Y-b,fc);     //        2 
        Lcd_DrawPoint(X-a,Y-b,fc);     //        3 
        Lcd_DrawPoint(X+b,Y+a,fc);     //        8 
        Lcd_DrawPoint(X-b,Y+a,fc);     //        5 
        Lcd_DrawPoint(X+b,Y-a,fc);     //        1 
        Lcd_DrawPoint(X-b,Y-a,fc);     //        4 

        if(c<0) c=c+4*a+6; 
        else 
        { 
            c=c+4*(a-b)+10; 
            b-=1; 
        } 
       a+=1; 
    } 
    if (a==b) 
    { 
        Lcd_DrawPoint(X+a,Y+b,fc); 
        Lcd_DrawPoint(X+a,Y+b,fc); 
        Lcd_DrawPoint(X+a,Y-b,fc); 
        Lcd_DrawPoint(X-a,Y-b,fc); 
        Lcd_DrawPoint(X+b,Y+a,fc); 
        Lcd_DrawPoint(X-b,Y+a,fc); 
        Lcd_DrawPoint(X+b,Y-a,fc); 
        Lcd_DrawPoint(X-b,Y-a,fc); 
    } 
	
} 
//画线函数，使用Bresenham 画线算法
void Gui_Line(u8 x0, u8 y0, u8 x1, u8 y1, u16 Color) {
int dx,             // difference in x's
    dy,             // difference in y's
    dx2,            // dx,dy * 2
    dy2, 
    x_inc,          // amount in pixel space to move during drawing
    y_inc,          // amount in pixel space to move during drawing
    error,          // the discriminant i.e. error i.e. decision variable
    index;          // used for looping	


	Lcd_SetPoint(x0,y0);
	dx = x1-x0;//计算x距离
	dy = y1-y0;//计算y距离

	if (dx>=0)
	{
		x_inc = 1;
	}
	else
	{
		x_inc = -1;
		dx    = -dx;  
	} 
	
	if (dy>=0)
	{
		y_inc = 1;
	} 
	else
	{
		y_inc = -1;
		dy    = -dy; 
	} 

	dx2 = dx << 1;
	dy2 = dy << 1;

	if (dx > dy)//x距离大于y距离，那么每个x轴上只有一个点，每个y轴上有若干个点
	{//且线的点数等于x距离，以x轴递增画点
		// initialize error term
		error = dy2 - dx; 

		// draw the line
		for (index=0; index <= dx; index++)//要画的点数不会超过x距离
		{
			//画点
			Lcd_DrawPoint(x0,y0,Color);
			
			// test if error has overflowed
			if (error >= 0) //是否需要增加y坐标值
			{
				error-=dx2;

				// move to next line
				y0+=y_inc;//增加y坐标值
			} // end if error overflowed

			// adjust the error term
			error+=dy2;

			// move to the next pixel
			x0+=x_inc;//x坐标值每次画点后都递增1
		} // end for
	} // end if |slope| <= 1
	else//y轴大于x轴，则每个y轴上只有一个点，x轴若干个点
	{//以y轴为递增画点
		// initialize error term
		error = dx2 - dy; 

		// draw the line
		for (index=0; index <= dy; index++)
		{
			// set the pixel
			Lcd_DrawPoint(x0,y0,Color);

			// test if error overflowed
			if (error >= 0)
			{
				error-=dy2;

				// move to next line
				x0+=x_inc;
			} // end if error overflowed

			// adjust the error term
			error+=dx2;

			// move to the next pixel
			y0+=y_inc;
		} // end for
	} // end else |slope| > 1
}

void Gui_Rect(u8 x, u8 y, u8 x_end, u8 y_end, u16 color) {
	Lcd_SetRect(x, y, x_end + 1, y_end);
	for(u16 count = (x_end - x + 1) * (y_end - y + 1); count > 0; count--)
		Lcd_Draw(color);
}

void Gui_Box(u8 x1, u8 y1, u8 x2, u8 y2, u16 color) {
	Gui_Rect(x1,  y1,  x2, y1, color); //H
	Gui_Rect(x1,  y1,  x1, y2, color); //V
	Gui_Rect(x1,  y2,  x2, y2, color); //H
	Gui_Rect(x2,  y1,  x2, y2, color); //V
}

/**************************************************************************************
功能描述: 在屏幕显示按钮
输    入: 按钮框左上角和右下角坐标，按钮是否被按下
**************************************************************************************/
void Gui_Btn(u8 x1, u8 y1, u8 x2, u8 y2, u8 pressed) {
	Gui_Rect(x1,  y1,  x2, y1, pressed ? GRAY2 : WHITE); //H
	Gui_Rect(x1,  y1,  x1, y2, pressed ? GRAY2 : WHITE); //V
	Gui_Rect(x1,  y2,  x2, y2, pressed ? WHITE : GRAY2); //H
	Gui_Rect(x2,  y1,  x2, y2, pressed ? WHITE : GRAY2); //V
	
	//if (pressed) {
		Gui_Rect(x1,  y1+1,  x2, y1+1,  pressed ? GRAY1 : GRAY0);
		Gui_Rect(x1+1,  y1,  x1+1, y2,  pressed ? GRAY1 : GRAY0);
	//} else {
		Gui_Rect(x1,  y2-1,  x2, y2-1, pressed ? GRAY0 : GRAY1);
		Gui_Rect(x2-1,  y1,  x2-1, y2, pressed ? GRAY0 : GRAY1);
	//}
}

#define DRAW_BITS do {\
					u8 bit = 1;\
					do {\
						Lcd_Draw( font_bit&bit ? color : Gui_BgColor );\
						bit <<= 1;\
					} while(bit != 0);\
				} while(0);

/*-------------------------------------------------------------------------------
程序名称：strDisplayLen
程序描述：计算字符串长度(GB2312)
---------------------------------------------------------------------------------*/
__STATIC_INLINE u16 strDisplayLen(char *str) {
	u8 len = 0;
	while(*str++) len++;
	return len * 8;
}
/*-------------------------------------------------------------------------------
程序名称：Gui_Text_XCenter
程序描述：居中显示字符串
---------------------------------------------------------------------------------*/
void Gui_Text_XCenter(u8 y, u16 color, char *str) {
	Gui_Text((X_MAX_PIXEL - strDisplayLen(str)) / 2, y, color, str);
}


/*-------------------------------------------------------------------------------
程序名称：binarySearch
程序描述：二分搜索GB2312
---------------------------------------------------------------------------------*/
__STATIC_INLINE u16 binarySearch(const struct CNFont a[], u16 high, u16 key) {
	u16 low = 0;

	while (low <= high) {
		u16 mid = (low + high) >> 1;
		int32_t midVal = a[mid].Chr.gbk - key;

		if (midVal < 0) {
			low = mid + 1;
		} else if (midVal > 0) {
			high = mid - 1;
		} else {
			return mid;
		}
	}

	return 0xFFFF;
}
/*-------------------------------------------------------------------------------
程序名称：Gui_Text
程序描述：显示字符串
---------------------------------------------------------------------------------*/
void Gui_Text(u8 x, u8 y, u16 color, char *str) {
	const u16 start_x = x;

	while(*str) {	
		u8 c = *str;
		if(c < 128) {
			str++;
			// CRLF
			if (c == '\n') {
				x = start_x;
				y += 16;
			} else {
				struct ENFont font = en_16[c - 32];

				Lcd_SetRect(x, y, x+8, y+16);
			  for(u8 h=0;h<16;h++) {
					u8 font_bit = font.Bit[h];
					DRAW_BITS
				}

				x+=8;
			}
			continue;
		}

		u16 idx = binarySearch(cn_16,(sizeof(cn_16)/sizeof(struct CNFont))-1,*((u16*)str));
		str+=2;
		struct CNFont font = idx == 0xFFFF ? cn_404 : cn_16[idx];

		Lcd_SetRect(x, y, x+16, y+16);
		for(u8 i=0;i<32;i += 2) {
				u8 font_bit = font.Bit[i];
				DRAW_BITS
				font_bit = font.Bit[i+1];
				DRAW_BITS
		}

		x+=16;
	}
}

//显示ASCII字符
void Gui_Char(u8 x, u8 y, u16 color, u8 c) {
	struct ENFont font = en_16[(c&0x7F) - 32];

	Lcd_SetRect(x, y, x+8, y+16);
	for(u8 h=0;h<16;h++) {
		u8 font_bit = font.Bit[h];
		DRAW_BITS
	}
}

//m^n
int	oled_pow(u8 m,u8 n) {
	u32 result=1;	 
	while(n--)result*=m;    
	return result;
}

void _CheckPoint() {
	
}

/*-------------------------------------------------------------------------------
程序名称：Gui_Num_Float
程序描述：显示int32
程序参数：point 大于0时，显示为n位伪小数
          len 填充数字之前len个字符的空间为背景色 (如果数字长度不够)
示    例：12345 123.45 (point=2)
---------------------------------------------------------------------------------*/
void Gui_Num_Float(u8 x, u8 y, u16 color, int8_t point, int32_t i, int8_t len) {
	u16 initial_x = x;
	x += len * 8;

	u8 isNeg = i < 0;
	if (isNeg) i = -i;

	int32_t r, q;
	while (i >= 65536) {
		q = i / 100;
		// really: r = i2 - (q * 100);
		r = i - ((q << 6) + (q << 5) + (q << 2));
		i = q;

		Gui_Char(x -= 8, y, color, (r%10)+'0');
		if (--point == 0) Gui_Char(x -= 8, y, color, '.');
		Gui_Char(x -= 8, y, color, (r/10)+'0'); 
		if (--point == 0) Gui_Char(x -= 8, y, color, '.');
	}

	do {
		q = (uint32_t)(i * 52429) >> (16 + 3);
		r = i - ((q << 3) + (q << 1));  // r = i2-(q2*10) ...
		i = q;

	 	Gui_Char(x -= 8, y, color, r+'0');
		if (--point == 0) {
			Gui_Char(x -= 8, y, color, '.');
			if (!i) isNeg |= 2;
		}
	} while (i);
	
	if (point > 0) {
		while(point--) Gui_Char(x -= 8, y, color, '0');
		Gui_Char(x -= 8, y, color, '.');
		isNeg |= 2;
	}

	if (isNeg&2) Gui_Char(x -= 8, y, color, '0'); 
	if (isNeg&1) Gui_Char(x -= 8, y, color, '-');
	// 填充之前数字可能占用的空间
	if (x > initial_x) {
		len = (x - initial_x) >> 3;
		while(len--) Gui_Char(x -= 8, y, color, ' '); 
	}
}

/*-------------------------------------------------------------------------------
程序名称：Gui_Num_4Time
程序描述：显示两位以:分隔的时间，例如23:59
---------------------------------------------------------------------------------*/
void Gui_Num_4Time(u8 x, u8 y, u16 color, u16 i) {
	u8 p = i/600;
	Gui_Char(x, y, color, p+'0');
	i -= p * 600;
	p = i/60;
	Gui_Char(x + 8, y, color, p+'0'); 
	i -= p*60;
	Gui_Char(x + 16, y, color, ':');
	p = i/10;
	Gui_Char(x + 24, y, color, p+'0'); 
	i -= p*10;
	Gui_Char(x + 32, y, color, i+'0');
}


#ifdef UI_ENABLE_BCD
//显示32*32的数码管字体
void Gui_Num_BCD(u8 x, u8 y, u16 color, u8 num) {
	unsigned char i,j,k,c;

	struct BCDFont bcd = bcd32[num];
	
	Lcd_SetRect(x, y, x+32, y+32);
  for(i=0;i<32;i++) {
		for(j=0;j<4;j++) {
			c = bcd.Bit[i*4+j];
			for (k=0;k<8;k++)	{
				Lcd_Draw( c&(0x80>>k) ? color : Gui_BgColor);
			}
		}
	}
}
#endif
