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

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/objstr.h"

#include "FreeRTOS.h"
#include "task.h"

#include "chip/M480.h"

#include "mods/pybuart.h"

#include "extmod/misc.h"


MP_WEAK uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    return mp_uos_dupterm_poll(poll_flags);
}

MP_WEAK int mp_hal_stdin_rx_chr(void) {
    for (;;) {
        int chr = mp_uos_dupterm_rx_chr();
        if(chr >= 0) {
            return chr;
        }
        MICROPY_EVENT_POLL_HOOK
    }
}

MP_WEAK void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    mp_uos_dupterm_tx_strn(str, len);
}

void mp_hal_stdout_tx_str(const char *str) {
    mp_hal_stdout_tx_strn(str, strlen(str));
}

// Efficiently convert "\n" to "\r\n"
void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len) {
    const char *last = str;
    while (len--) {
        if (*str == '\n') {
            if (str > last) {
                mp_hal_stdout_tx_strn(last, str - last);
            }
            mp_hal_stdout_tx_strn("\r\n", 2);
            ++str;
            last = str;
        } else {
            ++str;
        }
    }
    if (str > last) {
        mp_hal_stdout_tx_strn(last, str - last);
    }
}


mp_uint_t mp_hal_ticks_ms(void) {
    return xTaskGetTickCount();
}


mp_uint_t mp_hal_ticks_us(void) {
    uint irq_state = disable_irq();
    uint us = xTaskGetTickCount() * 1000 + (SysTick->LOAD - SysTick->VAL) / 192;    //CPU run at 192MHz
    enable_irq(irq_state);

    return us;
}


#if __CORTEX_M >= 0x03
void mp_hal_ticks_cpu_enable(void)
{
    if((DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk) == 0)
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

        #if __CORTEX_M == 7
        // on Cortex-M7 we must unlock the DWT before writing to its registers
        DWT->LAR = 0xc5acce55;
        #endif

        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }
}
#endif


void mp_hal_delay_ms(mp_uint_t ms)
{
    vTaskDelay(ms);
//    mp_hal_delay_us(ms*1000);
}


void mp_hal_delay_us(mp_uint_t us)
{
    uint i = us * 192 / 4;
    while(--i) __NOP();
}
