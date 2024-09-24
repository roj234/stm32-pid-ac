


#include <stm32f10x.h>
#include "lcd.h"
#include "util.h"

#define LCD_NOP			0x00	//������
#define LCD_SWRESET		0x01	//�����λ����˯�ߺ���ʾģʽ�£������������ȴ�120ms�󷽿�ִ����һ��ָ��

#define LCD_RDDID		0x04	//��ȡLCD��������ID��8λ���������汾ID�����λΪ1��7λ������������ID��8λ��
#define LCD_RDDST		0x09	//��ȡ��ʾ������״̬����
#define LCD_RDDPM		0x0A	//��ȡ��ʾ������ģʽ
#define LCD_RDDMADCTL	0x0B	//��ȡ��ʾ��MADCTL
#define LCD_RDDCOLMOD	0x0C	//��ȡ��ʾ�����ض���
#define LCD_RDDIM		0x0D	//��ȡ��ʾ��ͼƬģʽ
#define LCD_RDDSM		0x0E	//��ȡ��ʾ�����ź�ģʽ
#define LCD_RDDSDR		0x0F	//��ȡ��ʾ��������Ͻ��

#define LCD_SLPIN		0x10	//������С����ģʽ
#define LCD_SLPOUT 		0x11	//�ر�˯��ģʽ
#define LCD_PTLON		0x12	//��Partialģʽ
#define LCD_NORON		0x13	//�ָ�������ģʽ
#define LCD_INVOFF		0x20	//��ʾ��תģʽ�лָ�
#define LCD_INVON		0x21	//���뷴����ʾģʽ
#define LCD_GAMSET		0x26	//��ǰ��ʾѡ�������٤������
#define LCD_DISPOFF		0x28	//�ر���ʾ��֡�ڴ�����������
#define LCD_DISPON		0x29	//������ʾ��֡�ڴ�����������
#define LCD_CASET		0x2A	//�е�ַ���ã�ÿ��ֵ����֡�ڴ��е�һ��
#define LCD_RASET		0x2B	//�е�ַ���ã�ÿ��ֵ����֡�ڴ��е�һ��
#define LCD_RAMWR		0x2C	//д���ڴ�
#define LCD_RGBSET		0x2D	//��ɫģʽ����
#define LCD_RAMRD		0x2E	//��ȡ�ڴ�
#define LCD_PTLAR		0x30	//����ģʽ����ʾ��������
#define LCD_SCRLAR		0x33	//���崹ֱ�����������ʾ
#define LCD_TEOFF		0x34	//�ر�(Active Low) TE�ź��ߵ�˺��ЧӦ����ź�
#define LCD_TEON		0x35	//��TE�ź��ߵ�˺��Ч������ź�
#define LCD_MADCTL		0x36	//����֡�ڴ�Ķ�дɨ�跽��
#define LCD_MADCTL_MX  0x40
#define LCD_MADCTL_MY  0x80
#define LCD_MADCTL_MV  0x20
#define LCD_MADCTL_ML  0x10
#define LCD_MADCTL_RGB 0x08
#define LCD_MADCTL_MH  0x04
#define LCD_VSCSAD		0x37	//���ô�ֱ������ʼ��ַ���������봹ֱ��������(33h)һ��ʹ��
#define LCD_IDMOFF		0x38	//�رտ���ģʽ
#define LCD_IDMON			0x39	//��������ģʽ
#define LCD_COLMOD		0x3A	//����ͨ��MCU�ӿڴ����RGBͼƬ���ݵĸ�ʽ
#define LCD_FRMCTR1		0xB1	//����ȫɫ����ģʽ��֡Ƶ
#define LCD_FRMCTR2 	0xB2	//���ÿ���ģʽ��֡Ƶ
#define LCD_FRMCTR3 	0xB3	//���ò���ģʽ/ȫɫ��֡Ƶ��
#define LCD_INVCTR 		0xB4	//��תģʽ����
#define LCD_PWCTR1 		0xC0	//����AVDD��MODE��VRHP��VRHN
#define LCD_PWCTR2 		0xC1	//����VGH��VGL�Ĺ��繦��
#define LCD_PWCTR3 		0xC2	//��������ģʽ/ȫɫģʽ�µ��˷ŵĵ���
#define LCD_PWCTR4 		0xC3	//���ÿ���ģʽ/��ɫģʽ�µ��˷ŵĵ���
#define LCD_PWCTR5 		0xC4	//���ò���ģʽ/ȫɫģʽ�µ��˷ŵĵ���
#define LCD_VMCTR1 		0xC5	//����VCOM��ѹ��ƽ�Լ�����˸����
#define LCD_VMOFCTR		0xC7	//VCOMƫ�ƿ��ƣ���ʹ������0xC7֮ǰ������0xD9��λVMF_EN��������(����Ϊ1)
#define LCD_WRID2			0xD1	//д��LCDģ��汾��7λ���ݣ����浽NVM
#define LCD_WRID3			0xD2	//д����Ŀ����ģ���8λ���ݣ����浽NVM
#define LCD_NVFCTR1		0xD9	//NVM״̬����
#define LCD_RDID1			0xDA	//���ֽڷ���8λLCDģ���������ID
#define LCD_RDID2			0xDB	//���ֽڷ���8λLCDģ��/��������汾ID
#define LCD_RDID3			0xDC	//���ֽڷ���8λLCDģ��/����ID
#define LCD_NVFCTR2		0xDE	//NVM������
#define LCD_NVFCTR3		0xDF	//NVMд����
#define LCD_GMCTRP1		0xE0	//Gamma ��+�� Polarity Correction Characteristics Setting
#define LCD_GMCTRN1		0xE1	//Gamma ��+�� Polarity Correction Characteristics Setting
#define LCD_PWCTR6		0xFC	//Set the amount of current in Operational amplifier in Partial mode + Idle mode. 


