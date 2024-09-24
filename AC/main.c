#include <stm32f10x.h>
#include "lcd.h"
#include "ui.h"
#include "util.h"
#include "input.h"
///////////////////////////////////////////////////////////////////////////////
//  创建日期   : 2024/09/16
//  功能描述   : 中央空调控制程序 - 室内机
/******************************************************************************
//接口分配 液晶屏 (SPI1 TIM3)
//              CS    PA4
//              SCL   PA5
//              SDA   PA7
//              RES   PB10
//              DC    PB1
//              BLK   PB0     // PWM调光 TIM3_CH3
//							K1..4 PB6..9
///////////////////////////////////////////////////////////////////////////////
//接口分配 温度传感器 (ADC1 DMA1)
//              T1    PA1     // 室温
//              T2    PA2     // 出风温度
//              T3    PA3     // 水管温度
///////////////////////////////////////////////////////////////////////////////
//接口分配 继电器
//              HI    PA8     // 高风
//              MED   PA9     // 中风
//              LOW   PA10    // 低风
//              OPEN  PA11    // 阀开
//              CLOSE PA12    // 阀关
///////////////////////////////////////////////////////////////////////////////
//接口分配 PWM风机
//              FG    PA8     // TIM1_CH1
//              PWM   PA9     // TIM1_CH2
///////////////////////////////////////////////////////////////////////////////
//计时器分配
//       TIM2 10Hz
//              按键消抖
//              按键长按
//       TIM3 2000Hz (PWM) @ 100 levels
//              PWM风机       TIM3_CH1
//              LCD背光       TIM3_CH3
//       TIM4 50Hz
//              50x分频器
//                编辑计时器
//                睡眠模式计时器
//                定时关机计时器
//                背光计时器
//              温度调节循环
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
	//初始化延迟
	//SYSTICK的时钟固定为HCLK时钟的1/8
	SysTick->CTRL &= 0xfffffffb;//bit2清空,选择外部时钟  HCLK/8
	delay_us_val = SystemCoreClock / 8000000;
  // 打开所有用到的时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_SPI1 | RCC_APB2Periph_ADC1 | RCC_APB2Periph_AFIO | RCC_APB2Periph_TIM1, ENABLE);
	Init_IO();
	Lcd_Init();
	Init_ADC();
	Init_EXTI();
	Init_Timer();

	Lcd_Clear(WHITE);
	Gui_BgColor = WHITE;
	Gui_Text_XCenter(2,BLACK,"PID空调 v1.1");
	Gui_Text_XCenter(22,BLACK,"作者: Roj234");
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
				Gui_Text_XCenter(22,GRAY0,"烘干防霉");

				Status.mode |= MODE_EDIT;
			  Status.timer_generic = MAIN_LOOP_HZ * 90;
				TIM_Cmd(TIM4,ENABLE);
				do {
					Gui_Num_4Time(60, 42, GRAY0, Status.timer_generic / MAIN_LOOP_HZ);
				} while (Status.timer_generic);
				TIM_Cmd(TIM4,DISABLE);

				if (!ButtonStateEver) {
					//关掉所有的输出
					IO_Reset();

					for(u8 l=Config.bgLightActive;l>0;l--) {
						Set_Backlight(l);
						delay_ms(15);
					}

					//进入睡眠模式
					Set_Backlight(0);
					Lcd_Clear(WHITE);

					PWR_EnterSTOPMode(PWR_Regulator_LowPower, PWR_STOPEntry_WFI);

					Set_Backlight(Config.bgLightActive);
					//干掉定时器
					TIM2->CR1 &= (uint16_t)(~((uint16_t)TIM_CR1_CEN));
					//初始化时钟源
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
程序名称：Init_IO
程序描述：初始化GPIO端口
---------------------------------------------------------------------------------*/
__STATIC_INLINE void Init_IO() {
  GPIO_InitTypeDef gpio;
  gpio.GPIO_Speed = GPIO_Speed_50MHz;
	//推挽输出 继电器(A8-A12) | 这玩意貌似12V的，不会坏吧？
  gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9|GPIO_Pin_10|GPIO_Pin_11|GPIO_Pin_12;
  GPIO_Init(GPIOA, &gpio);

	//模拟输入 温度传感器(A1-A3)
  gpio.GPIO_Mode = GPIO_Mode_AIN;
	gpio.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_2|GPIO_Pin_3;
  GPIO_Init(GPIOA, &gpio);
	
	//上拉输入 独立按键(B6-B9)
  gpio.GPIO_Mode = GPIO_Mode_IPU;
	gpio.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9;
  GPIO_Init(GPIOB, &gpio);
	
	// 复用推挽输出 LCD BackLight/B0 (TIM3_CH3)
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	gpio.GPIO_Pin = GPIO_Pin_0;
	GPIO_Init(GPIOB, &gpio);
	
	// 推挽输出 DataCommand/B1 Reset/B10
	gpio.GPIO_Mode = GPIO_Mode_Out_PP;
	gpio.GPIO_Pin = GPIO_Pin_1|GPIO_Pin_10;
	GPIO_Init(GPIOB, &gpio);

	// ChipSelect/A4 = Low
	gpio.GPIO_Pin = GPIO_Pin_4;
	GPIO_Init(GPIOA, &gpio);
	// MOSI/A5 MISO/A6 CLOCK/A7 ; A6复用为PWM输出
	gpio.GPIO_Pin = GPIO_Pin_5/*|GPIO_Pin_6*/|GPIO_Pin_7;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA, &gpio);
}
/*-------------------------------------------------------------------------------
程序名称：Init_ADC
程序描述：初始化温度传感器的ADC并DMA到AdcVal
---------------------------------------------------------------------------------*/
__STATIC_INLINE void Init_ADC() {
  // 打开DMA时钟
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
	
  DMA_InitTypeDef dma;
  /* ------------------DMA配置---------------- */
  //dma.DMA_DIR = DMA_DIR_PeripheralSRC;
  // 存储器到存储器 禁用
  //dma.DMA_M2M = DMA_M2M_Disable;
	
  // 外设基址为：ADC 数据寄存器地址
  dma.DMA_PeripheralBaseAddr = (uint32_t)(& ADC1->DR );
	// 数据大小： Word (16bit) * 3
  dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
  dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
  // 存储器地址
  dma.DMA_MemoryBaseAddr = (uint32_t)AdcVal;
  dma.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
  dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
  // 循环传输
  dma.DMA_Mode = DMA_Mode_Circular;
  dma.DMA_BufferSize = ADC_COUNT;
		
  dma.DMA_Priority = DMA_Priority_Medium;
		
  DMA_Init(DMA1_Channel1, &dma);
  DMA_Cmd(DMA1_Channel1, ENABLE);
	
	ADC_InitTypeDef adc;
  /* ----------------ADC1 模式配置--------------------- */
  adc.ADC_Mode = ADC_Mode_Independent;
  adc.ADC_ScanConvMode = ENABLE;
  adc.ADC_ContinuousConvMode = ENABLE;
  adc.ADC_DataAlign = ADC_DataAlign_Right;
  // 不用外部触发转换
  adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
  adc.ADC_NbrOfChannel = ADC_COUNT;
  ADC_Init(ADC1, &adc);
	
  // 配置ADC通道的转换顺序和采样时间
  RCC_ADCCLKConfig(RCC_PCLK2_Div8);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 1, ADC_SampleTime_239Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 2, ADC_SampleTime_239Cycles5);
  ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 3, ADC_SampleTime_239Cycles5);
	
	/* ----------------ADC1 启动与校准--------------------- */
  ADC_DMACmd(ADC1, ENABLE);
  ADC_Cmd(ADC1, ENABLE);
	
  ADC_ResetCalibration(ADC1);
  while (ADC_GetResetCalibrationStatus(ADC1));
  ADC_StartCalibration(ADC1);
  while (ADC_GetCalibrationStatus(ADC1));
	
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

