#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/objstr.h"

#include "chip/M480.h"

#include "mods/pybuart.h"

#include "extmod/misc.h"

#include "irq.h"


static volatile uint32_t msTick;

void SysTick_Handler(void)
{
    msTick++;

    // clear the COUNTFLAG bit, which makes the logic in mp_hal_ticks_us work properly.
    SysTick->CTRL;
}


// Core delay function that does an efficient sleep and may switch thread context.
// If IRQs are enabled then we must have the GIL.
void mp_hal_delay_ms(mp_uint_t ms)
{
    if(query_irq() == IRQ_STATE_ENABLED)
    {
        const uint start = msTick;
        // Wraparound of tick is taken care of by 2's complement arithmetic.
        while(msTick - start < ms) {
            // This macro will execute the necessary idle behaviour.  It may
            // raise an exception, switch threads or enter sleep mode (waiting for
            // (at least) the SysTick interrupt).
            MICROPY_EVENT_POLL_HOOK
        }
    }
    else
    {
        // To prevent possible overflow of the counter we use a double loop.
        const uint count_1ms = SystemCoreClock / 4000;
        for(uint i = 0; i < ms; i++) {
            for(uint j = 0; j <= count_1ms; j++) {
            }
        }
    }
}


void mp_hal_delay_us(mp_uint_t us)
{
    if(query_irq() == IRQ_STATE_ENABLED)
    {
        const uint start = mp_hal_ticks_us();
        while(mp_hal_ticks_us() - start < us) {
        }
    }
    else
    {
        const uint count_1us = SystemCoreClock / 4000000 * us;
        for(uint i = 0; i <= count_1us; i++) {
        }
    }
}


mp_uint_t mp_hal_ticks_ms(void)
{
    return msTick;
}


mp_uint_t mp_hal_ticks_us(void)
{
    uint irq_state = disable_irq();
    uint counter = SysTick->VAL;
    uint ms = msTick;
    uint status = SysTick->CTRL;
    enable_irq(irq_state);

    // It's still possible for the countflag bit to get set if the counter was
    // reloaded between reading VAL and reading CTRL. With interrupts  disabled
    // it definitely takes less than 50 HCLK cycles between reading VAL and
    // reading CTRL, so the test (counter > 50) is to cover the case where VAL
    // is +ve and very close to zero, and the COUNTFLAG bit is also set.
    if((status & SysTick_CTRL_COUNTFLAG_Msk) && (counter > 50))
    {
        ms++;
    }

    uint load = SysTick->LOAD;  // load = SystemCoreClock / 1000 - 1
    counter = load - counter;   // SysTick counts down

    return ms * 1000 + (counter * 1000) / (load + 1);  // ms * 1000 + counter / ((load + 1) / 1000)
}


mp_uint_t mp_hal_ticks_cpu(void)
{
    if(!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk))
    {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;

        #if __CORTEX_M == 7
        // on Cortex-M7 we must unlock the DWT before writing to its registers
        DWT->LAR = 0xc5acce55;
        #endif

        DWT->CYCCNT = 0;
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
    }

    return DWT->CYCCNT;
}


// Nanoseconds since the Epoch.
uint64_t mp_hal_time_ns(void)
{
    return 0;   // not implemented
}


uintptr_t mp_hal_stdio_poll(uintptr_t poll_flags)
{
    return mp_os_dupterm_poll(poll_flags);
}


int mp_hal_stdin_rx_chr(void)
{
    for(;;)
    {
        int chr = mp_os_dupterm_rx_chr();
        if(chr >= 0)
        {
            return chr;
        }
        MICROPY_EVENT_POLL_HOOK
    }
}


mp_uint_t mp_hal_stdout_tx_strn(const char *str, size_t len)
{
    mp_os_dupterm_tx_strn(str, len);
}


void mp_hal_stdout_tx_str(const char *str)
{
    mp_hal_stdout_tx_strn(str, strlen(str));
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
