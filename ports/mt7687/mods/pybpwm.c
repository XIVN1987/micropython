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
#include <string.h>

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include "bufhelper.h"

#include "pybpin.h"
#include "pybpwm.h"

/// \moduleref pyb
/// \class PWM - Pulse-Width Modulation

typedef enum {
    PYB_PWM_0  =  0,
    PYB_PWM_1  =  1,
    PYB_PWM_2  =  2,
    PYB_PWM_3  =  3,
    PYB_PWM_4  =  4,
    PYB_PWM_5  =  5,
    PYB_PWM_6  =  6,
    PYB_PWM_7  =  7,
    PYB_PWM_8  =  8,
    PYB_PWM_9  =  9,
    PYB_PWM_10 =  10,
    PYB_PWM_11 =  11,
    PYB_PWM_12 =  12,
    PYB_PWM_13 =  13,
    PYB_PWM_14 =  14,
    PYB_PWM_15 =  15,
    PYB_PWM_16 =  16,
    PYB_PWM_17 =  17,
    PYB_PWM_18 =  18,
    PYB_PWM_19 =  19,
    PYB_PWM_20 =  20,
    PYB_PWM_21 =  21,
    PYB_PWM_22 =  22,
    PYB_PWM_23 =  23,
    PYB_PWM_24 =  24,
    PYB_PWM_25 =  25,
    PYB_PWM_26 =  26,
    PYB_PWM_27 =  27,
    PYB_PWM_28 =  28,
    PYB_PWM_29 =  29,
    PYB_PWM_30 =  30,
    PYB_PWM_31 =  31,
    PYB_PWM_32 =  32,
    PYB_PWM_33 =  33,
    PYB_PWM_34 =  34,
    PYB_PWM_35 =  35,
    PYB_PWM_36 =  36,
    PYB_PWM_37 =  37,
    PYB_PWM_38 =  38,
    PYB_PWM_39 =  39,
    PYB_NUM_PWMS
} pyb_pwm_id_t;

typedef struct _pyb_pwm_obj_t {
    mp_obj_base_t base;
    pyb_pwm_id_t pwm_id;
    PWM_TypeDef *PWMx;
    uint frequency;
    uint duty;
    bool global_start;
} pyb_pwm_obj_t;

/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_pwm_obj_t pyb_pwm_obj[PYB_NUM_PWMS] = { { .pwm_id = PYB_PWM_0,  .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_1,  .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_2,  .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_3,  .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_4,  .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_5,  .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_6,  .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_7,  .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_8,  .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_9,  .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_10, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_11, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_12, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_13, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_14, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_15, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_16, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_17, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_18, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_19, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_20, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_21, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_22, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_23, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_24, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_25, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_26, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_27, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_28, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_29, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_30, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_31, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_32, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_33, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_34, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_35, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_36, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_37, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_38, .PWMx = PWM },
                                                   { .pwm_id = PYB_PWM_39, .PWMx = PWM }};

/******************************************************************************
 DECLARE PRIVATE FUNCTIONS
 ******************************************************************************/

/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/


/******************************************************************************/
/* MicroPython bindings                                                      */
/******************************************************************************/
STATIC void pyb_pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_pwm_obj_t *self = self_in;

    mp_printf(print, "PWM(%d, frequency=%d, duty=%%%d.%d, global_start=%s)", self->pwm_id, self->frequency, self->duty/10, self->duty%10, self->global_start ? "True" : "False");
}

