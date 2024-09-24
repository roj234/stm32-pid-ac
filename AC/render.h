#ifndef __RENDER_H
#define __RENDER_H

enum UIState {
	// �ر�
	CLOSED = 0,
	// ������
	MAIN,
	// ģʽ��PID�ͱ����趨
	SETTING,

	// ��ʼ���趨
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
