#ifndef __MT7687_WDT_H__
#define __MT7687_WDT_H__

#include <stdint.h>


#define WDT_MODE_RESET       0
#define WDT_MODE_INTERRUPT   1


void WDT_Init(uint32_t mode, uint32_t time);
void WDT_Start(void);
void WDT_Stop(void);
void WDT_Feed(void);


#endif //__MT7687_WDT_H__
