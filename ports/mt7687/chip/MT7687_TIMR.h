#ifndef __MT7687_TIMR_H__
#define __MT7687_TIMR_H__


void TIMR_Init(uint32_t TIMRx, uint32_t period, uint32_t int_en);
void TIMR_Start(uint32_t TIMRx);
void TIMR_Stop(uint32_t TIMRx);

void TIMR_Restart(uint32_t TIMRx);

void TIMR_SetPeriod(uint32_t TIMRx, uint32_t period);
uint32_t TIMR_CurrentValue(uint32_t TIMRx);


#define TIMR0	0
#define TIMR1	1
#define TIMR2	2
#define TIMR4	4


#endif //__MT7687_TIMR_H__
