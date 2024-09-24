/*-------------------------------------------------------------------------------
�ļ����ƣ�input.c
�ļ������������ʼ��      
��    ע����
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
// ��������״̬���Լ��������¡�״̬
u8 ButtonState, ButtonStateEver, BLPCounter, ButtonLongPress;

/*-------------------------------------------------------------------------------
�������ƣ�TIM3_IRQHandler
�����������������
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
�������ƣ�TIM2_IRQHandler
���������������ж�����
---------------------------------------------------------------------------------*/
void TIM2_IRQHandler() {
	//��Ϊֻ������IT�жϣ����Բ���Ҫ���
	//if(TIM_GetITStatus(TIM2,TIM_IT_Update) != RESET){
	if (!BLPCounter) {
		Status.timer_edit = 0;
		if (Config.bgSleepTime && Status.timer_light == Config.bgSleepTime) {
			Set_Backlight(Config.bgLightActive);
			//������һ�ΰ���
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
�������ƣ�EXTI9_5_IRQHandler
���������������жϴ���
---------------------------------------------------------------------------------*/
void EXTI9_5_IRQHandler() {
	//����ͬ��
	//if (EXTI_GetITStatus(EXTI_Line6|EXTI_Line7|EXTI_Line8|EXTI_Line9) != RESET) {

	// ��ʱ��0λ���ɿ��ģ�1λ�ǰ��µ�
	u8 currentState = ((~GPIOB->IDR & (GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9)) >> 6) & 0xF;

	// ����ȥ���κ�δ���µ�
	ButtonState &= currentState;
	// Ȼ���ӳٵȴ�����
	ButtonStatePending = currentState;
	// �������
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
�������ƣ�TIM4_IRQHandler
�������������ж�ѭ��(50Hz / 0.02s)
---------------------------------------------------------------------------------*/
void TIM4_IRQHandler() {
	//��
	AdcSample();

	// ��ǰ�����޸�����
	if (Status.mode & MODE_EDIT) {
		if (Status.mode & MODE_VALVE_CAL) {
			Status.timer_generic++;
			goto clearINT;
		} else if (Status.timer_generic) {
			--Status.timer_generic;
			if (ButtonStateEver) Status.timer_generic = 0;
			goto clearINT;
		}

		// 15s���޲������˳��༭ģʽ
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

	// ÿ����ǰ��һ��
	if (++Status.timer_min == 60 * 50) {
		Status.timer_min = 0;
		UIChanged |= 3;

		// �Զ��ػ�
		if (Status.timer_stop && -- Status.timer_stop == 0) {
			ButtonState |= 8;
		}

		// ˯��ģʽ���Ƶ��¶�����
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
�������ƣ�tickWind
���������������Զ�����&���ƽ���л�
---------------------------------------------------------------------------------*/
__STATIC_INLINE void tickWind(s8 diff) {
	// TODO �Զ�ģʽ�����е�ʱ����ɫ����������Ⱦ
	if (Status.wind_mode >= 3) {
		// ��ֵС��0.5��ʱ������
		if (diff <= 5) Status._wind_set = 0;
		// ��ֵ����2.5��ʱ������
		else if (diff >= 25) Status._wind_set = 2;
		else {
			// ��ֵ��1.3�ȵ�1.8��ʱ������
			if ((diff >= 13 && diff <= 18) || Status._wind_set == 3) Status._wind_set = 1;

			// �������PWM��ֵ����
			if ((Config.windPwm&PCFG_WIND_PWM_INTP)) {
				// ����б�ʣ�������������Եģ��Ժ��ٸ�
				int32_t delta = (Config.windPwmPct[2] - Config.windPwmPct[0]) / (25 - 5);
				int32_t pwm = Config.windPwmPct[0] + (delta * (diff - 5));
				IO_SetPwmWind(pwm);
				return;
			}
		}
	}

	// ������ٸı���
	if (Status._wind_now != Status._wind_set) {
		Status._wind_now = Status._wind_set;
		// ��֪UI������Ⱦ
		UIChanged |= 3;

		if (Config.windPwm&PCFG_WIND_PWM) {
			// PWM
			IO_SetPwmWind(Config.windPwmPct[Status._wind_now]);
		} else {
			// �رշ�������1s���ٿ���������Ϊ�˷�ֹ��Ƶ�������ť
			Status._wind_timer = 255;
			GPIOA->BRR = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
		}
	}

	// ����ʱ
	if (Status._wind_timer && ! --Status._wind_timer) {
		// ����������
		GPIOA->BSRR = GPIO_Pin_8 << Status._wind_now;
	}
}

/*-------------------------------------------------------------------------------
�������ƣ�tickValve
������������ͳ���ſ���
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
		// ���ſ��ƣ�ע��B11�ǿ���B12�ǹط�����������1
		GPIOA->BSRR = GPIO_Pin_11 << (Status._valve_set^1);
	}
}

/*-------------------------------------------------------------------------------
�������ƣ�tickValvePid
����������PID�㷨���ſ���
---------------------------------------------------------------------------------*/
void tickValvePid(s8 error) {
		// ÿ�����������ڸ���һ��
		if (++Status._pid_timer < Config.valveTime[3] * 2) {
			if (Status._valve_timer) {
				int32_t sub = Status._pid_timer - Config.valveTime[3];
				if (!sub) {
					//ת��Ϊ����״̬
					GPIOA->BRR = GPIO_Pin_12;
					GPIOA->BSRR = GPIO_Pin_11;
					Status._valve_ui = 'O';
				} else {
					if (sub < 0 || --Status._valve_timer) return;
					//���Ѿ���
					GPIOA->BRR = GPIO_Pin_11 | GPIO_Pin_12;
					Status._valve_ui = 'R';
				}
			}
			return;
		}
		Status._pid_timer = 0;

		// to real acc ����û�ó���������̫���ˣ���Ȼ�˷�Ҳû�쵽����ȥ
		float Kp = Config.pid[0] * 0.001;
		float Ki = Config.pid[1] * 0.001;
		float Kd = Config.pid[2] * 0.001;

    float Pout = Kp * (error - Status._pid_error[0]); // ������ Kp * (e(t)-e(t-1))
    float Iout = Ki * error; // ������ Ki * e(t)
    float Dout = Kd * (error - 2*Status._pid_error[0] + Status._pid_error[1]); // ΢���� Kd * (e(t)-2*e(t-1)+e(t-2))
    float output = Pout + Iout + Dout + Status._pid_output; // �µ�Ŀ��ֵ  λ��ʽPID��u(t) = Kp*(e(t)-e(t-1)) + Ki*e(t) + Kd*(e(t)-2*e(t-1)+e(t-2)) + u(t-1

    if (output > 1) output = 1;
    else if (output < 0) output = 0;

		Status._pid_output = output;
		Status._pid_error[1] = Status._pid_error[0]; //e(k-2) = e(k) ,������һ�μ���ʹ��
		Status._pid_error[0] = error; // e(k-1) = e(k)

		//0-100%
		Status._valve_set = output * 10;

		if (Status._valve_now != Status._valve_set) {
			Status._valve_now = Status._valve_set;

			if (Status._valve_set == 10) {
				//ȫ��
				GPIOA->BRR = GPIO_Pin_12;
				GPIOA->BSRR = GPIO_Pin_11;
				Status._valve_ui = 'F';
			} else {
				//�뿪
				if (Status._valve_set)
					//����Ҫͨ����
					Status._valve_timer = (Config.valveTime[2] - Config.valveTime[0]) / 10 * Status._valve_set;
				//��ʼ�ط���ÿ���޸������ã����⿪�����Ƶ��ۻ����
				GPIOA->BRR = GPIO_Pin_11;
				GPIOA->BSRR = GPIO_Pin_12;
				Status._valve_ui = 'C';
			}
		}
}

/*-------------------------------------------------------------------------------
�������ƣ�IO_Reset
�����������ر��������
---------------------------------------------------------------------------------*/
void IO_Reset() {
	Status._wind_timer = 0;
	Status._wind_now = 4;//����ر�
	Status._valve_now = 0;//���Źر�
	IO_SetPwmWind(Config.windPwmPct[0]);//PWM�������
	GPIOA->BRR = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
	GPIOA->BSRR = GPIO_Pin_12;

	Status._pid_output   = 0;
	Status._pid_error[0] = 0;
	Status._pid_error[1] = 0;
}

/*-------------------------------------------------------------------------------
�������ƣ�Fatal_Error
���������������쳣ʱ��ͬƵ����˸���ؾ����(C13)
---------------------------------------------------------------------------------*/
void Fatal_Error(u16 interval) {
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
  GPIO_InitTypeDef _init;
	_init.GPIO_Pin = GPIO_Pin_13;
  _init.GPIO_Speed = GPIO_Speed_2MHz;
	//�������
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
�������ƣ�IO_SetPwmWind
��������������PWM���ת��
---------------------------------------------------------------------------------*/
void IO_SetPwmWind(u8 pwm) {
	TIM_SetCompare3(TIM1, (u16)pwm << 2);
}

/*-------------------------------------------------------------------------------
�������ƣ�Set_Backlight
�������������ñ������ȣ�400��
---------------------------------------------------------------------------------*/
void Set_Backlight(u8 level) {
	TIM_SetCompare3(TIM3, (u16)level << 2);
}

/*-------------------------------------------------------------------------------
�������ƣ�AdcToTemp
���������������������Adcֵת��Ϊ�¶�
---------------------------------------------------------------------------------*/
int16_t AdcToTemp(u16 temp);
#include "ntc_103_3950_adc.h"
