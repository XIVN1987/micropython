#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include "bufhelper.h"

#include "pybpin.h"
#include "pybpwm.h"


#define PWM_TRIG_TO_ADC   1


/// \moduleref pyb
/// \class PWM - Pulse-Width Modulation


/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef enum {
    PYB_PWM_0   =  0,
    PYB_PWM_1   =  1,
    PYB_PWM_2   =  2,
    PYB_PWM_3   =  3,
    PYB_PWM_4   =  4,
    PYB_PWM_5   =  5,
    PYB_NUM_PWMS
} pyb_pwm_id_t;

typedef struct _pyb_pwm_obj_t {
    mp_obj_base_t base;
    pyb_pwm_id_t pwm_id;
    PWM_TypeDef *PWMx;
    uint16_t period;
    uint16_t duty;
    uint16_t deadzone;

    uint8_t mode;
    uint8_t trigger;    // TO_ADC
} pyb_pwm_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_pwm_obj_t pyb_pwm_obj[PYB_NUM_PWMS] = {
    { {&pyb_pwm_type}, .pwm_id = PYB_PWM_0, .PWMx = PWM0 },
    { {&pyb_pwm_type}, .pwm_id = PYB_PWM_1, .PWMx = PWM1 },
    { {&pyb_pwm_type}, .pwm_id = PYB_PWM_2, .PWMx = PWM2 },
    { {&pyb_pwm_type}, .pwm_id = PYB_PWM_3, .PWMx = PWM3 },
    { {&pyb_pwm_type}, .pwm_id = PYB_PWM_4, .PWMx = PWM4 },
    { {&pyb_pwm_type}, .pwm_id = PYB_PWM_5, .PWMx = PWM5 },
};


/******************************************************************************/
/* MicroPython bindings                                                      */
/******************************************************************************/
STATIC void pwm_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_pwm_obj_t *self = self_in;

    if(self->mode == PWM_MODE_INDEP)
    {
        mp_printf(print, "PWM(%d, period=%u, duty=\%%d)", self->pwm_id, self->period, self->duty);
    }
    else
    {
        mp_printf(print, "PWM(%d, period=%u, duty=%d, deadzone=%d)", self->pwm_id, self->period, self->duty, self->deadzone);
    }
}


