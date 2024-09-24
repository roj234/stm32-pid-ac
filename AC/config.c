
#include <stm32f10x.h>
#include "config.h"
#include "ui.h"

/* 各个扇区的基地址 */
#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000)
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000)
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000)
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000)
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000)
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000)
#define ADDR_FLASH_SECTOR_6     ((uint32_t)0x08040000)
#define ADDR_FLASH_SECTOR_7     ((uint32_t)0x08060000)
#define ADDR_FLASH_SECTOR_8     ((uint32_t)0x08080000)
#define ADDR_FLASH_SECTOR_9     ((uint32_t)0x080A0000)
#define ADDR_FLASH_SECTOR_10    ((uint32_t)0x080C0000)
#define ADDR_FLASH_SECTOR_11    ((uint32_t)0x080E0000)

#define ADDR_FLASH_SECTOR_12     ((uint32_t)0x08100000)
#define ADDR_FLASH_SECTOR_13     ((uint32_t)0x08104000)
#define ADDR_FLASH_SECTOR_14     ((uint32_t)0x08108000)
#define ADDR_FLASH_SECTOR_15     ((uint32_t)0x0810C000)
#define ADDR_FLASH_SECTOR_16     ((uint32_t)0x08110000)
#define ADDR_FLASH_SECTOR_17     ((uint32_t)0x08120000)
#define ADDR_FLASH_SECTOR_18     ((uint32_t)0x08140000)
#define ADDR_FLASH_SECTOR_19     ((uint32_t)0x08160000)
#define ADDR_FLASH_SECTOR_20     ((uint32_t)0x08180000)
#define ADDR_FLASH_SECTOR_21     ((uint32_t)0x081A0000)
#define ADDR_FLASH_SECTOR_22     ((uint32_t)0x081C0000)
#define ADDR_FLASH_SECTOR_23     ((uint32_t)0x081E0000)

#define CONFIG_ADDRESS 0x08008000

struct CONFIG_STRUCT Config;


int FlashWrite(uint32_t Address, uint32_t *Data, uint32_t Length) {
    FLASH_Status status = FLASH_COMPLETE;

    FLASH_Unlock();
    /* 清空标志位 */
    FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_PGERR | FLASH_FLAG_WRPRTERR);

    /* 按页擦除*/
    uint32_t PageToClear = (Length + 1023) >> 10;
    for (uint32_t i = 0; i < PageToClear && status == FLASH_COMPLETE; i++) {
        status = FLASH_ErasePage(Address + (i << 10));
    }

		uint32_t *data = Data, addr = Address, end = addr + Length;

		while (addr < end && status == FLASH_COMPLETE) {
        status = FLASH_ProgramWord(addr, *data++);
				addr += 4;
    }

    FLASH_Lock();

    /* 校验 */
		while ((Address < end) && (status == FLASH_COMPLETE)) {
        if ((*(__IO uint32_t*) Address) != *Data++) {
						return -1234;
        }
        Address += 4;
    }
    return status;
}

__STATIC_INLINE void FlashRead(uint32_t Address, uint32_t *Data, uint32_t Length) {
		uint32_t end = Address + Length;
		while (Address < end) {
			 *Data++ = *(__IO uint32_t*) Address;
       Address += 4;
    }
}

__STATIC_INLINE void MemZero(uint32_t *Data, uint32_t Length) {
		while (Length--) {
			 *Data++ = 0;
    }
}

CFGSTATUS LoadUserConfig() {
	uint32_t len = sizeof(Config) & ~3;
	FlashRead(CONFIG_ADDRESS, (uint32_t*) &Config, len);
	if (Config.initFlag == 0xFFFF) {
		MemZero((uint32_t*) &Config, len);
		Config.bgLightActive = 75;
		Config.bgSleepTime = 1000;
		Config.initFlag = 0xFFFF;
	}
	return CFG_OK;
}

CFGSTATUS SaveUserConfig() {
	uint32_t len = sizeof(Config) & ~3;
	Config.initFlag = 0;
	return FlashWrite(CONFIG_ADDRESS, (uint32_t*) &Config, len);
}
