#include "MT7687.h"


void ADC_Init(ADC_TypeDef * ADCx, ADC_InitStructure * initStruct)
{
	ADCx->CR0 &= ~(ADC_CR0_AVG_Msk | ADC_CR0_MODE_Msk | ADC_CR0_CHEN_Msk);

	ADCx->CR0 |= (initStruct->Mode     << ADC_CR0_MODE_Pos) |
				 (initStruct->Channels << ADC_CR0_CHEN_Pos) |
				 (initStruct->SamplAvg << ADC_CR0_AVG_Pos);

	ADCx->CR1 = initStruct->Interval;

	uint32_t nchnl = ((initStruct->Channels >> 3) & 1) +
					 ((initStruct->Channels >> 2) & 1) +
					 ((initStruct->Channels >> 1) & 1) +
					 ((initStruct->Channels >> 0) & 1);
	ADCx->FFTRG = (nchnl - 1) << ADC_FFTRG_LVL_Pos;
}

void ADC_Start(ADC_TypeDef * ADCx)
{
    ADCx->CR0 |= ADC_CR0_EN_Msk;
}

void ADC_Stop(ADC_TypeDef * ADCx)
{
    ADCx->CR0 &= ~ADC_CR0_EN_Msk;
}

void ADC_ChnlSelect(ADC_TypeDef * ADCx, uint32_t channels)
{
	ADCx->CR0 &= ~ADC_CR0_CHEN_Msk;

	ADCx->CR0 |= channels << ADC_CR0_CHEN_Pos;

	uint32_t nchnl = ((channels >> 3) & 1) +
					 ((channels >> 2) & 1) +
					 ((channels >> 1) & 1) +
					 ((channels >> 0) & 1);
	ADCx->FFTRG = (nchnl - 1) << ADC_FFTRG_LVL_Pos;
}

uint32_t ADC_DataAvailable(ADC_TypeDef * ADCx)
{
    uint32_t rdpos = (ADCx->RXPTR & ADC_RXPTR_RD_Msk) >> ADC_RXPTR_RD_Pos;
    uint32_t wrpos = (ADCx->RXPTR & ADC_RXPTR_WR_Msk) >> ADC_RXPTR_WR_Pos;

	if(wrpos == rdpos)
	{
		return 0;
    }
    else if(wrpos > rdpos)
    {
		return (wrpos - rdpos);
    }
    else
    {
		return (wrpos + 16 - rdpos);
    }
}

uint32_t ADC_Read(ADC_TypeDef * ADCx, uint32_t * chn)
{
	uint32_t reg = ADCx->DR;

	*chn = (reg & ADC_DR_CHNUM_Msk) >> ADC_DR_CHNUM_Pos;

	return (reg & ADC_DR_VALUE_Msk) >> ADC_DR_VALUE_Pos;
}
