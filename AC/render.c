/* Includes ------------------------------------------------------------------*/
#include <stm32f10x.h>
#include "lcd.h"
#include "ui.h"
#include "util.h"
#include "input.h"
#include "render.h"
#include "config.h"

enum UIState uiState;
u8 btn_no = 0;
u8 UIChanged = 19;
//1: repaint components, 2: repaint button, 4: edit, 8: click
//16: replaint all

#define BTN_COUNT 4
u8 LPCounter = 0;
void SharedButtonHandler(u8 fps) {
	if (UIChanged&4 && ++LPCounter == fps) {
		LPCounter = 0;
		ButtonState |= ButtonLongPress&3;
	}
}

enum UIState MainUI() {
	while(1) {
		SharedButtonHandler(5);
		if(ButtonState&1) {
			if (UIChanged&4) {
				if (btn_no == 0) {
					if (Status.temp < MAX_TEMP - MIN_TEMP)
						Status.temp++;
				} else {
					if (Status.timer_stop < 24*60)
						Status.timer_stop++;
				}
			} else {
				if(btn_no == 0) btn_no = BTN_COUNT;
				else btn_no--;
				UIChanged |= 2;
			}
		}
		if(ButtonState&2) {
			if (UIChanged&4) {
				if (btn_no == 0) {
					if (Status.temp > 0)
						Status.temp--;
				} else {
					if (Status.timer_stop > 0)
						Status.timer_stop--;
				}
			} else {
				if(btn_no == BTN_COUNT) btn_no = 0;
				else btn_no++;
				UIChanged |= 2;
			}
		}
		if(ButtonState&4) UIChanged |= 8;
		if(ButtonState&8) return CLOSED;
		ButtonState = 0;

		if (UIChanged&2) {
			if(UIChanged&16) {
				Gui_Text_XCenter(2,GRAY0,"PID空调");
				Gui_Text(2,22,GRAY0,"温度:");
				Gui_Text(42+40+5,22,GRAY0,"风速");

				Gui_Text(2,42,GRAY0,"室温:");
				Gui_Text(87,42,GRAY0,"出风");
				Gui_Text(160-32-1,62,GRAY1,"设置");
			}
			if (UIChanged&1) {
				u16 color;
				switch (Status.mode&3) {
					case MODE_COLD: color = BLUE; break;
					case MODE_HOT: color = RED; break;
					default: color = BLACK; break;
				}

				Gui_Num_3Temp(2+48, 22, color, Status.temp + MIN_TEMP);

				const u16 colors[] = {RED,YELLOW,GREEN,GRAY1};
				color = colors[Status._wind_set];

				char *text[] = {" 低 ", " 中 ", " 高 ","自动"};
				Gui_Text(90+32,22,color,text[Status.wind_mode]);

				Gui_Text(2,62,Status.mode & MODE_SLEEP ? GREEN : GRAY1, "睡眠");
				if (!Status.timer_stop) Gui_Text(64-8,62,GRAY1, " 定时 ");
				else Gui_Num_4Time(60,62,GREEN,Status.timer_stop);
			}

			// 温度 风速 睡眠 定时 设置
			Gui_Box(42+5,    21, 82+2,     22+16+1, btn_no==0 ? GRAY0 : BLACK);
			Gui_Box(90+32-2, 20, 122+32+2, 22+16+1, btn_no==1 ? GRAY0 : BLACK);
			Gui_Box(1,       61, 1+32,     62+16,   btn_no==2 ? GRAY0 : BLACK);
			Gui_Box(59,      61, 100,      62+16,   btn_no==3 ? GRAY0 : BLACK);
			Gui_Box(160-32,  61, 159,      62+16,   btn_no==4 ? GRAY0 : BLACK);

			UIChanged &= 12;
		}

		if (UIChanged&8) {
			switch (btn_no) {
				default:
					Status.mode ^= MODE_EDIT;
					UIChanged = UIChanged&4 ? 3 : 4;
				goto end_of_repaint;
				case 1: // 风速
					if (++Status.wind_mode == 4) Status.wind_mode = 0;
					Status._wind_set = Status.wind_mode;
				break;
				case 2: // 睡眠
					Status.mode ^= MODE_SLEEP;
					Status.timer_sleep = 0;
				break;
				case 4: // 设置
					UIChanged = 19;
					btn_no = 0;
					return SETTING;
			}
			UIChanged = 3;
		}

		end_of_repaint:
		if (UIChanged&4) {
			if (btn_no == 0) {
				//温度
				Gui_Num_3Temp(2+48, 22, Status.timer_edit&32 ? GRAY1 : WHITE, Status.temp + 160);
			} else {
				//定时
				Gui_Num_4Time(60, 62, Status.timer_edit&32 ? GRAY1 : WHITE, Status.timer_stop);
			}
		}

		Gui_Num_3Temp(2+48, 42, GRAY0, AdcTmp[0]);
		Gui_Num_3Temp(126, 42, GRAY0, AdcTmp[1]);

		if (Status.mode&MODE_DEBUG) {
			//调试数据
			Gui_Num(2,2,GRAY1,Config.windPwm&PCFG_WIND_PWM ? TIM1->CCR3>>2 : Status._wind_set,3);
			Gui_Num(32,2,GRAY1,(GPIOA->ODR >> 8) & 31,2);

			Gui_Char(134,2,Status._valve_set ? GREEN : GRAY1,Status._valve_ui);
			Gui_Num(142, 2, !Status._pid_timer ? GREEN : GRAY1, Status._valve_set, 2);
		}
	}
}