/*-------------------------------------------------------------------------------
程序名称：Init_Timer
程序描述：初始化用于分钟计数的TIM4(50Hz)和屏幕背光的TIM3(2kHz)
---------------------------------------------------------------------------------*/
__STATIC_INLINE void Init_Timer() {
	//初始化计时器
	//TIM_InternalClockConfig(TIM3);//选择时钟源(是内部时钟还是外部时钟)

	//PWM频率计算公式
	//72MHz / ((89+1) * (399+1)) = 2KHz
	// 10KHz 5KHz 2KHz 1KHz 四档适配不同风机
  TIM3->PSC = 89; //Prescalar
	//Period为PWM占空比可调挡位
  TIM3->ARR = 399; //Period

	//初始化比较器
	TIM_OCInitTypeDef outCap;
	outCap.TIM_OCMode = TIM_OCMode_PWM1;
	outCap.TIM_OCPolarity = TIM_OCPolarity_High;
	outCap.TIM_OutputState = TIM_OutputState_Enable;
	//开启3通道
	TIM_OC3Init(TIM3,&outCap);
	TIM_OC3PreloadConfig(TIM3, TIM_OCPreload_Enable);

	//启动定时器
	TIM_Cmd(TIM3,ENABLE);

	//50Hz
  TIM4->PSC = 71;
  TIM4->ARR = 19999;
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE);//开启定时器中断
	TIM_ClearITPendingBit(TIM4,TIM_IT_Update);//清除中断标志位

	NVIC_InitTypeDef nvic;
	nvic.NVIC_IRQChannel=TIM4_IRQn;
	nvic.NVIC_IRQChannelCmd=ENABLE;
	nvic.NVIC_IRQChannelPreemptionPriority = 0;
	nvic.NVIC_IRQChannelSubPriority = 0;
	NVIC_Init(&nvic);

	//TIM_Cmd(TIM4,ENABLE);
}

