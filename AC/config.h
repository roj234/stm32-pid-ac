
#ifndef __CONFIG_H
#define __CONFIG_H

typedef u8 CFGSTATUS;
#define CFG_OK 0

// VAL =     0    1    2    3
// REP =    17   39   89  179
// HZ  = 10000 5000 2000 1000
#define PCFG_WIND_HZ_MASK  3
#define PCFG_WIND_PWM      4
#define PCFG_WIND_PWM_INTP 8

struct CONFIG_STRUCT {
	u8 bgLightActive;
	u16 bgSleepTime;

	s8 adc_cal[3];
	s16 pid[3];
	u8 windPwm, windPwmPct[3];
	u16 valveTime[4];

	uint32_t checksum;

	u16 initFlag;
	uint32_t reserved;
};

extern struct CONFIG_STRUCT Config;
extern const u16 IDX_TO_PWM_MAP[];

CFGSTATUS LoadUserConfig(void);
CFGSTATUS SaveUserConfig(void);

#endif
