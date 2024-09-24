#include <stm32f10x.h>
#include "lcd.h"
#include "ui.h"
#include "util.h"
#include "input.h"
///////////////////////////////////////////////////////////////////////////////
//  ��������   : 2024/09/16
//  ��������   : ����յ����Ƴ��� - ���ڻ�
/******************************************************************************
//�ӿڷ��� Һ���� (SPI1 TIM3)
//              CS    PA4
//              SCL   PA5
//              SDA   PA7
//              RES   PB10
//              DC    PB1
//              BLK   PB0     // PWM���� TIM3_CH3
//							K1..4 PB6..9
///////////////////////////////////////////////////////////////////////////////
//�ӿڷ��� �¶ȴ����� (ADC1 DMA1)
//              T1    PA1     // ����
//              T2    PA2     // �����¶�
//              T3    PA3     // ˮ���¶�
///////////////////////////////////////////////////////////////////////////////
//�ӿڷ��� �̵���
//              HI    PA8     // �߷�
//              MED   PA9     // �з�
//              LOW   PA10    // �ͷ�
//              OPEN  PA11    // ����
//              CLOSE PA12    // ����
///////////////////////////////////////////////////////////////////////////////
//�ӿڷ��� PWM���
//              FG    PA8     // TIM1_CH1
//              PWM   PA9     // TIM1_CH2
///////////////////////////////////////////////////////////////////////////////
//��ʱ������
//       TIM2 10Hz
//              ��������
//              ��������
//       TIM3 2000Hz (PWM) @ 100 levels
//              PWM���       TIM3_CH1
//              LCD����       TIM3_CH3
//       TIM4 50Hz
//              50x��Ƶ��
//                �༭��ʱ��
//                ˯��ģʽ��ʱ��
//                ��ʱ�ػ���ʱ��
//                �����ʱ��
//              �¶ȵ���ѭ��
*******************************************************************************/

#include "render.h"
#include "pid.h"
#include "config.h"

//declared in config.h
const u16 IDX_TO_PWM_MAP[4] = {17,35,89,179};

extern u16 delay_us_val;
static void Init_IO(void);
static void Init_ADC(void);
static void Init_Timer(void);
static void Init_PWM(void);
static void Init_EXTI(void);

int main() {
	//IWDG_Config(IWDG_Prescaler_64, 625);
	//��ʼ���ӳ�
	//SYSTICK��ʱ�ӹ̶�ΪHCLKʱ�ӵ�1/8
	SysTick->CTRL &= 0xfffffffb;//bit2���,ѡ���ⲿʱ��  HCLK/8
	delay_us_val = SystemCoreClock / 8000000;
  // �������õ���ʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_SPI1 | RCC_APB2Periph_ADC1 | RCC_APB2Periph_AFIO | RCC_APB2Periph_TIM1, ENABLE);
	Init_IO();
	Lcd_Init();
	Init_ADC();
	Init_EXTI();
	Init_Timer();

	Lcd_Clear(WHITE);
	Gui_BgColor = WHITE;
	Gui_Text_XCenter(2,BLACK,"PID�յ� v1.1");
	Gui_Text_XCenter(22,BLACK,"����: Roj234");
	LoadUserConfig();
	if (Config.windPwm&PCFG_WIND_PWM) Init_PWM();

	for(u8 l=1;l<Config.bgLightActive;l++) {
		Set_Backlight(l);
		delay_ms(15);
	}

	uiState = Config.initFlag != 0xFFFF ? MAIN : FACTORY;
	
	Gui_BgColor = BLACK;
  while(1) {
		Lcd_Clear(BLACK);
		ButtonState = ButtonStateEver = 0;
		UIChanged = 19;

		switch(uiState) {
			case CLOSED:
				Gui_Text_XCenter(22,GRAY0,"��ɷ�ù");

				Status.mode |= MODE_EDIT;
			  Status.timer_generic = MAIN_LOOP_HZ * 90;
				TIM_Cmd(TIM4,ENABLE);
				do {
					Gui_Num_4Time(60, 42, GRAY0, Status.timer_generic / MAIN_LOOP_HZ);
				} while (Status.timer_generic);
				TIM_Cmd(TIM4,DISABLE);

				if (!ButtonStateEver) {
					//�ص����е����
					IO_Reset();

					for(u8 l=Config.bgLightActive;l>0;l--) {
						Set_Backlight(l);
						delay_ms(15);
					}

					//����˯��ģʽ
					Set_Backlight(0);
					Lcd_Clear(WHITE);

					PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);

					Set_Backlight(Config.bgLightActive);
					//�ɵ���ʱ��
					TIM2->CR1 &= (uint16_t)(~((uint16_t)TIM_CR1_CEN));
					//��ʼ��ʱ��Դ
					SystemInit();
				} else {
					ButtonState = 0;
				}

				Lcd_Clear(BLACK);
			case MAIN:
				IO_Reset();
				TIM_Cmd(TIM4,ENABLE);
				uiState = MainUI();
				TIM_Cmd(TIM4,DISABLE);
			break;
			case SETTING: uiState = SettingUI(); break;
			case FACTORY: uiState = FactoryUI(); break;
			case FACTORY_ADC:
				FactoryUI_ADC();
				uiState++;
			break;
			case FACTORY_PID:
				FactoryUI_PID();
				uiState++;
			break;
			case FACTORY_Wind:
				FactoryUI_Wind();
				uiState++;
			break;
			case FACTORY_Valve:
				FactoryUI_Valve();
				SaveUserConfig();
				NVIC_SystemReset();
			break;
		}
  }
}

