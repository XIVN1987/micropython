/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Daniel Campora
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


 /******************************************************************************
 IMPORTS
 ******************************************************************************/
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "SWM320.h"

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/objstr.h"

#include "extmod/misc.h"

#include "mods/pybuart.h"


volatile uint32_t SysTick_ms = 0;   // 每毫秒加一

void SysTick_Handler(void)
{
    SysTick_ms++;
}


mp_uint_t mp_hal_ticks_ms(void)
{
    return SysTick_ms;
}

mp_uint_t mp_hal_ticks_us(void)
{
    return SysTick_ms * 1000 + (SysTick->LOAD - SysTick->VAL) / CyclesPerUs;
}

mp_uint_t mp_hal_ticks_cpu(void)
{
    return SysTick->LOAD - SysTick->VAL;
}

void mp_hal_delay_ms(mp_uint_t ms)
{
    uint32_t target_ms = mp_hal_ticks_ms() + ms;

    while(mp_hal_ticks_ms() < target_ms) __NOP();
}

void mp_hal_delay_us(mp_uint_t us)
{
    mp_uint_t target_us = mp_hal_ticks_us() + us;

    while(mp_hal_ticks_us() < target_us)  __NOP();
}


uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags)
{
    return mp_uos_dupterm_poll(poll_flags);
}

int mp_hal_stdin_rx_chr(void)
{
    for(;;)
    {
        int chr = mp_uos_dupterm_rx_chr();
        if(chr >= 0)
        {
            return chr;
        }
        MICROPY_EVENT_POLL_HOOK
    }
}

void mp_hal_stdout_tx_strn(const char *str, size_t len)
{
    mp_uos_dupterm_tx_strn(str, len);
}

// Efficiently convert "\n" to "\r\n"
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len)
{
    const char *last = str;
    while(len--)
    {
        if(*str == '\n')
        {
            if(str > last)
            {
                mp_hal_stdout_tx_strn(last, str - last);
            }
            mp_hal_stdout_tx_strn("\r\n", 2);
            ++str;
            last = str;
        }
        else
        {
            ++str;
        }
    }
    if(str > last)
    {
        mp_hal_stdout_tx_strn(last, str - last);
    }
}

void mp_hal_stdout_tx_str(const char *str)
{
    mp_hal_stdout_tx_strn(str, strlen(str));
}
