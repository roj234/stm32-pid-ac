


#include <stm32f10x.h>
#include "lcd.h"
#include "util.h"

#define LCD_NOP			0x00	//空命令
#define LCD_SWRESET		0x01	//软件复位，在睡眠和显示模式下，重置软件后需等待120ms后方可执行下一条指令

#define LCD_RDDID		0x04	//读取LCD的制造商ID（8位）、驱动版本ID（最高位为1，7位）、驱动程序ID（8位）
#define LCD_RDDST		0x09	//读取显示屏所有状态参数
#define LCD_RDDPM		0x0A	//读取显示屏能量模式
#define LCD_RDDMADCTL	0x0B	//读取显示屏MADCTL
#define LCD_RDDCOLMOD	0x0C	//读取显示屏像素定义
#define LCD_RDDIM		0x0D	//读取显示屏图片模式
#define LCD_RDDSM		0x0E	//读取显示屏单信号模式
#define LCD_RDDSDR		0x0F	//读取显示屏自我诊断结果

#define LCD_SLPIN		0x10	//进入最小功耗模式
#define LCD_SLPOUT 		0x11	//关闭睡眠模式
#define LCD_PTLON		0x12	//打开Partial模式
#define LCD_NORON		0x13	//恢复到正常模式
#define LCD_INVOFF		0x20	//显示反转模式中恢复
#define LCD_INVON		0x21	//进入反向显示模式
#define LCD_GAMSET		0x26	//当前显示选择所需的伽马曲线
#define LCD_DISPOFF		0x28	//关闭显示，帧内存的输出被禁用
#define LCD_DISPON		0x29	//开启显示，帧内存的输出被启用
#define LCD_CASET		0x2A	//列地址设置，每个值代表帧内存中的一列
#define LCD_RASET		0x2B	//行地址设置，每个值代表帧内存中的一列
#define LCD_RAMWR		0x2C	//写入内存
#define LCD_RGBSET		0x2D	//颜色模式设置
#define LCD_RAMRD		0x2E	//读取内存
#define LCD_PTLAR		0x30	//部分模式的显示区域设置
#define LCD_SCRLAR		0x33	//定义垂直滚动区域的显示
#define LCD_TEOFF		0x34	//关闭(Active Low) TE信号线的撕裂效应输出信号
#define LCD_TEON		0x35	//打开TE信号线的撕裂效果输出信号
#define LCD_MADCTL		0x36	//定义帧内存的读写扫描方向
#define LCD_MADCTL_MX  0x40
#define LCD_MADCTL_MY  0x80
#define LCD_MADCTL_MV  0x20
#define LCD_MADCTL_ML  0x10
#define LCD_MADCTL_RGB 0x08
#define LCD_MADCTL_MH  0x04
#define LCD_VSCSAD		0x37	//设置垂直滚动起始地址，此命令与垂直滚动定义(33h)一起使用
#define LCD_IDMOFF		0x38	//关闭空闲模式
#define LCD_IDMON			0x39	//开启空闲模式
#define LCD_COLMOD		0x3A	//定义通过MCU接口传输的RGB图片数据的格式
#define LCD_FRMCTR1		0xB1	//设置全色正常模式的帧频
#define LCD_FRMCTR2 	0xB2	//设置空闲模式的帧频
#define LCD_FRMCTR3 	0xB3	//设置部分模式/全色的帧频率
#define LCD_INVCTR 		0xB4	//反转模式控制
#define LCD_PWCTR1 		0xC0	//设置AVDD、MODE、VRHP、VRHN
#define LCD_PWCTR2 		0xC1	//设置VGH与VGL的供电功率
#define LCD_PWCTR3 		0xC2	//设置正常模式/全色模式下的运放的电流
#define LCD_PWCTR4 		0xC3	//设置空闲模式/八色模式下的运放的电流
#define LCD_PWCTR5 		0xC4	//设置部分模式/全色模式下的运放的电流
#define LCD_VMCTR1 		0xC5	//设置VCOM电压电平以减少闪烁问题
#define LCD_VMOFCTR		0xC7	//VCOM偏移控制，在使用命令0xC7之前，命令0xD9的位VMF_EN必须启用(设置为1)
#define LCD_WRID2			0xD1	//写入LCD模块版本的7位数据，保存到NVM
#define LCD_WRID3			0xD2	//写入项目代码模块的8位数据，保存到NVM
#define LCD_NVFCTR1		0xD9	//NVM状态控制
#define LCD_RDID1			0xDA	//读字节返回8位LCD模块的制造商ID
#define LCD_RDID2			0xDB	//读字节返回8位LCD模块/驱动程序版本ID
#define LCD_RDID3			0xDC	//读字节返回8位LCD模块/驱动ID
#define LCD_NVFCTR2		0xDE	//NVM读命令
#define LCD_NVFCTR3		0xDF	//NVM写命令
#define LCD_GMCTRP1		0xE0	//Gamma ‘+’ Polarity Correction Characteristics Setting
#define LCD_GMCTRN1		0xE1	//Gamma ‘+’ Polarity Correction Characteristics Setting
#define LCD_PWCTR6		0xFC	//Set the amount of current in Operational amplifier in Partial mode + Idle mode. 


