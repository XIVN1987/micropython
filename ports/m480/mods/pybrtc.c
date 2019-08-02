/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdio.h>

#include "py/runtime.h"

#include "chip/M480.h"

#include "mods/pybrtc.h"

#include "lib/timeutils/timeutils.h"


void rtc_init(void)
{
    CLK->PWRCTL |= CLK_PWRCTL_LXTEN_Msk;        // 32K (LXT) Enabled
    CLK->APBCLK0 |= CLK_APBCLK0_RTCCKEN_Msk;    // RTC Clock Enable
}


uint32_t rtc_get(void)
{
    S_RTC_TIME_DATA_T sRTCTimeData;

    RTC_GetDateAndTime(&sRTCTimeData);

    return timeutils_seconds_since_2000(sRTCTimeData.u32Year, sRTCTimeData.u32Month,
            sRTCTimeData.u32Day, sRTCTimeData.u32Hour, sRTCTimeData.u32Minute, sRTCTimeData.u32Second);
}


void rtc_set(uint32_t seconds)
{
    timeutils_struct_time_t tm;
    timeutils_seconds_since_2000_to_struct_time(seconds, &tm);

    S_RTC_TIME_DATA_T sRTCTimeData;
    sRTCTimeData.u32Year = tm.tm_year;
    sRTCTimeData.u32Month = tm.tm_mon;
    sRTCTimeData.u32Day = tm.tm_mday;
    sRTCTimeData.u32Hour = tm.tm_hour;
    sRTCTimeData.u32Minute = tm.tm_min;
    sRTCTimeData.u32Second = tm.tm_sec;
    sRTCTimeData.u32TimeScale = RTC_CLOCK_24;

    RTC_SetDateAndTime(&sRTCTimeData);
}


STATIC mp_obj_t pyb_rtc_get(void) {
    timeutils_struct_time_t tm;
    timeutils_seconds_since_2000_to_struct_time(rtc_get(), &tm);

    mp_obj_t tuple[8];
    tuple[0] = mp_obj_new_int(tm.tm_year);
    tuple[1] = mp_obj_new_int(tm.tm_mon);
    tuple[2] = mp_obj_new_int(tm.tm_mday);
    tuple[4] = mp_obj_new_int(tm.tm_hour);
    tuple[5] = mp_obj_new_int(tm.tm_min);
    tuple[6] = mp_obj_new_int(tm.tm_sec);
    tuple[7] = mp_obj_new_int(0);           // millisecond
    tuple[3] = mp_obj_new_int(tm.tm_wday);

    return mp_obj_new_tuple(8, tuple);
}
MP_DEFINE_CONST_FUN_OBJ_0(pyb_rtc_get_obj, pyb_rtc_get);


STATIC mp_obj_t pyb_rtc_set(const mp_obj_t tuple) {
    mp_obj_t *items;
    mp_obj_get_array_fixed_n(tuple, 8, &items);

    uint seconds = timeutils_seconds_since_2000(
                mp_obj_get_int(items[0]),   // year
                mp_obj_get_int(items[1]),   // month
                mp_obj_get_int(items[2]),   // day
                mp_obj_get_int(items[3]),   // hour
                mp_obj_get_int(items[4]),   // minute
                mp_obj_get_int(items[5]));  // second

    rtc_set(seconds);

    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(pyb_rtc_set_obj, pyb_rtc_set);
