/*-------------------------------------------------------------------------------
文件名称：input.c
文件描述：输入初始化      
备    注：无
---------------------------------------------------------------------------------*/
#include "input.h"
#include "config.h"
#include "util.h"
#include "ui.h"
#include "render.h" 

#define SAMPLE_AVG_COUNT 50
u16 AdcVal[ADC_COUNT], AdcTmp[ADC_COUNT], AdcAvg[ADC_COUNT] = {2048,2048,2048};
uint32_t PwmVal[2];
//__STATIC_INLINE uint8_t abs(int8_t delta) {return delta < 0 ? -delta : delta;}
__STATIC_INLINE void AdcSample() {
	for(u8 i = 0; i < 3; i++) {
		u16 avg = AdcAvg[i] = ((uint32_t)AdcAvg[i] * (SAMPLE_AVG_COUNT - 1) + AdcVal[i]) / SAMPLE_AVG_COUNT;
		AdcTmp[i] = AdcToTemp(avg) + Config.adc_cal[i];
	}
}

struct STATUS_STRUCT Status = {255-160,0,0,0,0};

static u8 ButtonStatePending;
// 立即按键状态，以及“曾按下”状态
u8 ButtonState, ButtonStateEver, BLPCounter, ButtonLongPress;

/*-------------------------------------------------------------------------------
程序名称：TIM3_IRQHandler
程序描述：风机测速
---------------------------------------------------------------------------------*/
/*
u16 FG_Duty, FG_Speed;
void TIM3_IRQHandler() {
	uint32_t IC1 = TIM_GetCapture1(TIM3);
	uint32_t IC2 = TIM_GetCapture2(TIM3);
	if (IC1) {
		FG_Duty = (IC2+1) * 100 / (IC1+1);
		FG_Speed = 1000 / (IC1+1);
	} else {
		FG_Duty = 0;
		FG_Speed = 0;
	}
	Gui_Num(2,62,0xffff,FG_Duty,3);

	TIM_ClearITPendingBit(TIM3,TIM_IT_CC4);
	TIM_ClearITPendingBit(TIM3,TIM_IT_CC2);
}*/

/*-------------------------------------------------------------------------------
程序名称：TIM2_IRQHandler
程序描述：按键中断消抖
---------------------------------------------------------------------------------*/
void TIM2_IRQHandler() {
	//因为只开启了IT中断，所以不需要检查
	//if(TIM_GetITStatus(TIM2,TIM_IT_Update) != RESET){
	if (!BLPCounter) {
		Status.timer_edit = 0;
		if (Config.bgSleepTime && Status.timer_light == Config.bgSleepTime) {
			Set_Backlight(Config.bgLightActive);
			//忽略这一次按键
			ButtonStatePending = 0;
		}
		Status.timer_light = 0;

		ButtonState |= ButtonStatePending;
		ButtonStateEver |= ButtonStatePending;
	}

	// 750ms
	if (++BLPCounter == 75 || !ButtonStatePending) {
		ButtonLongPress = ButtonStatePending;
		ButtonStatePending = 0;
		TIM2->CR1 &= (uint16_t)(~((uint16_t)TIM_CR1_CEN));
	}
	TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
}
/*-------------------------------------------------------------------------------
程序名称：EXTI9_5_IRQHandler
程序描述：按键中断处理
---------------------------------------------------------------------------------*/
void EXTI9_5_IRQHandler() {
	//理由同上
	//if (EXTI_GetITStatus(EXTI_Line6|EXTI_Line7|EXTI_Line8|EXTI_Line9) != RESET) {

	// 此时，0位是松开的，1位是按下的
	u8 currentState = ((~GPIOB->IDR & (GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9)) >> 6) & 0xF;

	// 首先去除任何未按下的
	ButtonState &= currentState;
	// 然后延迟等待按下
	ButtonStatePending = currentState;
	// 长按检测
	BLPCounter = 0;
	ButtonLongPress = 0;

	TIM2->CNT = 0;
	TIM2->CR1 |= TIM_CR1_CEN;

  EXTI_ClearITPendingBit(EXTI_Line6|EXTI_Line7|EXTI_Line8|EXTI_Line9);
}

static void tickWind(s8 diff);
static void tickValve(s8 diff);
static void tickValvePid(s8 diff);

