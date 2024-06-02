#include <stdio.h>
#include <stdint.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/gc.h"

#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#include "mods/pybpin.h"
#include "mods/pybuart.h"
#include "mods/pybtimer.h"
#include "mods/pybspi.h"
#include "mods/pybi2c.h"
#include "mods/pybcan.h"
#include "mods/pybadc.h"
#include "mods/pybdac.h"
#include "mods/pybpwm.h"
#include "mods/pybrng.h"
#include "mods/pybrtc.h"
#include "mods/pybwdt.h"

#include "misc/gccollect.h"


/// \module machine - functions related to the SoC
///

/******************************************************************************/
// MicroPython bindings;

static mp_obj_t machine_info(uint n_args, const mp_obj_t *args)
{
    // get and print clock speeds
    {
        printf("SystemCoreClock = %u\n", SystemCoreClock);
    }

    // to print info about memory
    {
        printf("__data_start__ = %p\n", &__data_start__);
        printf("__data_end__   = %p\n", &__data_end__);
        printf("__bss_start__  = %p\n", &__bss_start__);
        printf("__bss_end__    = %p\n", &__bss_end__);
        printf("__HeapBase     = %p\n", &__HeapBase);
        printf("__HeapLimit    = %p\n", &__HeapLimit);
        printf("__StackLimit   = %p\n", &__StackLimit);
        printf("__StackTop     = %p\n", &__StackTop);
    }

    // qstr info
    {
        size_t n_pool, n_qstr, n_str_data_bytes, n_total_bytes;
        qstr_pool_info(&n_pool, &n_qstr, &n_str_data_bytes, &n_total_bytes);
        printf("qstr:\n  n_pool=%u\n  n_qstr=%u\n  n_str_data_bytes=%u\n  n_total_bytes=%u\n", n_pool, n_qstr, n_str_data_bytes, n_total_bytes);
    }

    // GC info
    {
        gc_info_t info;
        gc_info(&info);
        printf("GC:\n");
        printf("  %u total\n", info.total);
        printf("  %u : %u\n", info.used, info.free);
        printf("  1=%u 2=%u m=%u\n", info.num_1block, info.num_2block, info.max_block);
    }

    if (n_args == 1) {
        // arg given means dump gc allocation table
        gc_dump_alloc_table(&mp_plat_print);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_info_obj, 0, 1, machine_info);


static mp_obj_t machine_freq(void)
{
    return mp_obj_new_int(SystemCoreClock);
}
static MP_DEFINE_CONST_FUN_OBJ_0(machine_freq_obj, machine_freq);


static mp_obj_t machine_reset(void)
{
    NVIC_SystemReset();

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_obj, machine_reset);


static mp_obj_t machine_sleep (void)
{
    //pyb_sleep_sleep();
    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(machine_sleep_obj, machine_sleep);


static mp_obj_t machine_deepsleep (void)
{
    //pyb_sleep_deepsleep();

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_0(machine_deepsleep_obj, machine_deepsleep);


static mp_obj_t machine_reset_cause (void)
{
    //return mp_obj_new_int(pyb_sleep_get_reset_cause());
    return 0;
}
static MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_cause_obj, machine_reset_cause);


static mp_obj_t machine_wake_reason (void)
{
    //return mp_obj_new_int(pyb_sleep_get_wake_reason());
    return 0;
}
static MP_DEFINE_CONST_FUN_OBJ_0(machine_wake_reason_obj, machine_wake_reason);


static const mp_rom_map_elem_t machine_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_machine) },

    { MP_ROM_QSTR(MP_QSTR_info),                MP_ROM_PTR(&machine_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_freq),                MP_ROM_PTR(&machine_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset),               MP_ROM_PTR(&machine_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep),               MP_ROM_PTR(&machine_sleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_deepsleep),           MP_ROM_PTR(&machine_deepsleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_cause),         MP_ROM_PTR(&machine_reset_cause_obj) },
    { MP_ROM_QSTR(MP_QSTR_wake_reason),         MP_ROM_PTR(&machine_wake_reason_obj) },

    { MP_ROM_QSTR(MP_QSTR_Pin),                 MP_ROM_PTR(&pin_type) },
    { MP_ROM_QSTR(MP_QSTR_UART),                MP_ROM_PTR(&pyb_uart_type) },
    { MP_ROM_QSTR(MP_QSTR_Timer),               MP_ROM_PTR(&pyb_timer_type) },
    { MP_ROM_QSTR(MP_QSTR_SPI),                 MP_ROM_PTR(&pyb_spi_type) },
    { MP_ROM_QSTR(MP_QSTR_I2C),                 MP_ROM_PTR(&pyb_i2c_type) },
    { MP_ROM_QSTR(MP_QSTR_CAN),                 MP_ROM_PTR(&pyb_can_type) },
    { MP_ROM_QSTR(MP_QSTR_ADC),                 MP_ROM_PTR(&pyb_adc_type) },
    { MP_ROM_QSTR(MP_QSTR_DAC),                 MP_ROM_PTR(&pyb_dac_type) },
    { MP_ROM_QSTR(MP_QSTR_PWM),                 MP_ROM_PTR(&pyb_pwm_type) },
    { MP_ROM_QSTR(MP_QSTR_RNG),                 MP_ROM_PTR(&pyb_rng_type) },
    { MP_ROM_QSTR(MP_QSTR_RTC),                 MP_ROM_PTR(&pyb_rtc_type) },
    { MP_ROM_QSTR(MP_QSTR_WDT),                 MP_ROM_PTR(&pyb_wdt_type) },

    // class constants
    // { MP_ROM_QSTR(MP_QSTR_PWRON_RESET),         MP_ROM_INT(PYB_SLP_PWRON_RESET) },
    // { MP_ROM_QSTR(MP_QSTR_HARD_RESET),          MP_ROM_INT(PYB_SLP_HARD_RESET) },
    // { MP_ROM_QSTR(MP_QSTR_WDT_RESET),           MP_ROM_INT(PYB_SLP_WDT_RESET) },
    // { MP_ROM_QSTR(MP_QSTR_SOFT_RESET),          MP_ROM_INT(PYB_SLP_SOFT_RESET) },
    // { MP_ROM_QSTR(MP_QSTR_PIN_WAKE),            MP_ROM_INT(PYB_SLP_WAKED_BY_GPIO) },
    // { MP_ROM_QSTR(MP_QSTR_RTC_WAKE),            MP_ROM_INT(PYB_SLP_WAKED_BY_RTC) },
};
static MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);


const mp_obj_module_t mp_module_machine = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&machine_module_globals,
};

MP_REGISTER_EXTENSIBLE_MODULE(MP_QSTR_machine, mp_module_machine);
