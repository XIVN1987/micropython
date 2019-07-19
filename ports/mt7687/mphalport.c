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

#include "MT7687.h"

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/objstr.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "moduos.h"

#include "pybuart.h"


mp_uint_t mp_hal_ticks_ms(void) {
    return xTaskGetTickCount();
}

mp_uint_t mp_hal_ticks_us(void) {
    return xTaskGetTickCount() * 1000 + (SysTick->LOAD - SysTick->VAL) / 192;    //CPU run at 192MHz
}

mp_uint_t mp_hal_ticks_cpu(void)
{
    return SysTick->LOAD - SysTick->VAL;
}

void mp_hal_delay_ms(mp_uint_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

void mp_hal_delay_us(mp_uint_t us)
{
    mp_uint_t start = mp_hal_ticks_us();

    while(mp_hal_ticks_us() < start + us)  __NOP();
}


void mp_hal_stdout_tx_str(const char *str) {
    mp_hal_stdout_tx_strn(str, strlen(str));
}

void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    if (MP_STATE_PORT(os_term_dup_obj)) {
        if (MP_OBJ_IS_TYPE(MP_STATE_PORT(os_term_dup_obj)->stream_o, &pyb_uart_type)) {
            uart_tx_strn(MP_STATE_PORT(os_term_dup_obj)->stream_o, str, len);
        } else {
            MP_STATE_PORT(os_term_dup_obj)->write[2] = mp_obj_new_str_of_type(&mp_type_str, (const byte *)str, len);
            mp_call_method_n_kw(1, 0, MP_STATE_PORT(os_term_dup_obj)->write);
        }
    }
}

void mp_hal_stdout_tx_strn_cooked (const char *str, size_t len) {
    int32_t nslen = 0;
    const char *_str = str;

    for (int i = 0; i < len; i++) {
        if (str[i] == '\n') {
            mp_hal_stdout_tx_strn(_str, nslen);
            mp_hal_stdout_tx_strn("\r\n", 2);
            _str += nslen + 1;
            nslen = 0;
        } else {
            nslen++;
        }
    }
    if (_str < str + len) {
        mp_hal_stdout_tx_strn(_str, nslen);
    }
}

int mp_hal_stdin_rx_chr(void) {
    for ( ;; ) {
        // read telnet first
        if (MP_STATE_PORT(os_term_dup_obj)) { // then the stdio_dup
            if (MP_OBJ_IS_TYPE(MP_STATE_PORT(os_term_dup_obj)->stream_o, &pyb_uart_type)) {
                if (uart_rx_any(MP_STATE_PORT(os_term_dup_obj)->stream_o)) {
                    return uart_rx_char(MP_STATE_PORT(os_term_dup_obj)->stream_o);
                }
            } else {
                MP_STATE_PORT(os_term_dup_obj)->read[2] = mp_obj_new_int(1);
                mp_obj_t data = mp_call_method_n_kw(1, 0, MP_STATE_PORT(os_term_dup_obj)->read);
                // data len is > 0
                if (mp_obj_is_true(data)) {
                    mp_buffer_info_t bufinfo;
                    mp_get_buffer_raise(data, &bufinfo, MP_BUFFER_READ);
                    return ((int *)(bufinfo.buf))[0];
                }
            }
        }
        mp_hal_delay_ms(1);
    }
}
