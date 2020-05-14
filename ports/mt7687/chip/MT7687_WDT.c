#include "MT7687.h"


void WDT_Init(uint32_t mode, uint32_t time)
{
    WDT->MODE = (mode << WDT_MODE_INT_Pos) |
                (0x22 << WDT_MODE_KEY_Pos);

    if(mode == WDT_MODE_RESET)
    {
        WDT->RSTT = time;
    }
    else
    {
        WDT->INTT = (time << WDT_INTT_TIME_Pos) |
                    (0x08 << WDT_INTT_KEY_Pos);
    }
}


void WDT_Start(void)
{
    WDT->MODE |= (1 << WDT_MODE_EN_Pos) |
                 (0x22 << WDT_MODE_KEY_Pos);
}


void WDT_Stop(void)
{
    uint32_t reg;

    reg = WDT->MODE;
    reg &= ~(1 << WDT_MODE_EN_Pos);
    reg |= (0x22 << WDT_MODE_KEY_Pos);
    WDT->MODE = reg;
}


void WDT_Feed(void)
{
    WDT->FEED = 0x1971;
}
