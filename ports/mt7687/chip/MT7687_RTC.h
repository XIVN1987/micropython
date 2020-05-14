#ifndef __MT7687_RTC_H__
#define __MT7687_RTC_H__

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint32_t year;
    uint32_t month;
    uint32_t day;
    uint32_t dayOfWeek;
    uint32_t hour;
    uint32_t minute;
    uint32_t second;
} RTC_TimeStruct;


#define RTC_ALARM_SEC   (1 << 1)    // compare second
#define RTC_ALARM_MIN   (1 << 2)
#define RTC_ALARM_HOU   (1 << 3)
#define RTC_ALARM_DOW   (1 << 4)
#define RTC_ALARM_DAY   (1 << 5)
#define RTC_ALARM_MON   (1 << 6)
#define RTC_ALARM_YEA   (1 << 7)


bool RTC_Init(void);
void RTC_Start(void);
void RTC_Stop(void);
void RTC_TimeSet(RTC_TimeStruct *time);
bool RTC_TimeGet(RTC_TimeStruct *time);

void RTC_AlarmSet(RTC_TimeStruct *time);
void RTC_AlarmGet(RTC_TimeStruct *time);
void RTC_AlarmCompareSet(uint32_t compare);
void RTC_AlarmOpen(void);
void RTC_AlarmClose(void);


#endif //__MT7687_RTC_H__