void HardFault_Handler() {Fatal_Error(500);}

/*-------------------------------------------------------------------------------
�������ƣ�Init_IO
������������ʼ��GPIO�˿�
---------------------------------------------------------------------------------*/
__STATIC_INLINE void Init_IO() {
  GPIO_InitTypeDef gpio;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
	//������� �̵���(A8-A12) | ������ò��12V�ģ����ỵ�ɣ�
  gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12;
  GPIO_Init(GPIOA, &gpio);

	//ģ������ �¶ȴ�����(A1-A3)
  gpio.GPIO_Mode = GPIO_Mode_AIN;
	gpio.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3;
  GPIO_Init(GPIOA, &gpio);
	
	//�������� ��������(B6-B9)
  gpio.GPIO_Mode = GPIO_Mode_IPU;
	gpio.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9;
  GPIO_Init(GPIOB, &gpio);
	
	// ����������� LCD BackLight/B0 (TIM3_CH3)
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOB, &gpio);
	
	// ������� DataCommand/B1 Reset/B10
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_10;
	GPIO_Init(GPIOB, &gpio);

	// ChipSelect/A4 = Low
	gpio.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOA, &gpio);
	// MOSI/A5 MISO/A6 CLOCK/A7 ; A6����ΪPWM���
	gpio.GPIO_Pin = GPIO_Pin_5/*|GPIO_Pin_6*/|GPIO_Pin_7;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio);
}
/*-------------------------------------------------------------------------------
�������ƣ�Init_ADC
������������ʼ���¶ȴ�������ADC��DMA��AdcVal
---------------------------------------------------------------------------------*/
__STATIC_INLINE void Init_ADC() {
  // ��DMAʱ��
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	
  DMA_InitTypeDef dma;
  /* ------------------DMA����---------------- */
  //dma.DMA_DIR = DMA_DIR_PeripheralSRC;
  // �洢�����洢�� ����
  //dma.DMA_M2M = DMA_M2M_Disable;
	
  // �����ַΪ��ADC ���ݼĴ�����ַ
  dma.DMA_PeripheralBaseAddr = (uint32_t)(& ADC1->DR );
	// ���ݴ�С�� Word (16bit) * 3
  dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  // �洢����ַ
  dma.DMA_MemoryBaseAddr = (uint32_t)AdcVal;
  dma.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
  // ѭ������
  dma.DMA_Mode = DMA_Mode_Circular;
  dma.DMA_BufferSize = ADC_COUNT;
		
  dma.DMA_Priority = DMA_Priority_Medium;
		
  DMA_Init(DMA1_Channel1, &dma);
  DMA_Cmd(DMA1_Channel1, ENABLE);
	
	ADC_InitTypeDef adc;
  /* ----------------ADC1 ģʽ����--------------------- */
  adc.ADC_Mode = ADC_Mode_Independent;
  adc.ADC_ScanConvMode = ENABLE;
  adc.ADC_ContinuousConvMode = ENABLE;
  adc.ADC_DataAlign = ADC_DataAlign_Right;
  // �����ⲿ����ת��
  adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  adc.ADC_NbrOfChannel = ADC_COUNT;
  ADC_Init(ADC1, &adc);
	
  // ����ADCͨ����ת��˳��Ͳ���ʱ��
  RCC_ADCCLKConfig(RCC_PCLK2_Div8);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_239Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 2, ADC_SampleTime_239Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 3, ADC_SampleTime_239Cycles5);
	
	/* ----------------ADC1 ������У׼--------------------- */
  ADC_DMACmd(ADC1, ENABLE);
  ADC_Cmd(ADC1, ENABLE);
	
  ADC_ResetCalibration(ADC1);
  while (ADC_GetResetCalibrationStatus(ADC1));
  ADC_StartCalibration(ADC1);
  while (ADC_GetCalibrationStatus(ADC1));
	
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

