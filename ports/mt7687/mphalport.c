#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "MT7687.h"

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/objstr.h"

#include "extmod/misc.h"

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "mods/pybuart.h"


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


int mp_hal_stdin_rx_chr(void) {
    for (;;) {
        int chr = mp_uos_dupterm_rx_chr();
        if(chr >= 0) {
            return chr;
        }
        MICROPY_EVENT_POLL_HOOK
    }
}

void mp_hal_stdout_tx_strn(const char *str, size_t len) {
    mp_uos_dupterm_tx_strn(str, len);
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

void mp_hal_stdout_tx_str(const char *str) {
    mp_hal_stdout_tx_strn(str, strlen(str));
}

uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags) {
    return mp_uos_dupterm_poll(poll_flags);
}
