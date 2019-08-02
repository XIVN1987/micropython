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

#include "chip/M480.h"

#include "mods/pybpin.h"
#include "mods/pybpwm.h"


#define PYB_NUM_CHANNELS    6


/// \moduleref pyb
/// \class PWM - Pulse-Width Modulation

typedef enum {
    PYB_BPWM_0    =  0,
    PYB_BPWM_1    =  1,
    PYB_EPWM_0    =  2,
    PYB_EPWM_1    =  3,
    PYB_NUM_PWMS
} pyb_pwm_id_t;


typedef struct _pyb_pwm_obj_t {
    mp_obj_base_t base;
    pyb_pwm_id_t pwm_id;
    union {
        BPWM_T *BPWMx;
        EPWM_T *EPWMx;
    };
    uint16_t clkdiv;
    union {
        uint16_t Bperiod;
        uint16_t Eperiod[PYB_NUM_CHANNELS];
    };
    uint16_t duty[PYB_NUM_CHANNELS];
    uint16_t chn_enabled;

    bool complementary[PYB_NUM_CHANNELS];
    uint16_t deadzone[PYB_NUM_CHANNELS];
} pyb_pwm_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_pwm_obj_t pyb_pwm_obj[PYB_NUM_PWMS] = {
    { .pwm_id = PYB_BPWM_0, .BPWMx = BPWM0 },
    { .pwm_id = PYB_BPWM_1, .BPWMx = BPWM1 },
    { .pwm_id = PYB_EPWM_0, .EPWMx = EPWM0 },
    { .pwm_id = PYB_EPWM_1, .EPWMx = EPWM1 },
};


/******************************************************************************/
/* MicroPython bindings                                                      */
/******************************************************************************/
STATIC void pyb_pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_pwm_obj_t *self = self_in;

    if(self->pwm_id < PYB_EPWM_0)
    {
        mp_printf(print, "PWM(%u, clkdiv=%u, period=%u)", self->pwm_id, self->clkdiv, self->Bperiod);
        for(uint i = 0; i < PYB_NUM_CHANNELS; i++)
        {
            if(self->chn_enabled & (1 << i))
            {
                mp_printf(print, "    CH%u(duty=%u", i, self->duty[i]);
            }
        }
    }
    else
    {
        mp_printf(print, "PWM(%u, clkdiv=%u)", self->pwm_id, self->clkdiv);
        for(uint i = 0; i < PYB_NUM_CHANNELS; i++)
        {
            if(self->chn_enabled & (1 << i))
            {
                if(self->complementary[i])
                {
                    mp_printf(print, "    CH%u(period=%u, duty=%u, deadzone=%u", i, self->Eperiod[i], self->duty[i], self->deadzone[i]);
                    i += 1;
                }
                else
                {
                    mp_printf(print, "    CH%u(period=%u, duty=%u", i, self->Eperiod[i], self->duty[i]);
                }
            }
        }
    }
}


STATIC mp_obj_t pyb_pwm_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_clkdiv, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 192} },
        { MP_QSTR_period, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 1000} },
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint pwm_id = args[0].u_int;
    if(pwm_id >= PYB_NUM_PWMS) {
        mp_raise_OSError(MP_ENODEV);
    }

    pyb_pwm_obj_t *self = &pyb_pwm_obj[pwm_id];
    self->base.type = &pyb_pwm_type;

    self->clkdiv = args[1].u_int;
    if(self->clkdiv > 0x10000)
    {
        goto invalid_args;
    }

    self->Bperiod = args[2].u_int;
    if(self->Bperiod > 0x10000)
    {
        goto invalid_args;
    }

    switch(self->pwm_id) {
    case PYB_BPWM_0:
        CLK_EnableModuleClock(BPWM0_MODULE);
        CLK_SetModuleClock(BPWM0_MODULE, CLK_CLKSEL2_BPWM0SEL_PCLK0, 0);
        break;

    case PYB_BPWM_1:
        CLK_EnableModuleClock(BPWM1_MODULE);
        CLK_SetModuleClock(BPWM1_MODULE, CLK_CLKSEL2_BPWM1SEL_PCLK1, 0);
        break;

    case PYB_EPWM_0:
        CLK_EnableModuleClock(EPWM0_MODULE);
        CLK_SetModuleClock(EPWM0_MODULE, CLK_CLKSEL2_EPWM0SEL_PCLK0, 0);
        break;

    case PYB_EPWM_1:
        CLK_EnableModuleClock(EPWM1_MODULE);
        CLK_SetModuleClock(EPWM1_MODULE, CLK_CLKSEL2_EPWM1SEL_PCLK1, 0);
        break;

    default:
        break;
    }

    if(self->pwm_id < PYB_EPWM_0)
    {
        self->BPWMx->CTL1 = BPWM_UP_COUNTER;

        self->BPWMx->CLKSRC = BPWM_CLKSRC_BPWM_CLK;
        self->BPWMx->CLKPSC = self->clkdiv;

        self->BPWMx->PERIOD = self->Bperiod;

        BPWM_SET_OUTPUT_LEVEL(self->BPWMx, 0x3F, BPWM_OUTPUT_NOTHING, BPWM_OUTPUT_LOW, BPWM_OUTPUT_HIGH, BPWM_OUTPUT_NOTHING);
    }
    else
    {
        self->EPWMx->CTL1 = (EPWM_UP_COUNTER << EPWM_CTL1_CNTTYPE0_Pos) | (EPWM_UP_COUNTER << EPWM_CTL1_CNTTYPE1_Pos) | (EPWM_UP_COUNTER << EPWM_CTL1_CNTTYPE2_Pos) |
                            (EPWM_UP_COUNTER << EPWM_CTL1_CNTTYPE3_Pos) | (EPWM_UP_COUNTER << EPWM_CTL1_CNTTYPE4_Pos) | (EPWM_UP_COUNTER << EPWM_CTL1_CNTTYPE5_Pos);

        self->EPWMx->CLKSRC = (EPWM_CLKSRC_EPWM_CLK << EPWM_CLKSRC_ECLKSRC0_Pos) | (EPWM_CLKSRC_EPWM_CLK << EPWM_CLKSRC_ECLKSRC2_Pos) | (EPWM_CLKSRC_EPWM_CLK << EPWM_CLKSRC_ECLKSRC4_Pos);
        self->EPWMx->CLKPSC[0] = self->clkdiv;
        self->EPWMx->CLKPSC[1] = self->clkdiv;
        self->EPWMx->CLKPSC[2] = self->clkdiv;

        EPWM_SET_OUTPUT_LEVEL(self->EPWMx, 0x3F, EPWM_OUTPUT_NOTHING, EPWM_OUTPUT_LOW, EPWM_OUTPUT_HIGH, EPWM_OUTPUT_NOTHING);
    }

    return self;