enum UIState SettingUI() {
	u8 sleepSettingChanged = 0;
	while(1) {
		SharedButtonHandler(5);
		if(ButtonState&1) {
			if (UIChanged&4) {
				if (btn_no == 2) {
					if (Config.bgSleepTime < 999 * MAIN_LOOP_HZ)
						Config.bgSleepTime += MAIN_LOOP_HZ;
				} else {
					if (Config.bgLightActive < 100)
						Config.bgLightActive++;
				}
				sleepSettingChanged = 1;
			} else {
				if(btn_no == 0) btn_no = 5;
				else btn_no--;
				UIChanged |= 2;
			}
		}
		if(ButtonState&2) {
			if (UIChanged&4) {
				if (btn_no == 2) {
					if (Config.bgSleepTime > 0)
						Config.bgSleepTime -= MAIN_LOOP_HZ;
				} else {
					if (Config.bgLightActive > 1)
						Config.bgLightActive--;
				}
				sleepSettingChanged = 1;
			} else {
				if(btn_no == 5) btn_no = 0;
				else btn_no++;
				UIChanged |= 2;
			}
		}
		if(ButtonState&4) UIChanged |= 8;
		if(ButtonState&8) {
			if (sleepSettingChanged)
				SaveUserConfig();
			return MAIN;
		}

		ButtonState = 0;

		if (UIChanged&2) {
			if(UIChanged&16) {
				Gui_Text_XCenter(2,GRAY0,"设置");
				Gui_Text(72,22,GRAY0,"睡眠:    s");
				Gui_Text(2,42,GRAY0,"亮度:    %");
				Gui_Text(24,62,GRAY1,"调试    初始化");
			}
			if(UIChanged&1) {
				const u16 colors[] = {BLUE,RED};
				u16 color = colors[Status.mode&1];

				char *text[] = {"制冷","制热","自动","自动"};
				Gui_Text(2,22,color,text[Status.mode&3]);

				Gui_Text(40,22,Status.mode&MODE_PID ? GREEN : GRAY1, "PID");
				Gui_Num(120,22,GRAY0,Config.bgSleepTime / MAIN_LOOP_HZ, 3);
				Gui_Num(50,42,GRAY0,Config.bgLightActive, 3);

				Gui_Text(24,62,Status.mode&MODE_DEBUG ? GREEN : GRAY1, "调试");
			}

			// 模式 PID 睡眠时间 背光亮度 调试 初始化
			Gui_Box(1,   21, 34,    22+16+1, btn_no==0 ? GRAY0 : BLACK);
			Gui_Box(38,  21, 64,    22+16+1, btn_no==1 ? GRAY0 : BLACK);
			Gui_Box(118, 21, 159,   22+16+1, btn_no==2 ? GRAY0 : BLACK);
			Gui_Box(48,  41, 84,    42+16+1, btn_no==3 ? GRAY0 : BLACK);
			Gui_Box(23,  61, 23+34, 62+16,   btn_no==4 ? GRAY0 : BLACK);
			Gui_Box(86,  61, 86+50, 62+16,   btn_no==5 ? GRAY0 : BLACK);

			UIChanged &= 12;
		}

		if (UIChanged&8) {
			switch (btn_no) {
				default: // 2&3
					Status.mode ^= MODE_EDIT;
					UIChanged = UIChanged&4 ? 3 : 4;
				goto end_of_repaint;
				case 0:{ // 模式
					u8 nextMode = (Status.mode + 1) & 3;
					if (nextMode == 3) nextMode = 0;
					Status.mode = (Status.mode & ~3) | nextMode;
				}
				break;
				case 1: // PID
					Status.mode ^= MODE_PID;
				break;
				case 4:
					Status.mode ^= MODE_DEBUG;
					Status._valve_ui = '_';
				break;
				case 5: // 设置
					UIChanged = 19;
					btn_no = 0;
					return FACTORY;
			}
			UIChanged = 3;
		}

		end_of_repaint:
		if (UIChanged&4) {
			if (btn_no == 2) {
				//温度
				Gui_Num(120,22,Status.timer_edit&32 ? GRAY1 : WHITE,Config.bgSleepTime / MAIN_LOOP_HZ, 3);
			} else {
				//定时
				Gui_Num(50,42,Status.timer_edit&32 ? GRAY1 : WHITE,Config.bgLightActive, 3);
				Set_Backlight(Config.bgLightActive);
			}
		}

		delay_ms(1);
	}
}


