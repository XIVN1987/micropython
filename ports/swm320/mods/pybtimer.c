#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include "pybtimer.h"
#include "pybpin.h"


#define TIMR_IRQ_TIMEOUT   1
#define TIMR_IRQ_CAPTURE   2


/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef enum {
    PYB_TIMER_0   =  0,
    PYB_TIMER_1   =  1,
    PYB_TIMER_2   =  2,
    PYB_TIMER_3   =  3,
    PYB_TIMER_4   =  4,
    PYB_TIMER_5   =  5,
    PYB_NUM_TIMERS
} pyb_timer_id_t;

typedef struct {
    mp_obj_base_t base;
    pyb_timer_id_t timer_id;
    TIMR_TypeDef *TIMRx;
    uint32_t period;
    uint8_t  mode;

    uint8_t  IRQn;
    uint8_t  irq_flags;         // 中断标志
    uint8_t  irq_trigger;
    uint8_t  irq_priority;      // 中断优先级
    mp_obj_t irq_callback;      // 中断处理函数
} pyb_timer_obj_t;


/******************************************************************************
 DEFINE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_timer_obj_t pyb_timer_obj[PYB_NUM_TIMERS] = {
    { {&pyb_timer_type}, .timer_id = PYB_TIMER_0, .TIMRx = TIMR0, .period = 0, .IRQn = TIMR0_IRQn },
    { {&pyb_timer_type}, .timer_id = PYB_TIMER_1, .TIMRx = TIMR1, .period = 0, .IRQn = TIMR1_IRQn },
    { {&pyb_timer_type}, .timer_id = PYB_TIMER_2, .TIMRx = TIMR2, .period = 0, .IRQn = TIMR2_IRQn },
    { {&pyb_timer_type}, .timer_id = PYB_TIMER_3, .TIMRx = TIMR3, .period = 0, .IRQn = TIMR3_IRQn },
    { {&pyb_timer_type}, .timer_id = PYB_TIMER_4, .TIMRx = TIMR4, .period = 0, .IRQn = TIMR4_IRQn },
    { {&pyb_timer_type}, .timer_id = PYB_TIMER_5, .TIMRx = TIMR5, .period = 0, .IRQn = TIMR5_IRQn },
};


/******************************************************************************/
/* MicroPython bindings                                                      */

STATIC void timer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_timer_obj_t *self = self_in;

    switch(self->mode)
    {
    case TIMR_MODE_TIMER:
        mp_printf(print, "Timer(%u, period=%uuS)", self->timer_id, self->period/CyclesPerUs);
        break;

    case TIMR_MODE_COUNTER:
        mp_printf(print, "Counter(%u, period=%u)", self->timer_id, self->period);
        break;
    }
}


STATIC mp_obj_t timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_id, ARG_period, ARG_mode, ARG_irq, ARG_callback,  ARG_priority, ARG_pin };
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,        MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_period,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 100000} },
        { MP_QSTR_mode,      MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = TIMR_MODE_TIMER} },
        { MP_QSTR_irq,       MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = TIMR_IRQ_TIMEOUT} },
        { MP_QSTR_callback,  MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_priority,  MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 3} },
        { MP_QSTR_pin,       MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(args), allowed_args, args);

    uint timer_id = args[ARG_id].u_int;
    if(timer_id >= PYB_NUM_TIMERS)
    {
        mp_raise_OSError(MP_ENODEV);
    }
    pyb_timer_obj_t *self = &pyb_timer_obj[timer_id];

    self->period = args[ARG_period].u_int;

    self->mode = args[ARG_mode].u_int;
    if(self->mode == TIMR_MODE_TIMER)
    {
        self->period *= CyclesPerUs;          // 定时器模式：单位为us
    }
    else if(self->mode == TIMR_MODE_COUNTER)
    {
        pin_obj_t * pin = pin_find_by_name(args[ARG_pin].u_obj);

        switch(self->timer_id)
        {
        case PYB_TIMER_0:
            if((pin->pbit % 2) == 1)
                mp_raise_ValueError("Timer0 Counter IN need be Even number pin, like PA0, PA2, PA4");
            pin_config(pin, FUNMUX0_TIMR0_IN, 0, 0);
            break;

        case PYB_TIMER_1:
            if((pin->pbit % 2) == 0)
                mp_raise_ValueError("Timer1 Counter IN need be Odd  number pin, like PA1, PA3, PA5");
            pin_config(pin, FUNMUX1_TIMR1_IN, 0, 0);
            break;

        case PYB_TIMER_2:
            if((pin->pbit % 2) == 1)
                mp_raise_ValueError("Timer2 Counter IN need be Even number pin, like PA0, PA2, PA4");
            pin_config(pin, FUNMUX0_TIMR2_IN, 0, 0);
            break;

        case PYB_TIMER_3:
            if((pin->pbit % 2) == 0)
                mp_raise_ValueError("Timer3 Counter IN need be Odd number pin, like PA1, PA3, PA5");
            pin_config(pin, FUNMUX1_TIMR3_IN, 0, 0);
            break;

        default:
            mp_raise_ValueError("Timer4 and Timer5 not have Counter function");
            break;
        }
    }
    else
    {
        mp_raise_ValueError("invalid mode value");
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
        NVIC_EnableIRQ(self->IRQn);
    }

    TIMR_Init(self->TIMRx, self->mode, self->period, self->irq_trigger & TIMR_IRQ_TIMEOUT);

    return self;
}


