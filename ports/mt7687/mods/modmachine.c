/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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

#include <stdio.h>
#include <stdint.h>

#include "py/runtime.h"
#include "py/mphal.h"
#include "pybpin.h"
#include "pybuart.h"
#include "pybtimer.h"
#include "pybpwm.h"
#include "moduos.h"
#include "FreeRTOS.h"
#include "portable.h"
#include "task.h"
#include "random.h"
#include "gccollect.h"
#include "mperror.h"


extern void*    mpTaskHandle;
extern void*    svTaskHandle;
extern void*    xSimpleLinkSpawnTaskHndl;


/// \module machine - functions related to the SoC
///

/******************************************************************************/
// MicroPython bindings;

STATIC mp_obj_t machine_reset(void) {
    // disable wlan
    // wlan_stop(SL_STOP_TIMEOUT_LONG);
    // reset the cpu and it's peripherals
    NVIC_SystemReset();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_obj, machine_reset);

STATIC mp_obj_t machine_info(uint n_args, const mp_obj_t *args) {
    // FreeRTOS info
    {
        printf("---------------------------------------------\n");
        printf("FreeRTOS\n");
        printf("---------------------------------------------\n");
        printf("Total heap: %u\n", configTOTAL_HEAP_SIZE);
        /*
        printf("Free heap: %u\n", xPortGetFreeHeapSize());
        printf("MpTask min free stack: %u\n", (unsigned int)uxTaskGetStackHighWaterMark((TaskHandle_t)mpTaskHandle));
        printf("ServersTask min free stack: %u\n", (unsigned int)uxTaskGetStackHighWaterMark((TaskHandle_t)svTaskHandle));
        printf("SlTask min free stack: %u\n", (unsigned int)uxTaskGetStackHighWaterMark(xSimpleLinkSpawnTaskHndl));
        printf("IdleTask min free stack: %u\n", (unsigned int)uxTaskGetStackHighWaterMark(xTaskGetIdleTaskHandle()));
        */
        uint32_t *pstack = (uint32_t *)&__stack_start__;
        while (*pstack == 0x55555555) {
            pstack++;
        }
        printf("MAIN min free stack: %u\n", pstack - ((uint32_t *)&__stack_start__));
        printf("---------------------------------------------\n");
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(machine_info_obj, 0, 1, machine_info);

STATIC mp_obj_t machine_freq(void) {
    return mp_obj_new_int(192000000);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_freq_obj, machine_freq);

STATIC mp_obj_t machine_unique_id(void) {
    // uint8_t mac[SL_BSSID_LENGTH];
    // wlan_get_mac (mac);
    uint8_t mac[8] = {0x11, 0x22, 0x33};
    return mp_obj_new_bytes(mac, 8);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_unique_id_obj, machine_unique_id);

STATIC mp_obj_t machine_main(mp_obj_t main) {
    if (MP_OBJ_IS_STR(main)) {
        MP_STATE_PORT(machine_config_main) = main;
    } else {
        mp_raise_ValueError(mpexception_value_invalid_arguments);
    }
    return mp_const_none;
}
MP_DEFINE_CONST_FUN_OBJ_1(machine_main_obj, machine_main);

STATIC mp_obj_t machine_idle(void) {
    __WFI();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_idle_obj, machine_idle);

STATIC mp_obj_t machine_sleep (void) {
    //pyb_sleep_sleep();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_sleep_obj, machine_sleep);

STATIC mp_obj_t machine_deepsleep (void) {
    //pyb_sleep_deepsleep();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_deepsleep_obj, machine_deepsleep);

STATIC mp_obj_t machine_reset_cause (void) {
    //return mp_obj_new_int(pyb_sleep_get_reset_cause());
    return 0;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_reset_cause_obj, machine_reset_cause);

STATIC mp_obj_t machine_wake_reason (void) {
    //return mp_obj_new_int(pyb_sleep_get_wake_reason());
    return 0;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(machine_wake_reason_obj, machine_wake_reason);

STATIC const mp_rom_map_elem_t machine_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),            MP_ROM_QSTR(MP_QSTR_umachine) },

    { MP_ROM_QSTR(MP_QSTR_reset),               MP_ROM_PTR(&machine_reset_obj) },
    { MP_ROM_QSTR(MP_QSTR_info),                MP_ROM_PTR(&machine_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_freq),                MP_ROM_PTR(&machine_freq_obj) },
    { MP_ROM_QSTR(MP_QSTR_unique_id),           MP_ROM_PTR(&machine_unique_id_obj) },
    { MP_ROM_QSTR(MP_QSTR_main),                MP_ROM_PTR(&machine_main_obj) },
    { MP_ROM_QSTR(MP_QSTR_rng),                 MP_ROM_PTR(&machine_rng_get_obj) },
    { MP_ROM_QSTR(MP_QSTR_idle),                MP_ROM_PTR(&machine_idle_obj) },
    { MP_ROM_QSTR(MP_QSTR_sleep),               MP_ROM_PTR(&machine_sleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_deepsleep),           MP_ROM_PTR(&machine_deepsleep_obj) },
    { MP_ROM_QSTR(MP_QSTR_reset_cause),         MP_ROM_PTR(&machine_reset_cause_obj) },
    { MP_ROM_QSTR(MP_QSTR_wake_reason),         MP_ROM_PTR(&machine_wake_reason_obj) },

    // { MP_ROM_QSTR(MP_QSTR_disable_irq),         MP_ROM_PTR(&pyb_disable_irq_obj) },
    // { MP_ROM_QSTR(MP_QSTR_enable_irq),          MP_ROM_PTR(&pyb_enable_irq_obj) },

    { MP_ROM_QSTR(MP_QSTR_Pin),                 MP_ROM_PTR(&pyb_pin_type) },
    { MP_ROM_QSTR(MP_QSTR_UART),                MP_ROM_PTR(&pyb_uart_type) },
    { MP_ROM_QSTR(MP_QSTR_Timer),               MP_ROM_PTR(&pyb_timer_type) },
    { MP_ROM_QSTR(MP_QSTR_PWM),                 MP_ROM_PTR(&pyb_pwm_type) },
    // { MP_ROM_QSTR(MP_QSTR_ADC),                 MP_ROM_PTR(&pyb_adc_type) },
    // { MP_ROM_QSTR(MP_QSTR_I2C),                 MP_ROM_PTR(&pyb_i2c_type) },
    // { MP_ROM_QSTR(MP_QSTR_SPI),                 MP_ROM_PTR(&pyb_spi_type) },

    // { MP_ROM_QSTR(MP_QSTR_WDT),                 MP_ROM_PTR(&pyb_wdt_type) },

    // class constants
    // { MP_ROM_QSTR(MP_QSTR_IDLE),                MP_ROM_INT(PYB_PWR_MODE_ACTIVE) },
    // { MP_ROM_QSTR(MP_QSTR_SLEEP),               MP_ROM_INT(PYB_PWR_MODE_LPDS) },
    // { MP_ROM_QSTR(MP_QSTR_DEEPSLEEP),           MP_ROM_INT(PYB_PWR_MODE_HIBERNATE) },
    // { MP_ROM_QSTR(MP_QSTR_POWER_ON),            MP_ROM_INT(PYB_SLP_PWRON_RESET) }, // legacy constant
    // { MP_ROM_QSTR(MP_QSTR_PWRON_RESET),         MP_ROM_INT(PYB_SLP_PWRON_RESET) },
    // { MP_ROM_QSTR(MP_QSTR_HARD_RESET),          MP_ROM_INT(PYB_SLP_HARD_RESET) },
    // { MP_ROM_QSTR(MP_QSTR_WDT_RESET),           MP_ROM_INT(PYB_SLP_WDT_RESET) },
    // { MP_ROM_QSTR(MP_QSTR_DEEPSLEEP_RESET),     MP_ROM_INT(PYB_SLP_HIB_RESET) },
    // { MP_ROM_QSTR(MP_QSTR_SOFT_RESET),          MP_ROM_INT(PYB_SLP_SOFT_RESET) },
    // { MP_ROM_QSTR(MP_QSTR_WLAN_WAKE),           MP_ROM_INT(PYB_SLP_WAKED_BY_WLAN) },
    // { MP_ROM_QSTR(MP_QSTR_PIN_WAKE),            MP_ROM_INT(PYB_SLP_WAKED_BY_GPIO) },
    // { MP_ROM_QSTR(MP_QSTR_RTC_WAKE),            MP_ROM_INT(PYB_SLP_WAKED_BY_RTC) },
};

STATIC MP_DEFINE_CONST_DICT(machine_module_globals, machine_module_globals_table);

const mp_obj_module_t machine_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&machine_module_globals,
};
