#ifndef __MT7687_ADC_H__
#define __MT7687_ADC_H__

#include <stdint.h>


typedef struct {
	uint8_t  Mode;			// ADC_MODE_ONE_TIME or ADC_MODE_PERIODIC
	uint8_t  Channels;		// ADC_CH0, ADC_CH1, ADC_CH2, ADC_CH3 and their 'or'
	uint8_t  SamplAvg;		// ADC_AVG_1SAMPLE, ADC_AVG_2SAMPLE, ADC_AVG_4SAMPLE, ...
	uint32_t Interval;		// Interval between two convert cycle in ADC_MODE_PERIODIC mode
} ADC_InitStructure;


#define ADC_CH0		1
#define ADC_CH1		2
#define ADC_CH2		4
#define ADC_CH3		8

#define ADC_MODE_ONE_TIME   0
#define ADC_MODE_PERIODIC	1

#define ADC_AVG_1SAMPLE 	0
#define ADC_AVG_2SAMPLE 	1
#define ADC_AVG_4SAMPLE 	2
#define ADC_AVG_8SAMPLE 	3
#define ADC_AVG_16SAMPLE 	4
#define ADC_AVG_32SAMPLE 	5
#define ADC_AVG_64SAMPLE 	6


void ADC_Init(ADC_TypeDef * ADCx, ADC_InitStructure * initStruct);
void ADC_Start(ADC_TypeDef * ADCx);
void ADC_Stop(ADC_TypeDef * ADCx);

void ADC_ChnlSelect(ADC_TypeDef * ADCx, uint32_t channels);

uint32_t ADC_DataAvailable(ADC_TypeDef * ADCx);
uint32_t ADC_Read(ADC_TypeDef * ADCx, uint32_t * chn);


#endif