STATIC mp_obj_t timer_period(size_t n_args, const mp_obj_t *args)
{
    pyb_timer_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        if(self->mode == TIMR_MODE_TIMER)
            return mp_obj_new_int(self->period / CyclesPerUs);  // 定时器模式：单位为us
        else
            return mp_obj_new_int(self->period);                // 计数器模式：脉冲个数
    }
    else                // set
    {
        self->period = mp_obj_get_int(args[1]);

        if(self->mode == TIMR_MODE_TIMER)
            self->period *= CyclesPerUs;                        // 定时器模式：单位为us

        TIMR_SetPeriod(self->TIMRx, self->period);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(timer_period_obj, 1, 2, timer_period);


STATIC mp_obj_t timer_value(mp_obj_t self_in)
{
    pyb_timer_obj_t *self = self_in;

    uint value = self->period - TIMR_GetCurValue(self->TIMRx);  // 向下计数

    if(self->mode == TIMR_MODE_TIMER)
    {
        return mp_obj_new_int(value / CyclesPerUs);
    }
    else //          TIMR_MODE_COUNTER
    {
        return mp_obj_new_int(value);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(timer_value_obj, timer_value);


STATIC mp_obj_t timer_start(mp_obj_t self_in)
{
    pyb_timer_obj_t *self = self_in;

    TIMR_Start(self->TIMRx);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(timer_start_obj, timer_start);


STATIC mp_obj_t timer_stop(mp_obj_t self_in)
{
    pyb_timer_obj_t *self = self_in;

    TIMR_Stop(self->TIMRx);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(timer_stop_obj, timer_stop);


STATIC mp_obj_t timer_irq_flags(mp_obj_t self_in)
{
    pyb_timer_obj_t *self = self_in;

    return mp_obj_new_int(self->irq_flags);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(timer_irq_flags_obj, timer_irq_flags);


STATIC mp_obj_t timer_irq_enable(mp_obj_t self_in, mp_obj_t irq_trigger)
{
    pyb_timer_obj_t *self = self_in;

    uint trigger = mp_obj_get_int(irq_trigger);

    if(trigger & TIMR_IRQ_TIMEOUT)
    {
        self->irq_trigger |= TIMR_IRQ_TIMEOUT;

        TIMR_INTEn(self->TIMRx);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(timer_irq_enable_obj, timer_irq_enable);


STATIC mp_obj_t timer_irq_disable(mp_obj_t self_in, mp_obj_t irq_trigger)
{
    pyb_timer_obj_t *self = self_in;

    uint trigger = mp_obj_get_int(irq_trigger);

    if(trigger & TIMR_IRQ_TIMEOUT)
    {
        self->irq_trigger &= ~TIMR_IRQ_TIMEOUT;

        TIMR_INTDis(self->TIMRx);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(timer_irq_disable_obj, timer_irq_disable);


void TIMR_Handler(pyb_timer_obj_t *self)
{
    if(TIMR_INTStat(self->TIMRx))
    {
        TIMR_INTClr(self->TIMRx);

        if(self->irq_trigger & TIMR_IRQ_TIMEOUT)
            self->irq_flags |= TIMR_IRQ_TIMEOUT;
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

            printf("uncaught exception in Timer(%u) interrupt handler\n", self->timer_id);
            mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        }
        gc_unlock();
    }

    self->irq_flags = 0;
}

void TIMR0_Handler(void)
{
    TIMR_Handler(&pyb_timer_obj[PYB_TIMER_0]);
}

void TIMR1_Handler(void)
{
    TIMR_Handler(&pyb_timer_obj[PYB_TIMER_1]);
}

void TIMR2_Handler(void)
{
    TIMR_Handler(&pyb_timer_obj[PYB_TIMER_2]);
}

void TIMR3_Handler(void)
{
    TIMR_Handler(&pyb_timer_obj[PYB_TIMER_3]);
}

void TIMR4_Handler(void)
{
    TIMR_Handler(&pyb_timer_obj[PYB_TIMER_4]);
}

void TIMR5_Handler(void)
{
    TIMR_Handler(&pyb_timer_obj[PYB_TIMER_5]);
}


STATIC const mp_rom_map_elem_t timer_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_period),              MP_ROM_PTR(&timer_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_value),               MP_ROM_PTR(&timer_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_start),               MP_ROM_PTR(&timer_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop),                MP_ROM_PTR(&timer_stop_obj) },

    { MP_ROM_QSTR(MP_QSTR_irq_flags),           MP_ROM_PTR(&timer_irq_flags_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq_enable),          MP_ROM_PTR(&timer_irq_enable_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq_disable),         MP_ROM_PTR(&timer_irq_disable_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_TIMER),               MP_ROM_INT(TIMR_MODE_TIMER) },
    { MP_ROM_QSTR(MP_QSTR_COUNTER),             MP_ROM_INT(TIMR_MODE_COUNTER) },

    { MP_ROM_QSTR(MP_QSTR_IRQ_TIMEOUT),         MP_ROM_INT(TIMR_IRQ_TIMEOUT) },
//  { MP_ROM_QSTR(MP_QSTR_IRQ_CAPTURE),         MP_ROM_INT(TIMR_IRQ_CAPTURE) },
};
STATIC MP_DEFINE_CONST_DICT(timer_locals_dict, timer_locals_dict_table);


const mp_obj_type_t pyb_timer_type = {
    { &mp_type_type },
    .name = MP_QSTR_Timer,
    .print = timer_print,
    .make_new = timer_make_new,
    .locals_dict = (mp_obj_t)&timer_locals_dict,
};