STATIC mp_obj_t pwm_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_id, ARG_period, ARG_duty, ARG_mode, ARG_deadzone, ARG_trigger, ARG_pin, ARG_pinN };
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 1} },
        { MP_QSTR_period,   MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 20000} },
        { MP_QSTR_duty,     MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 10000} },
        { MP_QSTR_mode,     MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = PWM_MODE_INDEP} },
        { MP_QSTR_deadzone, MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 0} },
        { MP_QSTR_trigger,  MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 0} },
        { MP_QSTR_pin,      MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
        { MP_QSTR_pinN,     MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint pwm_id = args[ARG_id].u_int;
    if(pwm_id >= PYB_NUM_PWMS)
    {
        mp_raise_OSError(MP_ENODEV);
    }
    pyb_pwm_obj_t *self = &pyb_pwm_obj[pwm_id];

    self->period   = args[ARG_period].u_int;
    self->duty     = args[ARG_duty].u_int;
    self->deadzone = args[ARG_deadzone].u_int;
    if((self->period > 65535) || (self->duty > self->period) || (self->deadzone > self->duty))
    {
        mp_raise_ValueError("period or duty or deadzone value invalid");
    }

    self->mode = args[ARG_mode].u_int;
    if(self->mode == PWM_MODE_INDEP)
    {
        if(args[ARG_pin].u_obj != mp_const_none)
        {
            pin_obj_t *pinA = pin_find_by_name(args[ARG_pin].u_obj);
            if((self->pwm_id == PYB_PWM_0) || (self->pwm_id == PYB_PWM_2) || (self->pwm_id == PYB_PWM_4))
            {
                if((pinA->pbit % 2) == 1)
                    mp_raise_ValueError("PWM0/2/4 pin need be Even number pin, like PA0, PA2, PA4");
            }
            else
            {
                if((pinA->pbit % 2) == 0)
                    mp_raise_ValueError("PWM1/3/5 pin need be Odd  number pin, like PA1, PA3, PA5");
            }

            switch(self->pwm_id)
            {
            case PYB_PWM_0:
                pin_config(pinA, FUNMUX0_PWM0A_OUT, 0, 0);
                break;

            case PYB_PWM_1:
                pin_config(pinA, FUNMUX1_PWM1A_OUT, 0, 0);
                break;

            case PYB_PWM_2:
                pin_config(pinA, FUNMUX0_PWM2A_OUT, 0, 0);
                break;

            case PYB_PWM_3:
                pin_config(pinA, FUNMUX1_PWM3A_OUT, 0, 0);
                break;

            case PYB_PWM_4:
                pin_config(pinA, FUNMUX0_PWM4A_OUT, 0, 0);
                break;

            case PYB_PWM_5:
                pin_config(pinA, FUNMUX1_PWM5A_OUT, 0, 0);
                break;
            }
        }
    }
    else
    {
        if((args[ARG_pin].u_obj != mp_const_none) && (args[ARG_pinN].u_obj != mp_const_none))
        {
            pin_obj_t *pinA = pin_find_by_name(args[ARG_pin].u_obj);
            pin_obj_t *pinB = pin_find_by_name(args[ARG_pinN].u_obj);
            if((self->pwm_id == PYB_PWM_0) || (self->pwm_id == PYB_PWM_2) || (self->pwm_id == PYB_PWM_4))
            {
                if(((pinA->pbit % 2) == 1) || ((pinB->pbit % 2) == 1))
                    mp_raise_ValueError("PWM0/2/4 pin need be Even number pin, like PA0, PA2, PA4");
            }
             else
            {
                if(((pinA->pbit % 2) == 0) || ((pinB->pbit % 2) == 0))
                    mp_raise_ValueError("PWM1/3/5 pin need be Odd  number pin, like PA1, PA3, PA5");
            }

            switch (self->pwm_id)
            {
            case PYB_PWM_0:
                pin_config(pinA, FUNMUX0_PWM0A_OUT, 0, 0);
                pin_config(pinB, FUNMUX0_PWM0B_OUT, 0, 0);
                break;

            case PYB_PWM_1:
                pin_config(pinA, FUNMUX1_PWM1A_OUT, 0, 0);
                pin_config(pinB, FUNMUX1_PWM1B_OUT, 0, 0);
                break;

            case PYB_PWM_2:
                pin_config(pinA, FUNMUX0_PWM2A_OUT, 0, 0);
                pin_config(pinB, FUNMUX0_PWM2B_OUT, 0, 0);
                break;

            case PYB_PWM_3:
                pin_config(pinA, FUNMUX1_PWM3A_OUT, 0, 0);
                pin_config(pinB, FUNMUX1_PWM3B_OUT, 0, 0);
                break;

            case PYB_PWM_4:
                pin_config(pinA, FUNMUX0_PWM4A_OUT, 0, 0);
                pin_config(pinB, FUNMUX0_PWM4B_OUT, 0, 0);
                break;

            case PYB_PWM_5:
                pin_config(pinA, FUNMUX1_PWM5A_OUT, 0, 0);
                pin_config(pinB, FUNMUX1_PWM5B_OUT, 0, 0);
                break;
            }
        }
    }

    PWM_InitStructure  PWM_initStruct;
    PWM_initStruct.clk_div = PWM_CLKDIV_8;		//F_PWM = 120M/8 = 15M
    PWM_initStruct.mode = self->mode;
    PWM_initStruct.cycleA = self->period;
    PWM_initStruct.hdutyA = self->duty;
    PWM_initStruct.deadzoneA  = self->deadzone;
    PWM_initStruct.initLevelA = 0;
    PWM_initStruct.cycleB     = 0;
    PWM_initStruct.hdutyB     = 0;
    PWM_initStruct.deadzoneB  = self->deadzone;
    PWM_initStruct.initLevelB = 0;
    PWM_initStruct.HEndAIEn = 0;
    PWM_initStruct.NCycleAIEn = 0;
    PWM_initStruct.HEndBIEn = 0;
    PWM_initStruct.NCycleBIEn = 0;
    PWM_Init(self->PWMx, &PWM_initStruct);

    self->trigger = args[ARG_trigger].u_int;
    if(self->trigger == PWM_TRIG_TO_ADC)
    {
        *(&PWMG->ADTRG0A + self->pwm_id * 2) = (1 << PWMG_ADTRG_EN_Pos) |
                  (0 << PWMG_ADTRG_EVEN_Pos) | (self->duty << PWMG_ADTRG_VALUE_Pos);
    }

    return self;
}

STATIC mp_obj_t pwm_start(mp_obj_t self_in)
{
    pyb_pwm_obj_t *self = self_in;

    PWM_Start(self->PWMx, 1, 0);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pwm_start_obj, pwm_start);


STATIC mp_obj_t pwm_stop(mp_obj_t self_in)
{
    pyb_pwm_obj_t *self = self_in;

    PWM_Stop(self->PWMx, 1, 0);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pwm_stop_obj, pwm_stop);


STATIC mp_obj_t pwm_period(size_t n_args, const mp_obj_t *args)
{
    pyb_pwm_obj_t *self = args[0];

    if(n_args == 1)
    {
        return MP_OBJ_NEW_SMALL_INT(self->period);
    }
    else
    {
        self->period = mp_obj_get_int(args[1]);
        PWM_SetCycle(self->PWMx, PWM_CH_A, self->period);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pwm_period_obj, 1, 2, pwm_period);


STATIC mp_obj_t pwm_duty(size_t n_args, const mp_obj_t *args)
{
    pyb_pwm_obj_t *self = args[0];

    if(n_args == 1)
    {
        return MP_OBJ_NEW_SMALL_INT(self->duty);
    }
    else
    {
        self->duty = mp_obj_get_int(args[1]);
        PWM_SetHDuty(self->PWMx, PWM_CH_A, self->duty);

        if(self->trigger == PWM_TRIG_TO_ADC)
        {
            *(&PWMG->ADTRG0A + self->pwm_id * 2) = (1 << PWMG_ADTRG_EN_Pos) |
                      (0 << PWMG_ADTRG_EVEN_Pos) | (self->duty << PWMG_ADTRG_VALUE_Pos);
        }

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pwm_duty_obj, 1, 2, pwm_duty);


STATIC mp_obj_t pwm_deadzone(size_t n_args, const mp_obj_t *args)
{
    pyb_pwm_obj_t *self = args[0];

    if(n_args == 1)
    {
        return MP_OBJ_NEW_SMALL_INT(self->deadzone);
    }
    else
    {
        self->deadzone = mp_obj_get_int(args[1]);
        PWM_SetDeadzone(self->PWMx, PWM_CH_A, self->deadzone);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pwm_deadzone_obj, 1, 2, pwm_deadzone);


STATIC const mp_rom_map_elem_t pwm_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_start),         MP_ROM_PTR(&pwm_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop),          MP_ROM_PTR(&pwm_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_period),        MP_ROM_PTR(&pwm_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_duty),          MP_ROM_PTR(&pwm_duty_obj) },
    { MP_ROM_QSTR(MP_QSTR_deadzone),      MP_ROM_PTR(&pwm_deadzone_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_MODE_INDEP),    MP_ROM_INT(PWM_MODE_INDEP) },
    { MP_ROM_QSTR(MP_QSTR_MODE_COMPL),    MP_ROM_INT(PWM_MODE_COMPL_CALIGN) },

    { MP_ROM_QSTR(MP_QSTR_TO_ADC),        MP_ROM_INT(PWM_TRIG_TO_ADC) },
};
STATIC MP_DEFINE_CONST_DICT(pwm_locals_dict, pwm_locals_dict_table);


const mp_obj_type_t pyb_pwm_type = {
    { &mp_type_type },
    .name = MP_QSTR_PWM,
    .print = pwm_print,
    .make_new = pwm_make_new,
    .locals_dict = (mp_obj_t)&pwm_locals_dict,
};