#define Lcd_Cmd GPIOB->BRR = GPIO_Pin_1
#define Lcd_Data GPIOB->BSRR = GPIO_Pin_1

//��SPI���ߴ���һ��8λ����
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
	LCD_SPI.SPI_BaudRatePrescaler	= SPI_BaudRatePrescaler_2;	// ʹ��Ƶֵ�ﵽ���
	LCD_SPI.SPI_CPHA				= SPI_CPHA_1Edge;			// �Ͻ�����Ч
	LCD_SPI.SPI_CPOL				= SPI_CPOL_High;				// �ߵ�ƽ��Ч
	LCD_SPI.SPI_CRCPolynomial		= 7;						// �� CRC У��
	LCD_SPI.SPI_DataSize			= SPI_DataSize_8b;			// �����ֳ�Ϊ 8 λ
	LCD_SPI.SPI_Direction			= SPI_Direction_1Line_Tx;	// ����Ϊ����
	LCD_SPI.SPI_FirstBit			= SPI_FirstBit_MSB;			// ��λ����
	LCD_SPI.SPI_Mode				= SPI_Mode_Master;			// ����ģʽ
	LCD_SPI.SPI_NSS					= SPI_NSS_Soft;				// �������
	SPI_Init( SPI1, &LCD_SPI );
	SPI_Cmd( SPI1, ENABLE );

	delay_ms(1);
	//B0���øߵ�ƽ(Init��ʱ�����)
	GPIOB->BSRR = GPIO_Pin_10;
	delay_ms(50);
	
	//����оƬ��ʼ��
	//��ϸ��Ϣ�鿴Datasheet: 10.2 Panel Function Command List and Description
	Lcd_Cmd;
	Lcd_Write(LCD_SLPOUT);
	// ��ɫ
	Lcd_Write(LCD_INVON);

	// RGB 565 16bit/pixel
	Lcd_Cmd_Write(LCD_COLMOD);
	Lcd_Write(0x05);

	// ٤������
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

	Lcd_SetRotation(0);//����ͼ����ת�Ƕ�
	
	Lcd_Cmd;
	Lcd_Write(LCD_DISPON);
}

//u16 GraphicMemory[X_MAX_PIXEL * Y_MAX_PIXEL];

/*************************************************
������Lcd_Clear
���ܣ�����
�����������ɫ
���أ���
*************************************************/
void Lcd_Clear(u16 Color) {
   Lcd_SetRect(0,0,X_MAX_PIXEL,Y_MAX_PIXEL);
   for(u16 i=0;i<X_MAX_PIXEL*Y_MAX_PIXEL;i++) {
	  	Lcd_Draw(Color);
	 }
}

/*************************************************
������Lcd_SetPoint
���ܣ�����lcd��ʾ��ʼ��
������xy����
���أ���
*************************************************/
void Lcd_SetPoint(u8 x,u8 y) {Lcd_SetRect(x, y, x+1, y+1);}
/*************************************************
������Lcd_SetRect
���ܣ�����lcd��ʾ�����ڴ�����д�����Զ�����
������xy�����յ�
���أ���
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
������Lcd_Draw
���ܣ���Һ����дһ��16λ����
������data
���أ���
*************************************************/
void Lcd_Draw(u16 data) {
	 Lcd_Write(data>>8);
	 Lcd_Write(data);
}
/*************************************************
������Lcd_DrawPoint
���ܣ���һ����
������λ�ú���ɫ
���أ���
*************************************************/
void Lcd_DrawPoint(u8 x, u8 y,u16 color) {
	Lcd_SetPoint(x, y);
	Lcd_Draw(color);
}
/*************************************************
������Lcd_DrawImage
���ܣ���һ��ͼ��
������λ�ã�ͼ��
���أ���
*************************************************/
void Lcd_DrawImage(u8 x, u8 y, struct Image image) {
	Lcd_SetRect(x, y, x + image.width, y + image.height);
	const u16 *colors = image.colors;
	for(u16 count = image.width * image.height; count > 0; count--) {
	  Lcd_Draw(* colors++);
	}
}
