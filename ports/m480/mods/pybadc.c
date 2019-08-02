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

#include "misc/bufhelper.h"

#include "chip/M480.h"

#include "mods/pybpin.h"
#include "mods/pybadc.h"


#define PYB_NUM_SAMPLES     16
#define PYB_NUM_CHANNELS    16


/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef enum {
    PYB_ADC_0     =  0,
    PYB_NUM_ADCS
} pyb_adc_id_t;


typedef struct {
    mp_obj_base_t base;
    pyb_adc_id_t adc_id;
    EADC_T *ADCx;

    uint16_t sw_samples;    // 软件触发采样
} pyb_adc_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_adc_obj_t pyb_adc_obj[PYB_NUM_ADCS] = {
    { .adc_id = PYB_ADC_0, .ADCx = EADC, .sw_samples = 0},
};


/******************************************************************************/
/* MicroPython bindings : adc object                                         */

STATIC void adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_adc_obj_t *self = self_in;

    mp_printf(print, "ADC(%u)", self->adc_id);
}


STATIC mp_obj_t adc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_id, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint adc_id = args[0].u_int;
    if(adc_id >= PYB_NUM_ADCS) {
        mp_raise_OSError(MP_ENODEV);
    }

    pyb_adc_obj_t *self = &pyb_adc_obj[adc_id];
    self->base.type = &pyb_adc_type;

    switch(self->adc_id) {
    case PYB_ADC_0:
        CLK_EnableModuleClock(EADC_MODULE);
        CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(16));    // EADC clock = 192MHz / 16 = 12MHz
        break;

    default:
        break;
    }

    EADC_Open(self->ADCx, EADC_CTL_DIFFEN_SINGLE_END);

    return self;
}


STATIC mp_obj_t pyb_adc_samp_config(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_sample, ARG_channel, ARG_trigger, ARG_trigger_pin, ARG_extend_sample_time, ARG_trigger_delay_time };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_sample,             MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_channel,            MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_trigger,            MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = EADC_SOFTWARE_TRIGGER} },
        { MP_QSTR_trigger_pin,        MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_extend_sample_time, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_trigger_delay_time, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 0} },
    };

    // parse args
    pyb_adc_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint sample = args[ARG_sample].u_int;
    if(sample >= PYB_NUM_SAMPLES)
    {
        goto invalid_args;
    }

    uint channel = args[ARG_channel].u_int;
    if(channel >= PYB_NUM_CHANNELS)
    {
        goto invalid_args;
    }

    uint trigger = args[ARG_trigger].u_int;
    if((trigger != EADC_SOFTWARE_TRIGGER) && (trigger != EADC_FALLING_EDGE_TRIGGER) && (trigger != EADC_RISING_EDGE_TRIGGER) && (trigger != EADC_FALLING_RISING_EDGE_TRIGGER) &&
       (trigger != EADC_TIMER0_TRIGGER) && (trigger != EADC_TIMER1_TRIGGER) && (trigger != EADC_TIMER2_TRIGGER) && (trigger != EADC_TIMER3_TRIGGER) &&
       (trigger != EADC_PWM0TG0_TRIGGER) && (trigger != EADC_PWM0TG1_TRIGGER) && (trigger != EADC_PWM0TG2_TRIGGER) && (trigger != EADC_PWM0TG3_TRIGGER) && (trigger != EADC_PWM0TG4_TRIGGER) && (trigger != EADC_PWM0TG5_TRIGGER) &&
       (trigger != EADC_PWM1TG0_TRIGGER) && (trigger != EADC_PWM1TG1_TRIGGER) && (trigger != EADC_PWM1TG2_TRIGGER) && (trigger != EADC_PWM1TG3_TRIGGER) && (trigger != EADC_PWM1TG4_TRIGGER) && (trigger != EADC_PWM1TG5_TRIGGER))
    {
        goto invalid_args;
    }

    uint extend_sample_time = args[ARG_extend_sample_time].u_int;
    if(extend_sample_time > 0xFF)
    {
        goto invalid_args;
    }

    uint trigger_delay_time = args[ARG_trigger_delay_time].u_int;
    if(trigger_delay_time > 0xFF*4)
    {
        goto invalid_args;
    }

    char pin_name[8] = {0}, af_name[32] = {0};
    snprintf(pin_name, 8, "PB%u", channel);
    snprintf(af_name, 16, "PB%u_EADC0_CH%u", channel, channel);
    pin_config_by_name(pin_name, af_name);

    if(args[ARG_trigger_pin].u_obj != mp_const_none)
    {
        pin_config_by_func(args[ARG_trigger_pin].u_obj, "%s_EADC0_ST", 0);
    }

    EADC_ConfigSampleModule(self->ADCx, sample, trigger, channel);
    if(trigger == EADC_SOFTWARE_TRIGGER) self->sw_samples |= (1 << sample);

    EADC_SetExtendSampleTime(self->ADCx, sample, extend_sample_time);
    EADC_SetTriggerDelayTime(self->ADCx, sample, trigger_delay_time, EADC_SCTL_TRGDLYDIV_DIVIDER_1);

    return mp_const_none;