invalid_args:
    mp_raise_ValueError("invalid argument(s) value");
}


STATIC mp_obj_t pyb_pwm_chn_config(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_chn, ARG_duty, ARG_period, ARG_complementary, ARG_deadzone, ARG_pin, ARG_pinN };
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_chn,           MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_duty,          MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 500} },
        { MP_QSTR_period,        MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 1000} },
        { MP_QSTR_complementary, MP_ARG_KW_ONLY  | MP_ARG_BOOL,{.u_bool=false} },
        { MP_QSTR_deadzone,      MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_pin,           MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_pinN,          MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    pyb_pwm_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint chn = args[ARG_chn].u_int;
    if(chn > PYB_NUM_CHANNELS)
    {
        goto invalid_args;
    }

    if(self->pwm_id < PYB_EPWM_0)
    {
        self->duty[chn] = args[ARG_duty].u_int;
        BPWM_SET_CMR(self->BPWMx, chn, self->duty[chn]);

        BPWM_EnableOutput(self->BPWMx, 1 << chn);

        if(args[ARG_pin].u_obj != mp_const_none)
        {
            if(self->pwm_id == PYB_BPWM_0)
                pin_config_by_func(args[ARG_pin].u_obj, "%s_BPWM0_CH%u", chn);
            else
                pin_config_by_func(args[ARG_pin].u_obj, "%s_BPWM1_CH%u", chn);
        }
    }
    else
    {
        self->Eperiod[chn] = args[ARG_period].u_int;
        EPWM_SET_CNR(self->EPWMx, chn, self->Eperiod[chn]);

        self->duty[chn] = args[ARG_duty].u_int;
        EPWM_SET_CMR(self->EPWMx, chn, self->duty[chn]);

        EPWM_EnableOutput(self->EPWMx, (1 << chn));

        if(args[ARG_pin].u_obj != mp_const_none)
        {
            if(self->pwm_id == PYB_EPWM_0)
                pin_config_by_func(args[ARG_pin].u_obj, "%s_EPWM0_CH%u", chn);
            else
                pin_config_by_func(args[ARG_pin].u_obj, "%s_EPWM1_CH%u", chn);
        }

        self->complementary[chn] = args[ARG_complementary].u_bool;
        self->deadzone[chn] = args[ARG_deadzone].u_int;

        if(self->complementary[chn])
        {
            switch(chn) {
            case 0: self->EPWMx->CTL1 |= (1 << EPWM_CTL1_OUTMODE0_Pos); break;
            case 2: self->EPWMx->CTL1 |= (1 << EPWM_CTL1_OUTMODE2_Pos); break;
            case 4: self->EPWMx->CTL1 |= (1 << EPWM_CTL1_OUTMODE4_Pos); break;
            default:
                break;
            }

            EPWM_EnableOutput(self->EPWMx, (1 << (chn + 1)));

            EPWM_SET_DEADZONE_CLK_SRC(self->EPWMx, chn, true);
            EPWM_EnableDeadZone(self->EPWMx, chn, self->deadzone[chn]);

            if(args[ARG_pinN].u_obj != mp_const_none)
            {
                if(self->pwm_id == PYB_EPWM_0)
                    pin_config_by_func(args[ARG_pinN].u_obj, "%s_EPWM0_CH%u", chn+1);
                else
                    pin_config_by_func(args[ARG_pinN].u_obj, "%s_EPWM1_CH%u", chn+1);
            }
        }
    }

    return mp_const_none;

invalid_args:
    mp_raise_ValueError("invalid argument(s) value");
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_pwm_chn_config_obj, 2, pyb_pwm_chn_config);


