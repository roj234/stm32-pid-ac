
#ifndef __INPUT_H
#define __INPUT_H

#include <stm32f10x.h>

#define ADC_COUNT 3

void Fatal_Error(u16 interval);
// 设置PWM风速
void IO_SetPwmWind(u8 pwm);
// 关闭所有输出
void IO_Reset(void);
// (main.c) PWM调光
void Set_Backlight(u8 light);

#define MAIN_LOOP_HZ 50
// 按钮按下状态“寄存器”，S1-S4位于0-3bit
extern u8 ButtonState, ButtonStateEver, ButtonLongPress;

// 将热敏电阻的Adc值转换为温度
int16_t AdcToTemp(u16 temp);
extern u16 AdcVal[], AdcTmp[];
extern uint32_t PwmVal[];

#define MIN_TEMP 160
#define MAX_TEMP 320

#if MAX_TEMP - MIN_TEMP > 255
#error Temp is U8
#endif

#define MODE_COLD  0
#define MODE_HOT   1
#define MODE_AUTO  2
#define MODE_SLEEP 4
#define MODE_EDIT  8
#define MODE_PID   32
#define MODE_VALVE_CAL 64
#define MODE_DEBUG 128

#define WIND_LOW  0
#define WIND_MED  1
#define WIND_HI   2
#define WIND_AUTO 3
struct STATUS_STRUCT {
	u8 temp;
	u8 mode, wind_mode;
	u8 timer_sleep, timer_stop;
	u16 timer_light, timer_min, timer_edit, timer_generic;

	u8 _wind_set, _wind_now, _wind_timer;

	u8 _valve_set, _valve_now, _valve_ui;
	u16 _pid_timer, _valve_timer;
	float _pid_error[2], _pid_output;
};
extern struct STATUS_STRUCT Status;

#endif