#define Lcd_Cmd GPIOB->BRR = GPIO_Pin_1
#define Lcd_Data GPIOB->BSRR = GPIO_Pin_1

//向SPI总线传输一个8位数据
void Lcd_Write(u8 data) {
	SPI_I2S_SendData( SPI1, data );
	while( !( SPI1->SR & SPI_I2S_FLAG_TXE) );
	//while( ( SPI1->SR & SPI_I2S_FLAG_BSY) );
}

void Lcd_Cmd_Write(u8 cmd) {
	Lcd_Cmd;
  Lcd_Write(cmd);
	Lcd_Data;
}

void Lcd_SetRotation(u8 inverse) {
  Lcd_Cmd_Write(LCD_MADCTL);
  Lcd_Write((inverse ? LCD_MADCTL_MY : LCD_MADCTL_MX) | LCD_MADCTL_MV | LCD_MADCTL_RGB);
}

void Lcd_Init() {
	//A4-A7 SPI1
	SPI_InitTypeDef LCD_SPI;
	LCD_SPI.SPI_BaudRatePrescaler	= SPI_BaudRatePrescaler_2;	// 使分频值达到最大
	LCD_SPI.SPI_CPHA				= SPI_CPHA_1Edge;			// 上降沿有效
	LCD_SPI.SPI_CPOL				= SPI_CPOL_High;				// 高电平有效
	LCD_SPI.SPI_CRCPolynomial		= 7;						// 无 CRC 校验
	LCD_SPI.SPI_DataSize			= SPI_DataSize_8b;			// 发送字长为 8 位
	LCD_SPI.SPI_Direction			= SPI_Direction_1Line_Tx;	// 方向为发送
	LCD_SPI.SPI_FirstBit			= SPI_FirstBit_MSB;			// 高位先行
	LCD_SPI.SPI_Mode				= SPI_Mode_Master;			// 主机模式
	LCD_SPI.SPI_NSS					= SPI_NSS_Soft;				// 软件控制
	SPI_Init( SPI1, &LCD_SPI );
	SPI_Cmd( SPI1, ENABLE );

	delay_ms(1);
	//B0脚置高电平(Init的时候低了)
	GPIOB->BSRR = GPIO_Pin_10;
	delay_ms(50);
	
	//控制芯片初始化
	//详细信息查看Datasheet: 10.2 Panel Function Command List and Description
	Lcd_Cmd;
	Lcd_Write(LCD_SLPOUT);
	// 反色
	Lcd_Write(LCD_INVON);

	// RGB 565 16bit/pixel
	Lcd_Cmd_Write(LCD_COLMOD);
	Lcd_Write(0x05);

	// 伽马曲线
	Lcd_Cmd_Write(LCD_GMCTRP1);
	Lcd_Write(0x10);
	Lcd_Write(0x0E);
	Lcd_Write(0x02);
	Lcd_Write(0x03);
	Lcd_Write(0x0E);
	Lcd_Write(0x07);
	Lcd_Write(0x02);
	Lcd_Write(0x07);
	Lcd_Write(0x0A);
	Lcd_Write(0x12);
	Lcd_Write(0x27);
	Lcd_Write(0x37);
	Lcd_Write(0x00);
	Lcd_Write(0x0D);
	Lcd_Write(0x0E);
	Lcd_Write(0x10);

	Lcd_Cmd_Write(LCD_GMCTRN1);
	Lcd_Write(0x10);
	Lcd_Write(0x0E);
	Lcd_Write(0x03);
	Lcd_Write(0x03);
	Lcd_Write(0x0F);
	Lcd_Write(0x06);
	Lcd_Write(0x02);
	Lcd_Write(0x08);
	Lcd_Write(0x0A);
	Lcd_Write(0x13);
	Lcd_Write(0x26);
	Lcd_Write(0x36);
	Lcd_Write(0x00);
	Lcd_Write(0x0D);
	Lcd_Write(0x0E);
	Lcd_Write(0x10);

#ifdef USE_CUSTOM_FRMCTR
	Lcd_Cmd_Write(LCD_FRMCTR1); 
	Lcd_Write(0x05);
	Lcd_Write(0x3A);
	Lcd_Write(0x3A);

	Lcd_Cmd_Write(LCD_FRMCTR2);
	Lcd_Write(0x05);
	Lcd_Write(0x3A);
	Lcd_Write(0x3A);

	Lcd_Cmd_Write(LCD_FRMCTR3); 
	Lcd_Write(0x05);  
	Lcd_Write(0x3A);
	Lcd_Write(0x3A);
	Lcd_Write(0x05);
	Lcd_Write(0x3A);
	Lcd_Write(0x3A);
#endif
#ifdef USE_CUSTOM_PWCTR
	Lcd_Cmd(LCD_PWCTR1);
	Lcd_Data(0x1F);
	Lcd_Data(112);

	Lcd_Cmd(LCD_PWCTR2);
	Lcd_Data(0x00);

	Lcd_Cmd(LCD_PWCTR3);
	Lcd_Data(0x01);
	Lcd_Data(0x01);
#endif

	Lcd_Cmd_Write(LCD_VMCTR1);
	Lcd_Write(0x64);
	Lcd_Write(0x64);

	Lcd_SetRotation(0);//设置图像旋转角度
	
	Lcd_Cmd;
	Lcd_Write(LCD_DISPON);
}

