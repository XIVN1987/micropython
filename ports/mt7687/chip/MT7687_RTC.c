#include "MT7687.h"


static bool RTC_Unlock(void);
static bool RTC_Sync(void);


bool RTC_Init(void)
{
    if(RTC_Unlock() == false)
        return false;

    RTC->XOSCCR &= ~RTC_XOSCCR_PDN_Msk;

    return true;
}

void RTC_Start(void)
{
    RTC->CR &= ~RTC_CR_RCSTOP_Msk;
}

void RTC_Stop(void)
{
    RTC->CR |=  RTC_CR_RCSTOP_Msk;
}

void RTC_TimeSet(RTC_TimeStruct *time)
{
    RTC->TC_YEA = time->year;
    RTC->TC_MON = time->month;
    RTC->TC_DAY = time->day;
    RTC->TC_DOW = time->dayOfWeek;
    RTC->TC_HOU = time->hour;
    RTC->TC_MIN = time->minute;
    RTC->TC_SEC = time->second;
}

bool RTC_TimeGet(RTC_TimeStruct *time)
{
    if(RTC_Sync() == false)
        return false;

    time->year = RTC->TC_YEA;

    if(RTC_Sync() == false)
        return false;

    time->month = RTC->TC_MON;

    if(RTC_Sync() == false)
        return false;

    time->day = RTC->TC_DAY;

    if(RTC_Sync() == false)
        return false;

    time->dayOfWeek = RTC->TC_DOW;

    if(RTC_Sync() == false)
        return false;

    time->hour = RTC->TC_HOU;

    if(RTC_Sync() == false)
        return false;

    time->minute = RTC->TC_MIN;

    if(RTC_Sync() == false)
        return false;

    time->second = RTC->TC_SEC;

    return true;
}

void RTC_AlarmSet(RTC_TimeStruct *time)
{
    RTC->AL_YEA = time->year;
    RTC->AL_MON = time->month;
    RTC->AL_DAY = time->day;
    RTC->AL_DOW = time->dayOfWeek;
    RTC->AL_HOU = time->hour;
    RTC->AL_MIN = time->minute;
    RTC->AL_SEC = time->second;
}

void RTC_AlarmGet(RTC_TimeStruct *time)
{
    time->year      = RTC->AL_YEA;
    time->month     = RTC->AL_MON;
    time->day       = RTC->AL_DAY;
    time->dayOfWeek = RTC->AL_DOW;
    time->hour      = RTC->AL_HOU;
    time->minute    = RTC->AL_MIN;
    time->second    = RTC->AL_SEC;
}

void RTC_AlarmCompareSet(uint32_t compare)
{
    RTC->ALMCR &= ~(RTC_ALMCR_YEA_Msk |
                    RTC_ALMCR_MON_Msk |
                    RTC_ALMCR_DAY_Msk |
                    RTC_ALMCR_DOW_Msk |
                    RTC_ALMCR_HOU_Msk |
                    RTC_ALMCR_MIN_Msk |
                    RTC_ALMCR_SEC_Msk);

    RTC->ALMCR |= compare;
}

void RTC_AlarmOpen(void)
{
    RTC->ALMCR |=  RTC_ALMCR_EN_Msk;
}

void RTC_AlarmClose(void)
{
    RTC->ALMCR &= ~RTC_ALMCR_EN_Msk;
}

static bool RTC_Unlock(void)
{
    uint32_t cnt = 0;

    do {
        if(++cnt > 1000000)
            return false;
    } while((RTC->COREPDN & RTC_COREPDN_RTCWEN_Msk) == 0);

    /* Set RTC Power Check */
    RTC->PWRCHK1 = 0xC6;
    RTC->PWRCHK2 = 0x9A;

    /* Set RTC Key */
    RTC->KEY = 0x59;

    /* Set RTC Protections */
    RTC->PROT1 = 0xA3;
    RTC->PROT2 = 0x57;
    RTC->PROT3 = 0x67;
    RTC->PROT4 = 0xD2;

    return true;
}

static bool RTC_Sync(void)
{
    uint32_t cnt = 0;

    do {
        if(++cnt > 1000000)
            return false;
    } while(RTC->CR & RTC_CR_SYNCBUSY_Msk);

    return true;
}
