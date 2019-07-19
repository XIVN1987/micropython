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
#include <string.h>

#include "py/mpconfig.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/mphal.h"
#include "mperror.h"


/******************************************************************************
 DEFINE CONSTANTS
 ******************************************************************************/
#define MPERROR_TOOGLE_MS                           (50)
#define MPERROR_SIGNAL_ERROR_MS                     (1200)
#define MPERROR_HEARTBEAT_ON_MS                     (80)
#define MPERROR_HEARTBEAT_OFF_MS                    (3920)

/******************************************************************************
DECLARE EXPORTED DATA
 ******************************************************************************/
const char mpexception_value_invalid_arguments[]    = "invalid argument(s) value";
const char mpexception_num_type_invalid_arguments[] = "invalid argument(s) num/type";
const char mpexception_uncaught[]                   = "uncaught exception";

/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
struct mperror_heart_beat {
    uint32_t off_time;
    uint32_t on_time;
    bool beating;
    bool enabled;
    bool do_disable;
} mperror_heart_beat = {.off_time = 0, .on_time = 0, .beating = false, .enabled = false, .do_disable = false};

/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/
void mperror_init0 (void) {
//    pin_config ((pin_obj_t *)&MICROPY_SYS_LED_GPIO, PIN_MODE_0, GPIO_DIR_MODE_OUT, PIN_TYPE_STD, 0);

    mperror_heart_beat.enabled = true;
    mperror_heartbeat_switch_off();
}

void mperror_bootloader_check_reset_cause (void) {
    // if we are recovering from a WDT reset, trigger
    // a hibernate cycle for a clean boot
    // if (MAP_PRCMSysResetCauseGet() == PRCM_WDT_RESET) {
    //     HWREG(0x400F70B8) = 1;
    //     UtilsDelay(800000/5);
    //     HWREG(0x400F70B0) = 1;
    //     UtilsDelay(800000/5);

    //     HWREG(0x4402E16C) |= 0x2;
    //     UtilsDelay(800);
    //     HWREG(0x4402F024) &= 0xF7FFFFFF;

    //     // since the reset cause will be changed, we must store the right reason
    //     // so that the application knows it when booting for the next time
    //     PRCMSetSpecialBit(PRCM_WDT_RESET_BIT);

    //     MAP_PRCMHibernateWakeupSourceEnable(PRCM_HIB_SLOW_CLK_CTR);
    //     // set the sleep interval to 10ms
    //     MAP_PRCMHibernateIntervalSet(330);
    //     MAP_PRCMHibernateEnter();
    // }
}

void mperror_deinit_sfe_pin (void) {
    // disable the pull-down
    //MAP_PinConfigSet(MICROPY_SAFE_BOOT_PIN_NUM, PIN_STRENGTH_4MA, PIN_TYPE_STD);
}

void mperror_signal_error (void) {
    uint32_t count = 0;
    while ((MPERROR_TOOGLE_MS * count++) < MPERROR_SIGNAL_ERROR_MS) {
        // toogle the led
        // MAP_GPIOPinWrite(MICROPY_SYS_LED_PORT, MICROPY_SYS_LED_PORT_PIN, ~MAP_GPIOPinRead(MICROPY_SYS_LED_PORT, MICROPY_SYS_LED_PORT_PIN));
        // UtilsDelay(UTILS_DELAY_US_TO_COUNT(MPERROR_TOOGLE_MS * 1000));
    }
}

void mperror_heartbeat_switch_off (void) {
    if (mperror_heart_beat.enabled) {
        mperror_heart_beat.on_time = 0;
        mperror_heart_beat.off_time = 0;
        //MAP_GPIOPinWrite(MICROPY_SYS_LED_PORT, MICROPY_SYS_LED_PORT_PIN, 0);
    }
}

void mperror_heartbeat_signal (void) {
    if (mperror_heart_beat.do_disable) {
        mperror_heart_beat.do_disable = false;
    } else if (mperror_heart_beat.enabled) {
        // if (!mperror_heart_beat.beating) {
        //     if ((mperror_heart_beat.on_time = mp_hal_ticks_ms()) - mperror_heart_beat.off_time > MPERROR_HEARTBEAT_OFF_MS) {
        //         MAP_GPIOPinWrite(MICROPY_SYS_LED_PORT, MICROPY_SYS_LED_PORT_PIN, MICROPY_SYS_LED_PORT_PIN);
        //         mperror_heart_beat.beating = true;
        //     }
        // } else {
        //     if ((mperror_heart_beat.off_time = mp_hal_ticks_ms()) - mperror_heart_beat.on_time > MPERROR_HEARTBEAT_ON_MS) {
        //         MAP_GPIOPinWrite(MICROPY_SYS_LED_PORT, MICROPY_SYS_LED_PORT_PIN, 0);
        //         mperror_heart_beat.beating = false;
        //     }
        // }
    }
}

void NORETURN __fatal_error(const char *msg) {
    // signal the crash with the system led
//    MAP_GPIOPinWrite(MICROPY_SYS_LED_PORT, MICROPY_SYS_LED_PORT_PIN, MICROPY_SYS_LED_PORT_PIN);
    for ( ;; ) {__WFI();}
}

void __assert_func(const char *file, int line, const char *func, const char *expr) {
    (void) func;
    printf("Assertion failed: %s, file %s, line %d\n", expr, file, line);
    __fatal_error(NULL);
}

void nlr_jump_fail(void *val) {
    __fatal_error(NULL);
}

void mperror_enable_heartbeat (bool enable) {
    if (enable) {
        mperror_heart_beat.enabled = true;
        mperror_heart_beat.do_disable = false;
        //mperror_heartbeat_switch_off();
    } else {
        mperror_heart_beat.do_disable = true;
        mperror_heart_beat.enabled = false;
    }
}

bool mperror_is_heartbeat_enabled (void) {
    return mperror_heart_beat.enabled;
}
