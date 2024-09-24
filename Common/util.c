#include <stm32f10x.h>
#include "util.h"

u16 delay_us_val;//us��ʱ����

//��ʱnms
//ע��nms�ķ�Χ
//SysTick->LOADΪ24λ�Ĵ���,����,�����ʱΪ:
//nms<=0xffffff*8*1000/SYSCLK
//SYSCLK��λΪHz,nms��λΪms
//��72M������,nms<=1864 
void delay_ms(u16 nms) {
	u32 temp;		   
	SysTick->LOAD=(u32)nms*delay_us_val*1000;//ʱ�����(SysTick->LOADΪ24bit)
	SysTick->VAL =0x00;//��ռ�����
	SysTick->CTRL=0x01;//��ʼ����  
	do {
		temp=SysTick->CTRL;
	} while(temp&0x01&&!(temp&(1<<16)));//�ȴ�ʱ�䵽��   
	SysTick->CTRL=0x00;//�رռ�����
}