invalid_args:
    mp_raise_ValueError("invalid argument(s) value");
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_adc_samp_config_obj, 2, pyb_adc_samp_config);


STATIC mp_obj_t pyb_adc_start(size_t n_args, const mp_obj_t *args) {
    pyb_adc_obj_t *self = args[0];

    uint samples = self->sw_samples;
    if(n_args == 2) samples = mp_obj_get_int(args[1]);

    EADC_START_CONV(self->ADCx, samples);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_adc_start_obj, 1, pyb_adc_start);


STATIC mp_obj_t pyb_adc_busy(mp_obj_t self_in) {
    pyb_adc_obj_t *self = self_in;

    return MP_OBJ_NEW_SMALL_INT(EADC_IS_BUSY(self->ADCx));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_adc_busy_obj, pyb_adc_busy);


STATIC mp_obj_t pyb_adc_read(size_t n_args, const mp_obj_t *args) {
    pyb_adc_obj_t *self = args[0];

    uint samples = 0xFFFF;
    if(n_args == 2) samples = mp_obj_get_int(args[1]);

    uint n = 0;
    mp_obj_t datas[PYB_NUM_SAMPLES];
    for(uint i = 0; i < PYB_NUM_SAMPLES; i++)
    {
        uint data = self->ADCx->DAT[i];
        if((samples & (1 << i)) && (data & EADC_DAT0_VALID_Msk))
        {
            mp_obj_t item[2] = { MP_OBJ_NEW_SMALL_INT(i), MP_OBJ_NEW_SMALL_INT(data & EADC_DAT0_RESULT_Msk) };

            datas[n++] = mp_obj_new_tuple(2, item);
        }
    }

    return mp_obj_new_tuple(n, datas);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_adc_read_obj, pyb_adc_read);