//u16 GraphicMemory[X_MAX_PIXEL * Y_MAX_PIXEL];

/*************************************************
函数：Lcd_Clear
功能：清屏
参数：填充颜色
返回：无
*************************************************/
void Lcd_Clear(u16 Color) {
   Lcd_SetRect(0,0,X_MAX_PIXEL,Y_MAX_PIXEL);
   for(u16 i=0;i<X_MAX_PIXEL*Y_MAX_PIXEL;i++) {
	  	Lcd_Draw(Color);
	 }
}

/*************************************************
函数：Lcd_SetPoint
功能：设置lcd显示起始点
参数：xy坐标
返回：无
*************************************************/
void Lcd_SetPoint(u8 x,u8 y) {Lcd_SetRect(x, y, x+1, y+1);}
/*************************************************
函数：Lcd_SetRect
功能：设置lcd显示区域，在此区域写数据自动换行
参数：xy起点和终点
返回：无
*************************************************/
void Lcd_SetRect(u8 x_start, u8 y_start, u8 x_end, u8 y_end) {
	Lcd_Cmd_Write(LCD_CASET);
	Lcd_Draw(x_start+1);
	Lcd_Draw(x_end);

	Lcd_Cmd_Write(LCD_RASET);
	Lcd_Draw(y_start+LCD_ROW_OFFSET+1);
	Lcd_Draw(y_end+LCD_ROW_OFFSET);
	
	Lcd_Cmd_Write(LCD_RAMWR);
}
/*************************************************
函数：Lcd_Draw
功能：向液晶屏写一个16位数据
参数：data
返回：无
*************************************************/
void Lcd_Draw(u16 data) {
	 Lcd_Write(data>>8);
	 Lcd_Write(data);
}
/*************************************************
函数：Lcd_DrawPoint
功能：画一个点
参数：位置和颜色
返回：无
*************************************************/
void Lcd_DrawPoint(u8 x, u8 y,u16 color) {
	Lcd_SetPoint(x, y);
	Lcd_Draw(color);
}
/*************************************************
函数：Lcd_DrawImage
功能：画一个图像
参数：位置，图像
返回：无
*************************************************/
void Lcd_DrawImage(u8 x, u8 y, struct Image image) {
	Lcd_SetRect(x, y, x + image.width, y + image.height);
	const u16 *colors = image.colors;
	for(u16 count = image.width * image.height; count > 0; count--) {
	  Lcd_Draw(* colors++);
	}
}