/*-------------------------------------------------------------------------------
程序名称：Init_EXTI
程序描述：初始化按键中断
---------------------------------------------------------------------------------*/
__STATIC_INLINE void Init_EXTI() {
	NVIC_InitTypeDef nvic;

	//初始化计时器(100Hz / 10ms)用于按键消抖
	//TIM_InternalClockConfig(TIM2);
	TIM_TimeBaseInitTypeDef timer;
	//timer.TIM_CounterMode = TIM_CounterMode_Up;
	timer.TIM_Prescaler = 71;
	timer.TIM_Period = 9999;
	TIM_TimeBaseInit(TIM2,&timer);
	//配置定时器中断
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
	nvic.NVIC_IRQChannel = TIM2_IRQn;
	nvic.NVIC_IRQChannelCmd = ENABLE;
	nvic.NVIC_IRQChannelPreemptionPriority = 2;
	nvic.NVIC_IRQChannelSubPriority = 2;
	NVIC_Init(&nvic);
  //使能定时器
	//TIM_Cmd(TIM2,ENABLE);
	
  /* 配置按键中断 */
  //NVIC_PriorityGroupConfig(NVIC_PriorityGroup_1);
  nvic.NVIC_IRQChannel = EXTI9_5_IRQn;
  nvic.NVIC_IRQChannelPreemptionPriority = 1;
  nvic.NVIC_IRQChannelSubPriority = 1;
  nvic.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&nvic);

  /* 配置外部中断控制器 */
	EXTI_InitTypeDef exti;
  exti.EXTI_Mode = EXTI_Mode_Interrupt;
  exti.EXTI_Trigger = EXTI_Trigger_Rising_Falling;// 上升和下降沿都产生以防止抖动
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
程序名称：Init_PWM
程序描述：初始化PWM定时器
---------------------------------------------------------------------------------*/
__STATIC_INLINE void Init_PWM() {
	GPIO_InitTypeDef gpio;
	gpio.GPIO_Mode = GPIO_Mode_AF_PP;  //复用推挽
	gpio.GPIO_Pin = GPIO_Pin_10;
	gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &gpio);

  TIM1->PSC = IDX_TO_PWM_MAP[Config.windPwm&PCFG_WIND_HZ_MASK]; //Prescalar
  TIM1->ARR = 399; //Period

	//初始化比较器
	TIM_OCInitTypeDef outCap;
	outCap.TIM_OCMode = TIM_OCMode_PWM2;
	outCap.TIM_OCPolarity = TIM_OCPolarity_High;
	outCap.TIM_OutputState = TIM_OutputState_Enable;
	//开启1和3通道
	//TIM_OC1Init(TIM3,&outCap);
	//TIM_OC1PreloadConfig(TIM3, TIM_OCPreload_Enable);
	TIM_OC3Init(TIM1,&outCap);
	TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);

	//输入捕获 会占用1、2通道，需要更换端口
	/*TIM_ICInitTypeDef inputCap;
	inputCap.TIM_Channel = TIM_Channel_1;
	inputCap.TIM_ICPolarity = TIM_ICPolarity_Rising; 
	inputCap.TIM_ICSelection = TIM_ICSelection_DirectTI; //映射到 TI1 上
	inputCap.TIM_ICPrescaler = TIM_ICPSC_DIV1;
	inputCap.TIM_ICFilter = 0x00;
	TIM_PWMIConfig(TIM3,&inputCap);
	TIM_SelectInputTrigger(TIM3, TIM_TS_TI1FP1);  //选择TIM3输入触发源  TI1
	TIM_SelectSlaveMode(TIM3, TIM_SlaveMode_Reset);  //选择从机模式，复位模式
	TIM_SelectMasterSlaveMode(TIM3, TIM_MasterSlaveMode_Enable);  //开启复位模式*/

	//启动定时器
	TIM_Cmd(TIM1,ENABLE);
	TIM_CtrlPWMOutputs(TIM1,ENABLE);
}