/*-------------------------------------------------------------------------------
程序名称：TIM4_IRQHandler
程序描述：主中断循环(50Hz / 0.02s)
---------------------------------------------------------------------------------*/
void TIM4_IRQHandler() {
	//略
	AdcSample();

	// 当前正在修改配置
	if (Status.mode & MODE_EDIT) {
		if (Status.mode & MODE_VALVE_CAL) {
			Status.timer_generic++;
			goto clearINT;
		} else if (Status.timer_generic) {
			--Status.timer_generic;
			if (ButtonStateEver) Status.timer_generic = 0;
			goto clearINT;
		}

		// 15s内无操作，退出编辑模式
		if (++Status.timer_edit == 15 * 50) {
			Status.timer_edit = 0;
			ButtonState |= 4;
		}

		goto clearINT;
	}

	if (Status.timer_light < Config.bgSleepTime) {
		if (++Status.timer_light == Config.bgSleepTime) {
			Set_Backlight(2);
		}
	}

	// 每分钟前进一格
	if (++Status.timer_min == 60 * 50) {
		Status.timer_min = 0;
		UIChanged |= 3;

		// 自动关机
		if (Status.timer_stop && -- Status.timer_stop == 0) {
			ButtonState |= 8;
		}

		// 睡眠模式控制的温度升高
		if (Status.mode & MODE_SLEEP && Status.timer_sleep < 120) {
			switch (++Status.timer_sleep) {
				case 30:
				break;
				case 60:
				break;
				case 90:
				break;
				case 120:
				break;
			}
		}
	}

	{
		s8 dt = AdcTmp[0] - (Status.temp+MIN_TEMP);
		if (Status.mode & MODE_HOT) dt = -dt;

		tickWind(dt);

		if (Status.mode&MODE_PID) tickValvePid(dt);
		else tickValve(dt);
	}

	clearINT:
	TIM_ClearITPendingBit(TIM4,TIM_IT_Update);
}

/*-------------------------------------------------------------------------------
程序名称：tickWind
程序描述：风速自动调节&风机平滑切换
---------------------------------------------------------------------------------*/
__STATIC_INLINE void tickWind(s8 diff) {
	// TODO 自动模式改了有的时候颜色不会重新渲染
	if (Status.wind_mode >= 3) {
		// 差值小于0.5度时，低速
		if (diff <= 5) Status._wind_set = 0;
		// 差值超过2.5度时，高速
		else if (diff >= 25) Status._wind_set = 2;
		else {
			// 差值在1.3度到1.8度时，中速
			if ((diff >= 13 && diff <= 18) || Status._wind_set == 3) Status._wind_set = 1;

			// 如果启用PWM插值功能
			if ((Config.windPwm&PCFG_WIND_PWM_INTP)) {
				// 曲线斜率，这里假设是线性的，以后再改
				int32_t delta = (Config.windPwmPct[2] - Config.windPwmPct[0]) / (25 - 5);
				int32_t pwm = Config.windPwmPct[0] + (delta * (diff - 5));
				IO_SetPwmWind(pwm);
				return;
			}
		}
	}

	// 如果风速改变了
	if (Status._wind_now != Status._wind_set) {
		Status._wind_now = Status._wind_set;
		// 告知UI重新渲染
		UIChanged |= 3;

		if (Config.windPwm&PCFG_WIND_PWM) {
			// PWM
			IO_SetPwmWind(Config.windPwmPct[Status._wind_now]);
		} else {
			// 关闭风机输出，1s后再开启，这是为了防止人频繁点击按钮
			Status._wind_timer = 255;
			GPIOA->BRR = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
		}
	}

	// 倒计时
	if (Status._wind_timer && ! --Status._wind_timer) {
		// 开启风机输出
		GPIOA->BSRR = GPIO_Pin_8 << Status._wind_now;
	}
}

/*-------------------------------------------------------------------------------
程序名称：tickValve
程序描述：传统阀门控制
---------------------------------------------------------------------------------*/
void tickValve(s8 diff) {
	if (diff > 18) {
		Status._valve_set = 1;
	} else if (diff < 2) {
		Status._valve_set = 0;
	}

	if (Status._valve_now != Status._valve_set) {
		Status._valve_now = Status._valve_set;

		// 0.1s
		Status._valve_timer = MAIN_LOOP_HZ / 10;
		GPIOA->BRR = GPIO_Pin_11 | GPIO_Pin_12;
	}

	if (Status._valve_timer && ! --Status._valve_timer) {
		// 阀门控制，注意B11是开阀B12是关阀所以这里与1
		GPIOA->BSRR = GPIO_Pin_11 << (Status._valve_set^1);
	}
}

