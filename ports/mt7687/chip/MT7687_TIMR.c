#include "MT7687.h"


void TIMR_Init(uint32_t TIMRx, uint32_t period, uint32_t int_en)
{
	TIMR_Stop(TIMRx);
	
	switch(TIMRx)
	{
	case TIMR0:
		TIMR->T0CR |= (1 << TIMR_CR_AR_Pos) |
					  (0 << TIMR_CR_32K_Pos);
		TIMR->T0IV = period;
	
		if(int_en)
		{
			TIMR->IF = (1 << TIMR_IF_T0_Pos);
			TIMR->IE |= (1 << TIMR_IE_T0_Pos);
			NVIC_EnableIRQ(GPT_IRQ);
		}
		break;
	
	case TIMR1:
		TIMR->T1CR |= (1 << TIMR_CR_AR_Pos) |
					  (0 << TIMR_CR_32K_Pos);
		TIMR->T1IV = period;
	
		if(int_en)
		{
			TIMR->IF = (1 << TIMR_IF_T1_Pos);
			TIMR->IE |= (1 << TIMR_IE_T1_Pos);
			NVIC_EnableIRQ(GPT_IRQ);
		}
		break;
	}
}

void TIMR_Start(uint32_t TIMRx)
{
	switch(TIMRx)
	{
	case TIMR0:
		TIMR->T0CR |= (1 << TIMR_CR_EN_Pos);
		break;
	
	case TIMR1:
		TIMR->T1CR |= (1 << TIMR_CR_EN_Pos);
		break;
	}
}

void TIMR_Stop(uint32_t TIMRx)
{
	switch(TIMRx)
	{
	case TIMR0:
		TIMR->T0CR &= ~(1 << TIMR_CR_EN_Pos);
		break;
	
	case TIMR1:
		TIMR->T1CR &= ~(1 << TIMR_CR_EN_Pos);
		break;
	}
}

void TIMR_Restart(uint32_t TIMRx)
{
	switch(TIMRx)
	{
	case TIMR0:
		TIMR->T0CR |= (1 << TIMR_CR_RESTART_Pos);
		break;
	
	case TIMR1:
		TIMR->T1CR |= (1 << TIMR_CR_RESTART_Pos);
		break;
	}
}

void TIMR_SetPeriod(uint32_t TIMRx, uint32_t period)
{
    switch(TIMRx)
    {
    case TIMR0:
        TIMR->T0IV = period;
        break;

    case TIMR1:
        TIMR->T1IV = period;
        break;
    }
}

uint32_t TIMR_CurrentValue(uint32_t TIMRx)
{
	switch(TIMRx)
	{
	case TIMR0:
		return TIMR->T0V;
	
	case TIMR1:
		return TIMR->T1V;
	}
	
	return 0xFFFFFFFF;
}
