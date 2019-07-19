#include "MT7687.h"


void PWM_ClockSelect(uint32_t clksrc)
{
	PWM->GCR &= ~PWM_GCR_CLKSRC_Msk;
	PWM->GCR |= (clksrc << PWM_GCR_CLKSRC_Pos);
}

void PWM_Init(uint32_t chn, uint16_t on_time, uint16_t off_time, uint32_t global_start)
{
	PWM->CH[chn].CR = (0 << PWM_CR_REPLAY_EN_Pos)     |
					  (0 << PWM_CR_POLARITY_Pos)      |
					  (0 << PWM_CR_OPEN_DRAIN_Pos)    |
					  (1 << PWM_CR_CLKEN_Pos)         |
					  (1 << PWM_CR_S0_STAY_CYCLE_Pos) |
					  (global_start << PWM_CR_GLOBAL_START_EN_Pos);
	
	PWM->CH[chn].S0 = (on_time << PWM_S0_ON_TIME_Pos) |
					  (off_time << PWM_S0_OFF_TIME_Pos);
}

void PWM_Start(uint32_t chn)
{
	PWM->CH[chn].CR |= (1 << PWM_CR_START_Pos);
}

void PWM_Stop(uint32_t chn)
{
	PWM->CH[chn].CR &= ~(1 << PWM_CR_START_Pos);
}

void PWM_GStart(void)
{
	PWM->GCR |= (1 << PWM_GCR_START_Pos);
}

void PWM_GStop(void)
{
	PWM->GCR &= ~(1 << PWM_GCR_START_Pos);
}

void PWM_SetOnTime(uint32_t chn, uint16_t on_time)
{
    PWM->CH[chn].S0 &= ~PWM_S0_ON_TIME_Msk;
    PWM->CH[chn].S0 |= (on_time << PWM_S0_ON_TIME_Pos);
}

uint16_t PWM_GetOnTime(uint32_t chn)
{
    return ((PWM->CH[chn].S0 & PWM_S0_ON_TIME_Msk) >> PWM_S0_ON_TIME_Pos);
}

void PWM_SetOffTime(uint32_t chn, uint16_t off_time)
{
    PWM->CH[chn].S0 &= ~PWM_S0_OFF_TIME_Msk;
    PWM->CH[chn].S0 |= (off_time << PWM_S0_OFF_TIME_Pos);
}

uint16_t PWM_GetOffTime(uint32_t chn)
{
    return ((PWM->CH[chn].S0 & PWM_S0_OFF_TIME_Msk) >> PWM_S0_OFF_TIME_Pos);
}
