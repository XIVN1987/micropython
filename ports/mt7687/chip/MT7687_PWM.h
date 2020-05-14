#ifndef __MT7687_PWM_H__
#define __MT7687_PWM_H__


void PWM_ClockSelect(uint32_t clksrc);

void PWM_Init(uint32_t chn, uint16_t on_time, uint16_t off_time, uint32_t global_start);
void PWM_Start(uint32_t chn);
void PWM_Stop(uint32_t chn);
void PWM_GStart(void);
void PWM_GStop(void);

void PWM_SetOnTime(uint32_t chn, uint16_t on_time);
uint16_t PWM_GetOnTime(uint32_t chn);
void PWM_SetOffTime(uint32_t chn, uint16_t off_time);
uint16_t PWM_GetOffTime(uint32_t chn);


#define PWM_CLKSRC_32KHz	0
#define PWM_CLKSRC_2MHz		1
#define PWM_CLKSRC_XTAL		2


#define PWM0	0
#define PWM1	1
#define PWM2	2
#define PWM3	3
#define PWM4	4
#define PWM5	5
#define PWM6	6
#define PWM7	7
#define PWM8	8
#define PWM9	9
#define PWM10	10
#define PWM11	11
#define PWM12	12
#define PWM13	13
#define PWM14	14
#define PWM15	15
#define PWM16	16
#define PWM17	17
#define PWM18	18
#define PWM19	19
#define PWM20	20
#define PWM21	21
#define PWM22	22
#define PWM23	23
#define PWM24	24
#define PWM25	25
#define PWM26	26
#define PWM27	27
#define PWM28	28
#define PWM29	29
#define PWM30	30
#define PWM31	31
#define PWM32	32
#define PWM33	33
#define PWM34	34
#define PWM35	35
#define PWM36	36
#define PWM37	37
#define PWM38	38
#define PWM39	39


#endif //__MT7687_PWM_H__
