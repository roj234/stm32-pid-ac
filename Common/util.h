#ifndef __UTIL_H
#define __UTIL_H
#include "stm32f10x.h"
void delay_ms(u16 ms);
void delay_100ms(u16 ms);
void delay_100ms_async(u16 ms, void (*callback)(void));
#endif
