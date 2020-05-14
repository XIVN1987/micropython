#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include "pybpulse.h"
#include "pybpin.h"


#define TIMR_IRQ_CAPTURE   2


/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef enum {
    PYB_PULSE_0   =  0,
    PYB_NUM_PULSES
} pyb_pulse_id_t;

typedef struct {
    mp_obj_base_t base;
    pyb_pulse_id_t pulse_id;
    TIMRG_TypeDef *TIMRx;
    uint32_t pulse;

    uint8_t  IRQn;
    uint8_t  irq_flags;         // 中断标志
    uint8_t  irq_trigger;
    uint8_t  irq_priority;      // 中断优先级
    mp_obj_t irq_callback;      // 中断处理函数
} pyb_pulse_obj_t;


/******************************************************************************
 DEFINE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_pulse_obj_t pyb_pulse_obj[PYB_NUM_PULSES] = {
    { {&pyb_pulse_type}, .pulse_id = PYB_PULSE_0, .TIMRx = TIMRG, .IRQn = PULSE_IRQn },
};


/******************************************************************************/
/* MicroPython bindings                                                      */

STATIC void pulse_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_pulse_obj_t *self = self_in;

    mp_printf(print, "Pulse(%u, pulse='%s')", self->pulse_id, self->pulse == PULSE_LOW ? "low" : "high");
}


STATIC mp_obj_t pulse_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_id, ARG_pulse, ARG_irq, ARG_callback,  ARG_priority, ARG_pin };
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,        MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_pulse,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = PULSE_HIGH} },
        { MP_QSTR_irq,       MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_callback,  MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_priority,  MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 3} },
        { MP_QSTR_pin,       MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(args), allowed_args, args);

    uint pulse_id = args[ARG_id].u_int;
    if(pulse_id >= PYB_NUM_PULSES)
    {
        mp_raise_OSError(MP_ENODEV);
    }
    pyb_pulse_obj_t *self = &pyb_pulse_obj[pulse_id];

    self->pulse = args[ARG_pulse].u_int;
    if((self->pulse != PULSE_LOW) && (self->pulse != PULSE_HIGH))
    {
        mp_raise_ValueError("invalid pulse value");
    }

    if(args[ARG_pin].u_obj != mp_const_none)
    {
        pin_obj_t * pin = pin_find_by_name(args[ARG_pin].u_obj);

        if((pin->pbit % 2) == 0)
            mp_raise_ValueError("Pulse IN need be Odd number pin, like PA1, PA2, PA5");
        pin_config(pin, FUNMUX1_PULSE_IN, 0, 0);
    }

    self->irq_trigger  = args[ARG_irq].u_int;
    self->irq_callback = args[ARG_callback].u_obj;
    self->irq_priority = args[ARG_priority].u_int;
    if(self->irq_priority > 7)
    {
        mp_raise_ValueError("invalid priority value");
    }
    else
    {
        NVIC_SetPriority(self->IRQn, self->irq_priority);
    }

    Pulse_Init(self->pulse, self->irq_trigger & TIMR_IRQ_CAPTURE);

    return self;
}


STATIC mp_obj_t pulse_start(size_t n_args, const mp_obj_t *args)
{
    pyb_pulse_obj_t *self = args[0];

    if(n_args == 2)
    {
        self->pulse = mp_obj_get_int(args[1]);

        self->TIMRx->PCTRL &= ~TIMRG_PCTRL_HIGH_Msk;
        self->TIMRx->PCTRL |= (self->pulse << TIMRG_PCTRL_HIGH_Pos);
    }

    Pulse_Start();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pulse_start_obj, 1, 2, pulse_start);


STATIC mp_obj_t pulse_done(mp_obj_t self_in)
{
    pyb_pulse_obj_t *self = self_in;

    return mp_obj_new_int(Pulse_Done());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pulse_done_obj, pulse_done);


STATIC mp_obj_t pulse_value(mp_obj_t self_in)
{
    pyb_pulse_obj_t *self = self_in;

    return mp_obj_new_int(self->TIMRx->PCVAL);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pulse_value_obj, pulse_value);


STATIC mp_obj_t pulse_irq_flags(mp_obj_t self_in)
{
    pyb_pulse_obj_t *self = self_in;

    return mp_obj_new_int(self->irq_flags);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pulse_irq_flags_obj, pulse_irq_flags);


STATIC mp_obj_t pulse_irq_enable(mp_obj_t self_in, mp_obj_t irq_trigger)
{
    pyb_pulse_obj_t *self = self_in;

    uint trigger = mp_obj_get_int(irq_trigger);

    if(trigger & TIMR_IRQ_CAPTURE)
    {
        self->irq_trigger |= TIMR_IRQ_CAPTURE;

        NVIC_EnableIRQ(self->IRQn);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pulse_irq_enable_obj, pulse_irq_enable);


STATIC mp_obj_t pulse_irq_disable(mp_obj_t self_in, mp_obj_t irq_trigger)
{
    pyb_pulse_obj_t *self = self_in;

    uint trigger = mp_obj_get_int(irq_trigger);

    if(trigger & TIMR_IRQ_CAPTURE)
    {
        self->irq_trigger &= ~TIMR_IRQ_CAPTURE;

        NVIC_DisableIRQ(self->IRQn);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pulse_irq_disable_obj, pulse_irq_disable);


void PULSE_Handler(void)
{
    pyb_pulse_obj_t *self = &pyb_pulse_obj[0];

    if(self->TIMRx->IF & TIMRG_IF_PULSE_Msk)
    {
        self->TIMRx->IF = TIMRG_IF_PULSE_Msk;

        if(self->irq_trigger & TIMR_IRQ_CAPTURE)
            self->irq_flags |= TIMR_IRQ_CAPTURE;
    }

    if(self->irq_callback != mp_const_none)
    {
        gc_lock();
        nlr_buf_t nlr;
        if(nlr_push(&nlr) == 0)
        {
            mp_call_function_1(self->irq_callback, self);
            nlr_pop();
        }
        else
        {
            self->irq_callback = mp_const_none;

            printf("uncaught exception in Pulse(%u) interrupt handler\n", self->pulse_id);
            mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        }
        gc_unlock();
    }

    self->irq_flags = 0;
}


STATIC const mp_rom_map_elem_t pulse_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_start),        MP_ROM_PTR(&pulse_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_done),         MP_ROM_PTR(&pulse_done_obj) },
    { MP_ROM_QSTR(MP_QSTR_value),        MP_ROM_PTR(&pulse_value_obj) },

    { MP_ROM_QSTR(MP_QSTR_irq_flags),    MP_ROM_PTR(&pulse_irq_flags_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq_enable),   MP_ROM_PTR(&pulse_irq_enable_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq_disable),  MP_ROM_PTR(&pulse_irq_disable_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_PULSE_LOW),    MP_ROM_INT(PULSE_LOW) },
    { MP_ROM_QSTR(MP_QSTR_PULSE_HIGH),   MP_ROM_INT(PULSE_HIGH) },

    { MP_ROM_QSTR(MP_QSTR_IRQ_CAPTURE),  MP_ROM_INT(TIMR_IRQ_CAPTURE) },
};
STATIC MP_DEFINE_CONST_DICT(pulse_locals_dict, pulse_locals_dict_table);


const mp_obj_type_t pyb_pulse_type = {
    { &mp_type_type },
    .name = MP_QSTR_Pulse,
    .print = pulse_print,
    .make_new = pulse_make_new,
    .locals_dict = (mp_obj_t)&pulse_locals_dict,
};
