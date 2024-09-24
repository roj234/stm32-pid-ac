#ifndef __RENDER_H
#define __RENDER_H

enum UIState {
	// 关闭
	CLOSED = 0,
	// 主界面
	MAIN,
	// 模式、PID和背光设定
	SETTING,

	// 初始化设定
	FACTORY,
	FACTORY_ADC,
	FACTORY_PID,
	FACTORY_Wind,
	FACTORY_Valve,
};

extern enum UIState uiState;
extern u8 UIChanged;

enum UIState MainUI(void);
enum UIState SettingUI(void);
enum UIState FactoryUI(void);
void FactoryUI_ADC(void);
void FactoryUI_PID(void);
void FactoryUI_Wind(void);
void FactoryUI_Valve(void);
#endif