STATIC const mp_arg_t pyb_pwm_init_args[] = {
    { MP_QSTR_id,        MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 1} },
    { MP_QSTR_frequency, MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 1000} },
    { MP_QSTR_duty,      MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 250} },
    { MP_QSTR_global_start,                MP_ARG_BOOL, {.u_bool= false} },
};
STATIC mp_obj_t pyb_pwm_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_pwm_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(args), pyb_pwm_init_args, args);

    uint pwm_id = args[0].u_int;
    if (pwm_id > PYB_NUM_PWMS-1) {
        mp_raise_OSError(MP_ENODEV);
    }

    pyb_pwm_obj_t *self = &pyb_pwm_obj[pwm_id];
    self->base.type = &pyb_pwm_type;

    self->frequency = args[1].u_int;
    self->duty = args[2].u_int;
    if((self->frequency > 100000) || (self->frequency < 2000000/65536) || (self->duty > 1000))
    {
        goto invalid_args;
    }

    self->global_start = args[3].u_bool;

    switch (self->pwm_id) {
    case PYB_PWM_0:  PORT_Init(GPIO0,  GPIO0_PWM0);    break;
    case PYB_PWM_1:  PORT_Init(GPIO1,  GPIO1_PWM1);    break;
    case PYB_PWM_2:  PORT_Init(GPIO4,  GPIO4_PWM2);    break;
    case PYB_PWM_3:  PORT_Init(GPIO5,  GPIO5_PWM3);    break;
    case PYB_PWM_4:  PORT_Init(GPIO6,  GPIO6_PWM4);    break;
    case PYB_PWM_5:  PORT_Init(GPIO7,  GPIO7_PWM5);    break;
    case PYB_PWM_6:                                    break;
    case PYB_PWM_7:                                    break;
    case PYB_PWM_8:                                    break;
    case PYB_PWM_9:                                    break;
    case PYB_PWM_10:                                   break;
    case PYB_PWM_11:                                   break;
    case PYB_PWM_12:                                   break;
    case PYB_PWM_13:                                   break;
    case PYB_PWM_14:                                   break;
    case PYB_PWM_15:                                   break;
    case PYB_PWM_16:                                   break;
    case PYB_PWM_17:                                   break;
    case PYB_PWM_18: PORT_Init(GPIO35, GPIO35_PWM18);  break;
    case PYB_PWM_19: PORT_Init(GPIO36, GPIO36_PWM19);  break;
    case PYB_PWM_20: PORT_Init(GPIO37, GPIO37_PWM20);  break;
    case PYB_PWM_21: PORT_Init(GPIO38, GPIO38_PWM21);  break;
    case PYB_PWM_22: PORT_Init(GPIO39, GPIO39_PWM22);  break;
    case PYB_PWM_23: PORT_Init(GPIO2,  GPIO2_PWM23);   break;
    case PYB_PWM_24: PORT_Init(GPIO3,  GPIO3_PWM24);   break;
    case PYB_PWM_25: PORT_Init(GPIO24, GPIO24_PWM25);  break;
    case PYB_PWM_26: PORT_Init(GPIO25, GPIO25_PWM26);  break;
    case PYB_PWM_27: PORT_Init(GPIO26, GPIO26_PWM27);  break;
    case PYB_PWM_28: PORT_Init(GPIO27, GPIO27_PWM28);  break;
    case PYB_PWM_29: PORT_Init(GPIO28, GPIO28_PWM29);  break;
    case PYB_PWM_30: PORT_Init(GPIO29, GPIO29_PWM30);  break;
    case PYB_PWM_31: PORT_Init(GPIO30, GPIO30_PWM31);  break;
    case PYB_PWM_32: PORT_Init(GPIO31, GPIO31_PWM32);  break;
    case PYB_PWM_33: PORT_Init(GPIO32, GPIO32_PWM33);  break;
    case PYB_PWM_34: PORT_Init(GPIO33, GPIO33_PWM34);  break;
    case PYB_PWM_35: PORT_Init(GPIO34, GPIO34_PWM35);  break;
    case PYB_PWM_36: PORT_Init(GPIO57, GPIO57_PWM36);  break;
    case PYB_PWM_37: PORT_Init(GPIO58, GPIO58_PWM37);  break;
    case PYB_PWM_38: PORT_Init(GPIO59, GPIO59_PWM38);  break;
    case PYB_PWM_39: PORT_Init(GPIO60, GPIO60_PWM39);  break;
    default:  break;
    }

    PWM_ClockSelect(PWM_CLKSRC_2MHz);

    uint16_t on_time  = (2000000/self->frequency) * self->duty / 1000,
             off_time = (2000000/self->frequency) * (1000 - self->duty) / 1000;
    PWM_Init(self->pwm_id, on_time, off_time, self->global_start);

    return self;

