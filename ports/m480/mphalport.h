#include <stdint.h>
#include <stdbool.h>


void mp_hal_ticks_cpu_enable(void);
static inline mp_uint_t mp_hal_ticks_cpu(void)
{
#if __CORTEX_M == 0
    return 0;
#else
    if(!(DWT->CTRL & DWT_CTRL_CYCCNTENA_Msk))
    {
        mp_hal_ticks_cpu_enable();
    }

    return DWT->CYCCNT;
#endif
}


void mp_hal_set_interrupt_char(int c);