enum UIState FactoryUI() {
		Gui_Text_XCenter(2,GRAY0,"初始化");
		Gui_Text_XCenter(22,GRAY0,"S1/S2 调节");
		Gui_Text_XCenter(42,GRAY0,"S3切换 S4确定");
		Gui_Text_XCenter(62,GRAY0,"断电取消");

		delay_ms(1500);
		return FACTORY_ADC;
}

//ADC校准
void FactoryUI_ADC() {
	while(1) {
		SharedButtonHandler(5);
		if(ButtonState&1) {
			Config.adc_cal[btn_no]++;
			UIChanged |= 2;
		}
		if(ButtonState&2) {
			Config.adc_cal[btn_no]--;
			UIChanged |= 2;
		}
		if(ButtonState&4) {
				if(btn_no == 2) btn_no = 0;
				else btn_no++;
				UIChanged |= 2;
		}
		if(ButtonState&8) return;
		ButtonState = 0;

		if (UIChanged&2) {
			if(UIChanged&16) {
				Gui_Text_XCenter(2,GRAY0,"ADC校准");
				Gui_Text(50, 22,GRAY0,"RT   OT   WT");
				Gui_Text(2,42,GRAY0,"CAL:");
			}

			Gui_Num(34, 42, btn_no==0 ? GREEN : GRAY1, Config.adc_cal[0], 4);
			Gui_Num(72, 42, btn_no==1 ? GREEN : GRAY1, Config.adc_cal[1], 4);
			Gui_Num(112,42, btn_no==2 ? GREEN : GRAY1, Config.adc_cal[2], 4);

			UIChanged = 4;
		}

		Gui_Num_3Temp(34, 62, GRAY0, AdcToTemp(AdcVal[0]+Config.adc_cal[0]));
		Gui_Num_3Temp(64+8, 62, GRAY0, AdcToTemp(AdcVal[1]+Config.adc_cal[1]));
		Gui_Num_3Temp(104+8, 62, GRAY0, AdcToTemp(AdcVal[2]+Config.adc_cal[2]));
	}
}

//PID参数设置
void FactoryUI_PID() {
	while(1) {
		SharedButtonHandler(1);
		if(ButtonState&1) {
			Config.pid[btn_no]++;
			UIChanged |= 2;
		}
		if(ButtonState&2) {
			Config.pid[btn_no]--;
			UIChanged |= 2;
		}
		if(ButtonState&4) {
				if(btn_no == 2) btn_no = 0;
				else btn_no++;
				UIChanged |= 2;
		}
		if(ButtonState&8) return;
		ButtonState = 0;

		if (UIChanged&2) {
			if(UIChanged&16) Gui_Text_XCenter(2,GRAY0,"PID校准");

			u8 y = 22;
			char *names[] = {"Kp", "Ki", "Kd"};
			for(u8 i = 0; i < 3; i++) {
				Gui_Text(2, y,GRAY0, names[i]);
				Gui_Num_Float(96, y, i == btn_no ? GREEN : GRAY1, 3, Config.pid[i], 6);
				y += 20;
			}

			UIChanged = 4;
		}

		delay_ms(1);
	}
}