STATIC const mp_rom_map_elem_t adc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_samp_config),  MP_ROM_PTR(&pyb_adc_samp_config_obj) },
    { MP_ROM_QSTR(MP_QSTR_start),        MP_ROM_PTR(&pyb_adc_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_busy),         MP_ROM_PTR(&pyb_adc_busy_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),         MP_ROM_PTR(&pyb_adc_read_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_CHN_0),        MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_CHN_1),        MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_CHN_2),        MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_CHN_3),        MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_CHN_4),        MP_ROM_INT(4) },
    { MP_ROM_QSTR(MP_QSTR_CHN_5),        MP_ROM_INT(5) },
    { MP_ROM_QSTR(MP_QSTR_CHN_6),        MP_ROM_INT(6) },
    { MP_ROM_QSTR(MP_QSTR_CHN_7),        MP_ROM_INT(7) },
    { MP_ROM_QSTR(MP_QSTR_CHN_8),        MP_ROM_INT(8) },
    { MP_ROM_QSTR(MP_QSTR_CHN_9),        MP_ROM_INT(9) },
    { MP_ROM_QSTR(MP_QSTR_CHN_10),       MP_ROM_INT(10) },
    { MP_ROM_QSTR(MP_QSTR_CHN_11),       MP_ROM_INT(11) },
    { MP_ROM_QSTR(MP_QSTR_CHN_12),       MP_ROM_INT(12) },
    { MP_ROM_QSTR(MP_QSTR_CHN_13),       MP_ROM_INT(13) },
    { MP_ROM_QSTR(MP_QSTR_CHN_14),       MP_ROM_INT(14) },
    { MP_ROM_QSTR(MP_QSTR_CHN_15),       MP_ROM_INT(15) },

    { MP_ROM_QSTR(MP_QSTR_SAMP_0),       MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_SAMP_1),       MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_SAMP_2),       MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_SAMP_3),       MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_SAMP_4),       MP_ROM_INT(4) },
    { MP_ROM_QSTR(MP_QSTR_SAMP_5),       MP_ROM_INT(5) },
    { MP_ROM_QSTR(MP_QSTR_SAMP_6),       MP_ROM_INT(6) },
    { MP_ROM_QSTR(MP_QSTR_SAMP_7),       MP_ROM_INT(7) },
    { MP_ROM_QSTR(MP_QSTR_SAMP_8),       MP_ROM_INT(8) },
    { MP_ROM_QSTR(MP_QSTR_SAMP_9),       MP_ROM_INT(9) },
    { MP_ROM_QSTR(MP_QSTR_SAMP_10),      MP_ROM_INT(10) },
    { MP_ROM_QSTR(MP_QSTR_SAMP_11),      MP_ROM_INT(11) },
    { MP_ROM_QSTR(MP_QSTR_SAMP_12),      MP_ROM_INT(12) },
    { MP_ROM_QSTR(MP_QSTR_SAMP_13),      MP_ROM_INT(13) },
    { MP_ROM_QSTR(MP_QSTR_SAMP_14),      MP_ROM_INT(14) },
    { MP_ROM_QSTR(MP_QSTR_SAMP_15),      MP_ROM_INT(15) },

    { MP_ROM_QSTR(MP_QSTR_TRIG_SW),      MP_ROM_INT(EADC_SOFTWARE_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_FALLING), MP_ROM_INT(EADC_FALLING_EDGE_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_RISING),  MP_ROM_INT(EADC_RISING_EDGE_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_BOTH),    MP_ROM_INT(EADC_FALLING_RISING_EDGE_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER0),  MP_ROM_INT(EADC_TIMER0_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER1),  MP_ROM_INT(EADC_TIMER1_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER2),  MP_ROM_INT(EADC_TIMER2_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER3),  MP_ROM_INT(EADC_TIMER3_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER0),  MP_ROM_INT(EADC_TIMER0_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_PWM0TG0), MP_ROM_INT(EADC_PWM0TG0_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_PWM0TG1), MP_ROM_INT(EADC_PWM0TG1_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_PWM0TG2), MP_ROM_INT(EADC_PWM0TG2_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_PWM0TG3), MP_ROM_INT(EADC_PWM0TG3_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_PWM0TG4), MP_ROM_INT(EADC_PWM0TG4_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_PWM0TG5), MP_ROM_INT(EADC_PWM0TG5_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_PWM1TG0), MP_ROM_INT(EADC_PWM1TG0_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_PWM1TG1), MP_ROM_INT(EADC_PWM1TG1_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_PWM1TG2), MP_ROM_INT(EADC_PWM1TG2_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_PWM1TG3), MP_ROM_INT(EADC_PWM1TG3_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_PWM1TG4), MP_ROM_INT(EADC_PWM1TG4_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_PWM1TG5), MP_ROM_INT(EADC_PWM1TG5_TRIGGER) },
};

STATIC MP_DEFINE_CONST_DICT(adc_locals_dict, adc_locals_dict_table);


const mp_obj_type_t pyb_adc_type = {
    { &mp_type_type },
    .name = MP_QSTR_ADC,
    .print = adc_print,
    .make_new = adc_make_new,
    .locals_dict = (mp_obj_t)&adc_locals_dict,
};