STATIC mp_obj_t pyb_pwm_start(size_t n_args, const mp_obj_t *args) {
    pyb_pwm_obj_t *self = args[0];

    if(self->pwm_id < PYB_EPWM_0)
    {
        self->BPWMx->CNTEN = 1;
    }
    else
    {
        if(n_args == 1)
        {
            self->EPWMx->CNTEN = self->chn_enabled;
        }
        else
        {
            self->EPWMx->CNTEN = mp_obj_get_int(args[1]);
        }
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_pwm_start_obj, 1, 2, pyb_pwm_start);


STATIC mp_obj_t pyb_pwm_stop(size_t n_args, const mp_obj_t *args) {
    pyb_pwm_obj_t *self = args[0];

    if(self->pwm_id < PYB_EPWM_0)
    {
        self->BPWMx->CNTEN = 0;
    }
    else
    {
        if(n_args == 1)
        {
            self->EPWMx->CNTEN &= ~self->chn_enabled;
        }
        else
        {
            self->EPWMx->CNTEN &= ~mp_obj_get_int(args[1]);
        }
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_pwm_stop_obj, 1, 2, pyb_pwm_stop);


STATIC mp_obj_t pyb_pwm_period(size_t n_args, const mp_obj_t *args) {
    pyb_pwm_obj_t *self = args[0];

    if(self->pwm_id < PYB_EPWM_0)
    {
        if(n_args == 1)
        {
            return MP_OBJ_NEW_SMALL_INT(self->Bperiod);
        }
        else // if(n_args == 2)
        {
            self->Bperiod = mp_obj_get_int(args[1]);

            self->BPWMx->PERIOD = self->Bperiod;
        }
    }
    else
    {
        uint chn = mp_obj_get_int(args[1]);

        if(n_args == 2)
        {
            return MP_OBJ_NEW_SMALL_INT(self->Eperiod[chn]);
        }
        else // if(n_args == 3)
        {
            self->Eperiod[chn] = mp_obj_get_int(args[2]);

            self->EPWMx->PERIOD[chn] = self->Eperiod[chn];
        }
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_pwm_period_obj, 1, 3, pyb_pwm_period);


STATIC mp_obj_t pyb_pwm_duty(size_t n_args, const mp_obj_t *args) {
    pyb_pwm_obj_t *self = args[0];

    uint chn = mp_obj_get_int(args[1]);

    if(n_args == 2)
    {
        return MP_OBJ_NEW_SMALL_INT(self->duty[chn]);
    }
    else // if(n_args == 3)
    {
        self->duty[chn] = mp_obj_get_int(args[2]);

        self->BPWMx->CMPDAT[chn] = self->duty[chn];

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_pwm_duty_obj, 2, 3, pyb_pwm_duty);


STATIC mp_obj_t pyb_pwm_deadzone(size_t n_args, const mp_obj_t *args) {
    pyb_pwm_obj_t *self = args[0];

    if(self->pwm_id < PYB_EPWM_0) return mp_const_none;

    uint chn = mp_obj_get_int(args[1]);

    if(n_args == 2)
    {
        return MP_OBJ_NEW_SMALL_INT(self->deadzone[chn]);
    }
    else // if(n_args == 3)
    {
        self->deadzone[chn] = mp_obj_get_int(args[2]);

        EPWM_EnableDeadZone(self->EPWMx, chn, self->deadzone[chn]);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_pwm_deadzone_obj, 2, 3, pyb_pwm_deadzone);


STATIC const mp_rom_map_elem_t pyb_pwm_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_chn_config),          MP_ROM_PTR(&pyb_pwm_chn_config_obj) },
    { MP_ROM_QSTR(MP_QSTR_start),               MP_ROM_PTR(&pyb_pwm_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop),                MP_ROM_PTR(&pyb_pwm_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_period),              MP_ROM_PTR(&pyb_pwm_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty),                MP_ROM_PTR(&pyb_pwm_duty_obj) },
    { MP_ROM_QSTR(MP_QSTR_deadzone),            MP_ROM_PTR(&pyb_pwm_deadzone_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_CH_0),                MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_CH_1),                MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_CH_2),                MP_ROM_INT(2) },
    { MP_ROM_QSTR(MP_QSTR_CH_3),                MP_ROM_INT(3) },
    { MP_ROM_QSTR(MP_QSTR_CH_4),                MP_ROM_INT(4) },
    { MP_ROM_QSTR(MP_QSTR_CH_5),                MP_ROM_INT(5) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_pwm_locals_dict, pyb_pwm_locals_dict_table);


const mp_obj_type_t pyb_pwm_type = {
    { &mp_type_type },
    .name = MP_QSTR_PWM,
    .print = pyb_pwm_print,
    .make_new = pyb_pwm_make_new,
    .locals_dict = (mp_obj_t)&pyb_pwm_locals_dict,
};