//风机参数设置
void FactoryUI_Wind() {
	while(1) {
		if (btn_no > 2) {
			if (!(ButtonState&3)) goto end;
			if (btn_no == 3) {
				Config.windPwm ^= PCFG_WIND_PWM_INTP;
			} else {
				u8 next = (Config.windPwm + 1) & 3;
				TIM3->PSC = IDX_TO_PWM_MAP[next]; //Prescalar
				Config.windPwm = Config.windPwm & ~3 | next; 
			}
			UIChanged |= 2;
		} else {
			SharedButtonHandler(5);
			if(ButtonState&1 && Config.windPwmPct[btn_no] < 100) {
				IO_SetPwmWind(++Config.windPwmPct[btn_no]);
				UIChanged |= 2;
			}
			if(ButtonState&2 && Config.windPwmPct[btn_no] > 0) {
				IO_SetPwmWind(--Config.windPwmPct[btn_no]);
				UIChanged |= 2;
			}
		}

		end:
		if(ButtonState&4) {
				if(btn_no == 4) btn_no = 0;
				else btn_no++;
				UIChanged |= 2;
		}

		if(ButtonState&8) {
			if (Config.windPwmPct[0] | Config.windPwmPct[2]) {
				Config.windPwm |= PCFG_WIND_PWM;
			} else {
				Config.windPwm = 0;
			}
			return;
		}
		ButtonState = 0;

		if (UIChanged&2) {
			if(UIChanged&16) {
				Gui_Text_XCenter(2,GRAY0,"PWM风机 交流跳过");
				Gui_Text_XCenter(22,GRAY0,"Low  Med  High");
				Gui_Text(18,62, GRAY0, "插值:");
				Gui_Text(130,62, GRAY0, "kHz");
			}

			Gui_Num(24, 42, btn_no==0 ? GREEN : GRAY1, Config.windPwmPct[0], 3);
			Gui_Num(64, 42, btn_no==1 ? GREEN : GRAY1, Config.windPwmPct[1], 3);
			Gui_Num(112,42, btn_no==2 ? GREEN : GRAY1, Config.windPwmPct[2], 3);
			Gui_Text(66,62, btn_no==3 ? GREEN : GRAY1, Config.windPwm&PCFG_WIND_PWM_INTP ? "开" : "关");
			const u8 hz[] = {10, 5, 2, 1};
			Gui_Num(112,62, btn_no==4 ? GREEN : GRAY1, hz[Config.windPwm&PCFG_WIND_HZ_MASK], 2);

			UIChanged = 4;
		}
		
		delay_ms(1);
	}
}

//阀门参数设置
void FactoryUI_Valve() {
	TIM_Cmd(TIM4,ENABLE);
	Status.mode |= MODE_EDIT | MODE_VALVE_CAL;

	while(1) {
		if(ButtonState&1) {
			if (!btn_no++) {
				GPIOA->BRR = GPIO_Pin_11 | GPIO_Pin_12;
				delay_ms(200);
				GPIOA->BSRR = GPIO_Pin_11;
				Status.timer_generic = 0;
				UIChanged |= 2;
			} else {
				Config.valveTime[btn_no-2] = Status.timer_generic;
				if (btn_no == 5) {
					btn_no = 0;
					ButtonState |= 2;
				}
			}
		}

		if(ButtonState&2) {
			btn_no = 0;
			UIChanged |= 2;
			GPIOA->BRR = GPIO_Pin_11 | GPIO_Pin_12;
			delay_ms(200);
			GPIOA->BSRR = GPIO_Pin_12;
			UIChanged |= 2;
		}

		if(ButtonState&8) {
			TIM_Cmd(TIM4,DISABLE);
			Status.mode &= ~(MODE_EDIT | MODE_VALVE_CAL);
			return;
		}
		ButtonState = 0;

		if (UIChanged&2) {
			if(UIChanged&16) {
				Gui_Text_XCenter(2,GRAY0,"阀门校准");
				Gui_Text(10, 22,GRAY0,"1%  50%  99%  100%");
				btn_no = 0;
			}

			if (!btn_no) {
				Gui_Text(10, 62,  GRAY1, "S1开阀");
			} else {
				Gui_Text(10, 62,  GREEN, "S1打点");
			}
			Gui_Text(80, 62, !btn_no ? GREEN : GRAY1, "S2关阀");

			UIChanged = 4;
		}

		u8 x = 2;
		for(u8 i = 0; i < 4; i++) {
			Gui_Num(x, 42, GRAY1, btn_no && i >= btn_no-1 ? Status.timer_generic : Config.valveTime[i], 3);
			x += 40;
		}
	}
}