invalid_args:
    mp_raise_ValueError("invalid argument(s) value");
}

STATIC mp_obj_t pyb_pwm_start(mp_obj_t self_in) {
    pyb_pwm_obj_t *self = self_in;

    PWM_Start(self->pwm_id);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_pwm_start_obj, pyb_pwm_start);

STATIC mp_obj_t pyb_pwm_stop(mp_obj_t self_in) {
    pyb_pwm_obj_t *self = self_in;

    PWM_Stop(self->pwm_id);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_pwm_stop_obj, pyb_pwm_stop);

STATIC mp_obj_t pyb_pwm_gstart(mp_obj_t self_in) {
    pyb_pwm_obj_t *self = self_in;
    self = self;

    PWM_GStart();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_pwm_gstart_obj, pyb_pwm_gstart);

STATIC mp_obj_t pyb_pwm_gstop(mp_obj_t self_in) {
    pyb_pwm_obj_t *self = self_in;
    self = self;

    PWM_GStop();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_pwm_gstop_obj, pyb_pwm_gstop);

STATIC mp_obj_t pyb_pwm_frequency(size_t n_args, const mp_obj_t *args) {
    pyb_pwm_obj_t *self = args[0];
    if (n_args == 1) {
        return MP_OBJ_NEW_SMALL_INT(self->frequency);
    } else {
        self->frequency = mp_obj_get_int(args[1]);

        uint16_t on_time  = (2000000/self->frequency) * self->duty / 1000,
                 off_time = (2000000/self->frequency) * (1000 - self->duty) / 1000;
        PWM_SetOnTime(self->pwm_id, on_time);
        PWM_SetOffTime(self->pwm_id, off_time);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_pwm_frequency_obj, 1, 2, pyb_pwm_frequency);

STATIC mp_obj_t pyb_pwm_duty(size_t n_args, const mp_obj_t *args) {
    pyb_pwm_obj_t *self = args[0];
    if (n_args == 1) {
        return MP_OBJ_NEW_SMALL_INT(self->duty);
    } else {
        self->duty = mp_obj_get_int(args[1]);

        uint16_t on_time  = (2000000/self->frequency) * self->duty / 1000,
                 off_time = (2000000/self->frequency) * (1000 - self->duty) / 1000;
        PWM_SetOnTime(self->pwm_id, on_time);
        PWM_SetOffTime(self->pwm_id, off_time);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_pwm_duty_obj, 1, 2, pyb_pwm_duty);


STATIC const mp_rom_map_elem_t pyb_pwm_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_start),               MP_ROM_PTR(&pyb_pwm_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop),                MP_ROM_PTR(&pyb_pwm_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_gstart),              MP_ROM_PTR(&pyb_pwm_gstart_obj) },
    { MP_ROM_QSTR(MP_QSTR_gstop),               MP_ROM_PTR(&pyb_pwm_gstop_obj) },
    { MP_ROM_QSTR(MP_QSTR_frequency),           MP_ROM_PTR(&pyb_pwm_frequency_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty),                MP_ROM_PTR(&pyb_pwm_duty_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_CLKSRC_32KHz),        MP_ROM_INT(PWM_CLKSRC_32KHz) },
    { MP_ROM_QSTR(MP_QSTR_CLKSRC_2MHz),         MP_ROM_INT(PWM_CLKSRC_2MHz) },
    { MP_ROM_QSTR(MP_QSTR_CLKSRC_XTAL),         MP_ROM_INT(PWM_CLKSRC_XTAL) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_pwm_locals_dict, pyb_pwm_locals_dict_table);

const mp_obj_type_t pyb_pwm_type = {
    { &mp_type_type },
    .name = MP_QSTR_PWM,
    .print = pyb_pwm_print,
    .make_new = pyb_pwm_make_new,
    .locals_dict = (mp_obj_t)&pyb_pwm_locals_dict,
};