/*-------------------------------------------------------------------------------
�������ƣ�Init_Timer
������������ʼ�����ڷ��Ӽ�����TIM4(50Hz)����Ļ�����TIM3(2kHz)
---------------------------------------------------------------------------------*/
__STATIC_INLINE void Init_Timer() {
	//��ʼ����ʱ��
	//TIM_InternalClockConfig(TIM3);//ѡ��ʱ��Դ(���ڲ�ʱ�ӻ����ⲿʱ��)

	//PWMƵ�ʼ��㹫ʽ
	//72MHz / ((89+1) * (399+1)) = 2KHz
	// 10KHz 5KHz 2KHz 1KHz �ĵ����䲻ͬ���
  TIM3->PSC = 89; //Prescalar
	//PeriodΪPWMռ�ձȿɵ���λ
  TIM3->ARR = 399; //Period

	//��ʼ���Ƚ���
	TIM_OCInitTypeDef outCap;
	outCap.TIM_OCMode = TIM_OCMode_PWM1;
	outCap.TIM_OCPolarity = TIM_OCPolarity_High;
	outCap.TIM_OutputState = TIM_OutputState_Enable;
	//����3ͨ��
	TIM_OC3Init(TIM3,&outCap);
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);

	//������ʱ��
	TIM_Cmd(TIM3,ENABLE);

	//50Hz
  TIM4->PSC = 71;
  TIM4->ARR = 19999;
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);//������ʱ���ж�
	TIM_ClearITPendingBit(TIM4,TIM_IT_Update);//����жϱ�־λ

	NVIC_InitTypeDef nvic;
	nvic.NVIC_IRQChannel=TIM4_IRQn;
	nvic.NVIC_IRQChannelCmd=ENABLE;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvic);

	//TIM_Cmd(TIM4,ENABLE);
}

/*-------------------------------------------------------------------------------
�������ƣ�Init_EXTI
������������ʼ�������ж�
---------------------------------------------------------------------------------*/
__STATIC_INLINE void Init_EXTI() {
	NVIC_InitTypeDef nvic;

	//��ʼ����ʱ��(100Hz / 10ms)���ڰ�������
	//TIM_InternalClockConfig(TIM2);
	TIM_TimeBaseInitTypeDef timer;
	//timer.TIM_CounterMode = TIM_CounterMode_Up;
	timer.TIM_Prescaler = 71;
	timer.TIM_Period = 9999;
	TIM_TimeBaseInit(TIM2,&timer);
	//���ö�ʱ���ж�
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
	nvic.NVIC_IRQChannel = TIM2_IRQn;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	nvic.NVIC_IRQChannelPreemptionPriority = 2;
	nvic.NVIC_IRQChannelSubPriority = 2;
	NVIC_Init(&nvic);
  //ʹ�ܶ�ʱ��
	//TIM_Cmd(TIM2,ENABLE);
	
  /* ���ð����ж� */
  //NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  nvic.NVIC_IRQChannel = EXTI9_5_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority = 1;
  nvic.NVIC_IRQChannelSubPriority = 1;
  nvic.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvic);

  /* �����ⲿ�жϿ����� */
	EXTI_InitTypeDef exti;
  exti.EXTI_Mode = EXTI_Mode_Interrupt;
  exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;// �������½��ض������Է�ֹ����
  exti.EXTI_LineCmd = ENABLE;

  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource6);
  exti.EXTI_Line = EXTI_Line6;
  EXTI_Init(&exti);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource7);
  exti.EXTI_Line = EXTI_Line7;
  EXTI_Init(&exti);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource8);
  exti.EXTI_Line = EXTI_Line8;
  EXTI_Init(&exti);
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource9);
  exti.EXTI_Line = EXTI_Line9;
  EXTI_Init(&exti);
}

/*-------------------------------------------------------------------------------
�������ƣ�Init_PWM
������������ʼ��PWM��ʱ��
---------------------------------------------------------------------------------*/
__STATIC_INLINE void Init_PWM() {
	GPIO_InitTypeDef gpio;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;  //��������
	gpio.GPIO_Pin = GPIO_Pin_10;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

  TIM1->PSC = IDX_TO_PWM_MAP[Config.windPwm&PCFG_WIND_HZ_MASK]; //Prescalar
  TIM1->ARR = 399; //Period

	//��ʼ���Ƚ���
	TIM_OCInitTypeDef outCap;
	outCap.TIM_OCMode = TIM_OCMode_PWM2;
	outCap.TIM_OCPolarity = TIM_OCPolarity_High;
	outCap.TIM_OutputState = TIM_OutputState_Enable;
	//����1��3ͨ��
	//TIM_OC1Init(TIM3,&outCap);
	//TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC3Init(TIM1,&outCap);
	TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);

	//���벶�� ��ռ��1��2ͨ������Ҫ�����˿�
	/*TIM_ICInitTypeDef inputCap;
	inputCap.TIM_Channel = TIM_Channel_1;
	inputCap.TIM_ICPolarity = TIM_ICPolarity_Rising; 
	inputCap.TIM_ICSelection = TIM_ICSelection_DirectTI; //ӳ�䵽 TI1 ��
	inputCap.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	inputCap.TIM_ICFilter = 0x00;
	TIM_PWMIConfig(TIM3,&inputCap);
	TIM_SelectInputTrigger(TIM3, TIM_TS_TI1FP1);  //ѡ��TIM3���봥��Դ  TI1
	TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Reset);  //ѡ��ӻ�ģʽ����λģʽ
	TIM_SelectMasterSlaveMode(TIM3, TIM_MasterSlaveMode_Enable);  //������λģʽ*/

	//������ʱ��
	TIM_Cmd(TIM1,ENABLE);
	TIM_CtrlPWMOutputs(TIM1,ENABLE);
}