/*-------------------------------------------------------------------------------
程序名称：tickValvePid
程序描述：PID算法阀门控制
---------------------------------------------------------------------------------*/
void tickValvePid(s8 error) {
		// 每两个阀门周期更新一次
		if (++Status._pid_timer < Config.valveTime[3] * 2) {
			if (Status._valve_timer) {
				int32_t sub = Status._pid_timer - Config.valveTime[3];
				if (!sub) {
					//转换为开阀状态
					GPIOA->BRR = GPIO_Pin_12;
					GPIOA->BSRR = GPIO_Pin_11;
					Status._valve_ui = 'O';
				} else {
					if (sub < 0 || --Status._valve_timer) return;
					//阀已就绪
					GPIOA->BRR = GPIO_Pin_11 | GPIO_Pin_12;
					Status._valve_ui = 'R';
				}
			}
			return;
		}
		Status._pid_timer = 0;

		// to real acc 下面没用除法，除法太慢了，虽然乘法也没快到哪里去
		float Kp = Config.pid[0] * 0.001;
		float Ki = Config.pid[1] * 0.001;
		float Kd = Config.pid[2] * 0.001;

    float Pout = Kp * (error - Status._pid_error[0]); // 比例项 Kp * (e(t)-e(t-1))
    float Iout = Ki * error; // 积分项 Ki * e(t)
    float Dout = Kd * (error - 2*Status._pid_error[0] + Status._pid_error[1]); // 微分项 Kd * (e(t)-2*e(t-1)+e(t-2))
    float output = Pout + Iout + Dout + Status._pid_output; // 新的目标值  位置式PID：u(t) = Kp*(e(t)-e(t-1)) + Ki*e(t) + Kd*(e(t)-2*e(t-1)+e(t-2)) + u(t-1

    if (output > 1) output = 1;
    else if (output < 0) output = 0;

		Status._pid_output = output;
		Status._pid_error[1] = Status._pid_error[0]; //e(k-2) = e(k) ,进入下一次计算使用
		Status._pid_error[0] = error; // e(k-1) = e(k)

		//0-100%
		Status._valve_set = output * 10;

		if (Status._valve_now != Status._valve_set) {
			Status._valve_now = Status._valve_set;

			if (Status._valve_set == 10) {
				//全开
				GPIOA->BRR = GPIO_Pin_12;
				GPIOA->BSRR = GPIO_Pin_11;
				Status._valve_ui = 'F';
			} else {
				//半开
				if (Status._valve_set)
					//计算要通电多久
					Status._valve_timer = (Config.valveTime[2] - Config.valveTime[0]) / 10 * Status._valve_set;
				//开始关阀，每次修改先重置，避免开环控制的累积误差
				GPIOA->BRR = GPIO_Pin_11;
				GPIOA->BSRR = GPIO_Pin_12;
				Status._valve_ui = 'C';
			}
		}
}

/*-------------------------------------------------------------------------------
程序名称：IO_Reset
程序描述：关闭所有输出
---------------------------------------------------------------------------------*/
void IO_Reset() {
	Status._wind_timer = 0;
	Status._wind_now = 4;//风机关闭
	Status._valve_now = 0;//阀门关闭
	IO_SetPwmWind(Config.windPwmPct[0]);//PWM风机低速
	GPIOA->BRR = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
	GPIOA->BSRR = GPIO_Pin_12;

	Status._pid_output   = 0;
	Status._pid_error[0] = 0;
	Status._pid_error[1] = 0;
}

/*-------------------------------------------------------------------------------
程序名称：Fatal_Error
程序描述：出现异常时不同频率闪烁板载警告灯(C13)
---------------------------------------------------------------------------------*/
void Fatal_Error(u16 interval) {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  GPIO_InitTypeDef _init;
	_init.GPIO_Pin = GPIO_Pin_13;
  _init.GPIO_Speed = GPIO_Speed_2MHz;
	//推挽输出
  _init.GPIO_Mode = GPIO_Mode_Out_PP;
  GPIO_Init(GPIOC, &_init);

  while(interval) {
		delay_ms(interval);
		GPIOC->BSRR = GPIO_Pin_13;
		delay_ms(interval);
		GPIOC->BRR = GPIO_Pin_13;
	}
}

/*-------------------------------------------------------------------------------
程序名称：IO_SetPwmWind
程序描述：设置PWM风机转速
---------------------------------------------------------------------------------*/
void IO_SetPwmWind(u8 pwm) {
	TIM_SetCompare3(TIM1, (u16)pwm << 2);
}

/*-------------------------------------------------------------------------------
程序名称：Set_Backlight
程序描述：设置背光亮度，400档
---------------------------------------------------------------------------------*/
void Set_Backlight(u8 level) {
	TIM_SetCompare3(TIM3, (u16)level << 2);
}

/*-------------------------------------------------------------------------------
程序名称：AdcToTemp
程序描述：将热敏电阻的Adc值转换为温度
---------------------------------------------------------------------------------*/
int16_t AdcToTemp(u16 temp);
#include "ntc_103_3950_adc.h"
