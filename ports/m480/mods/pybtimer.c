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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include "chip/M480.h"

#include "mods/pybtimer.h"

#define TIMR_MODE_TIMER     0
#define TIMR_MODE_COUNTER   1
#define TIMR_MODE_CAPTURE   2

#define TIMR_EDGE_FALLING   0
#define TIMR_EDGE_RISING    1
#define TIMR_EDGE_BOTH      2

#define TIMR_IRQ_NONE       0
#define TIMR_IRQ_TIMEOUT    1
#define TIMR_IRQ_CAPTURE    2

#define TIMER_TRG_TO_NONE   0


/******************************************************************************
 DEFINE PRIVATE TYPES
 ******************************************************************************/
typedef enum {
    PYB_TIMER_0      =  0,
    PYB_TIMER_1      =  1,
    PYB_TIMER_2      =  2,
    PYB_TIMER_3      =  3,
    PYB_NUM_TIMERS
} pyb_timer_id_t;


typedef struct _pyb_timer_obj_t {
    mp_obj_base_t base;
    pyb_timer_id_t timer_id;
    TIMER_T *TIMRx;
    uint32_t period;
    uint8_t  mode;
    uint8_t  edge;

    uint8_t  trigger;           // to ADC、DAC、DMA、PWM

    uint8_t  IRQn;
    uint8_t  irq_flags;         // IRQ_TIMEOUT、IRQ_CAPTUE
    uint8_t  irq_trigger;
    uint8_t  irq_priority;      // 中断优先级
    mp_obj_t irq_callback;      // 中断处理函数
} pyb_timer_obj_t;


/******************************************************************************
 DEFINE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_timer_obj_t pyb_timer_obj[PYB_NUM_TIMERS] = {
    { .timer_id = PYB_TIMER_0, .TIMRx = TIMER0, .IRQn = TMR0_IRQn, .period = 0 },
    { .timer_id = PYB_TIMER_1, .TIMRx = TIMER1, .IRQn = TMR1_IRQn, .period = 0 },
    { .timer_id = PYB_TIMER_2, .TIMRx = TIMER2, .IRQn = TMR2_IRQn, .period = 0 },
    { .timer_id = PYB_TIMER_3, .TIMRx = TIMER3, .IRQn = TMR3_IRQn, .period = 0 },
};


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/


/******************************************************************************/
/* MicroPython bindings                                                      */

STATIC void pyb_timer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_timer_obj_t *self = self_in;

    switch(self->mode) {
    case TIMR_MODE_TIMER:
        mp_printf(print, "Timer(%u, period=%uuS)", self->timer_id, self->period);
        break;

    case TIMR_MODE_COUNTER:
        mp_printf(print, "Counter(%u, period=%u, edge=%s)", self->timer_id, self->period,
                  self->edge == TIMR_EDGE_FALLING ? "Falling" : "Rising");
        break;

    case TIMR_MODE_CAPTURE:
        mp_printf(print, "Capture(%u, period=%u, edge=%s)", self->timer_id, self->period,
                  self->edge == TIMR_EDGE_FALLING ? "Falling" : (self->edge == TIMR_EDGE_RISING ? "Rising" : "Both"));
        break;

    default:
        break;
    }
}


STATIC mp_obj_t pyb_timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_id, ARG_period, ARG_mode, ARG_edge, ARG_trigger, ARG_irq, ARG_callback, ARG_priority };
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_period,   MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 120000} },
        { MP_QSTR_mode,     MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = TIMR_MODE_TIMER} },
        { MP_QSTR_edge,     MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = TIMR_EDGE_FALLING} },
        { MP_QSTR_trigger,  MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = TIMER_TRG_TO_NONE} },
        { MP_QSTR_irq,      MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = TIMR_IRQ_TIMEOUT} },
        { MP_QSTR_callback, MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_priority, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 7} }
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint timer_id = args[0].u_int;
    if(timer_id >= PYB_NUM_TIMERS)
    {
        mp_raise_OSError(MP_ENODEV);
    }

    pyb_timer_obj_t *self = &pyb_timer_obj[timer_id];
    self->base.type = &pyb_timer_type;

    self->period = args[ARG_period].u_int;
    if(self->period > 0xFFFFFF)
    {
        goto invalid_args;
    }

    self->mode = args[ARG_mode].u_int;
    if((self->mode != TIMR_MODE_TIMER) && (self->mode != TIMR_MODE_COUNTER) && (self->mode != TIMR_MODE_CAPTURE))
    {
        goto invalid_args;
    }

    self->edge = args[ARG_edge].u_int;
    if(((self->mode == TIMR_MODE_COUNTER) && (self->edge > TIMR_EDGE_RISING)) ||
       ((self->mode == TIMR_MODE_CAPTURE) && (self->edge > TIMR_EDGE_BOTH)))
    {
        goto invalid_args;
    }

    self->trigger = args[ARG_trigger].u_int;
    if((self->trigger != TIMER_TRG_TO_EADC) && (self->trigger != TIMER_TRG_TO_DAC) && (self->trigger != TIMER_TRG_TO_EPWM) &&
       (self->trigger != TIMER_TRG_TO_PDMA) && (self->trigger != TIMER_TRG_TO_NONE))
    {
        goto invalid_args;
    }

    self->irq_trigger = args[ARG_irq].u_int;
    if((self->irq_trigger != TIMR_IRQ_TIMEOUT) && (self->irq_trigger != TIMR_IRQ_CAPTURE) && (self->irq_trigger != TIMR_IRQ_NONE))
    {
        goto invalid_args;
    }

    self->irq_callback = args[ARG_callback].u_obj;

    self->irq_priority = args[ARG_priority].u_int;
    if(self->irq_priority > 15)
    {
        goto invalid_args;
    }

    switch(self->timer_id) {
    case PYB_TIMER_0:
        CLK_EnableModuleClock(TMR0_MODULE);
        CLK_SetModuleClock(TMR0_MODULE, CLK_CLKSEL1_TMR0SEL_PCLK0, 0);
        break;

    case PYB_TIMER_1:
        CLK_EnableModuleClock(TMR1_MODULE);
        CLK_SetModuleClock(TMR1_MODULE, CLK_CLKSEL1_TMR1SEL_PCLK0, 0);
        break;

    case PYB_TIMER_2:
        CLK_EnableModuleClock(TMR2_MODULE);
        CLK_SetModuleClock(TMR2_MODULE, CLK_CLKSEL1_TMR2SEL_PCLK1, 0);
        break;

    case PYB_TIMER_3:
        CLK_EnableModuleClock(TMR3_MODULE);
        CLK_SetModuleClock(TMR3_MODULE, CLK_CLKSEL1_TMR3SEL_PCLK1, 0);
        break;

    default:
        break;
    }

    self->TIMRx->CTL &= ~TIMER_CTL_OPMODE_Msk;
    self->TIMRx->CTL |= TIMER_PERIODIC_MODE;

    TIMER_SET_CMP_VALUE(self->TIMRx, self->period);

    switch (self->mode) {
    case TIMR_MODE_TIMER:
        TIMER_SET_PRESCALE_VALUE(self->TIMRx, 192-1);
        break;

    case TIMR_MODE_COUNTER:
        TIMER_SET_PRESCALE_VALUE(self->TIMRx, 0);

        TIMER_EnableEventCounter(self->TIMRx, self->edge);
        break;

    case TIMR_MODE_CAPTURE:
        TIMER_SET_PRESCALE_VALUE(self->TIMRx, 0);

        TIMER_EnableCapture(self->TIMRx, TIMER_CAPTURE_COUNTER_RESET_MODE, self->edge);

        if(self->irq_trigger & TIMR_IRQ_CAPTURE) TIMER_EnableCaptureInt(self->TIMRx);
        break;

    default:
        break;
    }

    TIMER_SetTriggerSource(self->TIMRx, TIMER_TRGSRC_TIMEOUT_EVENT);
    if(self->trigger != TIMER_TRG_TO_NONE) TIMER_SetTriggerTarget(self->TIMRx, self->trigger);

    if(self->irq_trigger & TIMR_IRQ_TIMEOUT) TIMER_EnableInt(self->TIMRx);

    NVIC_SetPriority(self->IRQn, self->irq_priority);
    NVIC_EnableIRQ(self->IRQn);

    return self;

invalid_args:
    mp_raise_ValueError("invalid argument(s) value");
}


STATIC mp_obj_t pyb_timer_period(size_t n_args, const mp_obj_t *args) {
    pyb_timer_obj_t *self = args[0];
    if (n_args == 1) {  // get
        return mp_obj_new_int(self->period);
    } else {            // set
        self->period = mp_obj_get_int(args[1]);
        TIMER_SET_CMP_VALUE(self->TIMRx, self->period);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_timer_period_obj, 1, 2, pyb_timer_period);


STATIC mp_obj_t pyb_timer_value(mp_obj_t self_in) {
    pyb_timer_obj_t *self = self_in;

    return MP_OBJ_SMALL_INT_VALUE(TIMER_GetCounter(self->TIMRx));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_timer_value_obj, pyb_timer_value);


STATIC mp_obj_t pyb_timer_start(mp_obj_t self_in) {
    pyb_timer_obj_t *self = self_in;

    TIMER_Start(self->TIMRx);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_timer_start_obj, pyb_timer_start);


STATIC mp_obj_t pyb_timer_stop(mp_obj_t self_in) {
    pyb_timer_obj_t *self = self_in;

    TIMER_Stop(self->TIMRx);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_timer_stop_obj, pyb_timer_stop);


STATIC mp_obj_t pyb_timer_irq_flags(mp_obj_t self_in) {
    pyb_timer_obj_t *self = self_in;

    return MP_OBJ_SMALL_INT_VALUE(self->irq_flags);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_timer_irq_flags_obj, pyb_timer_irq_flags);


STATIC mp_obj_t pyb_timer_irq_enable(mp_obj_t self_in, mp_obj_t irq_trigger) {
    pyb_timer_obj_t *self = self_in;

    uint trigger = mp_obj_get_int(irq_trigger);
    switch(trigger) {
    case TIMR_IRQ_TIMEOUT:
        self->irq_trigger |= TIMR_IRQ_TIMEOUT;

        TIMER_ClearIntFlag(self->TIMRx);
        TIMER_EnableInt(self->TIMRx);

        NVIC_EnableIRQ(self->IRQn);
        break;

    case TIMR_IRQ_CAPTURE:
        self->irq_trigger |= TIMR_IRQ_CAPTURE;

        TIMER_ClearCaptureIntFlag(self->TIMRx);
        TIMER_EnableCaptureInt(self->TIMRx);

        NVIC_EnableIRQ(self->IRQn);
        break;

    default:
        break;
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_timer_irq_enable_obj, pyb_timer_irq_enable);


STATIC mp_obj_t pyb_timer_irq_disable(mp_obj_t self_in, mp_obj_t irq_trigger) {
    pyb_timer_obj_t *self = self_in;

    uint trigger = mp_obj_get_int(irq_trigger);
    switch(trigger) {
    case TIMR_IRQ_TIMEOUT:
        self->irq_trigger &= ~TIMR_IRQ_TIMEOUT;

        TIMER_DisableInt(self->TIMRx);
        break;

    case TIMR_IRQ_CAPTURE:
        self->irq_trigger &= ~TIMR_IRQ_CAPTURE;

        TIMER_DisableCaptureInt(self->TIMRx);
        break;

    default:
        break;
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_timer_irq_disable_obj, pyb_timer_irq_disable);


void TIMR_Handler(uint timer_id)
{
    pyb_timer_obj_t *self = &pyb_timer_obj[timer_id];

    if(TIMER_GetIntFlag(self->TIMRx))
    {
        TIMER_ClearIntFlag(self->TIMRx);

        if(self->irq_trigger & TIMR_IRQ_TIMEOUT)
            self->irq_flags |= TIMR_IRQ_TIMEOUT;
    }

    if(TIMER_GetCaptureIntFlag(self->TIMRx))
    {
        TIMER_ClearCaptureIntFlag(self->TIMRx);

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

            printf("uncaught exception in Timer(%u) interrupt handler\n", self->timer_id);
            mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        }
        gc_unlock();
    }

    self->irq_flags = 0;
}

void TMR0_IRQHandler(void)
{
    TIMR_Handler(PYB_TIMER_0);
}

void TMR1_IRQHandler(void)
{
    TIMR_Handler(PYB_TIMER_1);
}

void TMR2_IRQHandler(void)
{
    TIMR_Handler(PYB_TIMER_2);
}

void TMR3_IRQHandler(void)
{
    TIMR_Handler(PYB_TIMER_3);
}


STATIC const mp_rom_map_elem_t pyb_timer_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_period),             MP_ROM_PTR(&pyb_timer_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_value),              MP_ROM_PTR(&pyb_timer_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_start),              MP_ROM_PTR(&pyb_timer_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop),               MP_ROM_PTR(&pyb_timer_stop_obj) },

    { MP_ROM_QSTR(MP_QSTR_irq_flags),          MP_ROM_PTR(&pyb_timer_irq_flags_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq_enable),         MP_ROM_PTR(&pyb_timer_irq_enable_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq_disable),        MP_ROM_PTR(&pyb_timer_irq_disable_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_MODE_TIMER),         MP_ROM_INT(TIMR_MODE_TIMER) },
    { MP_ROM_QSTR(MP_QSTR_MODE_COUNTER),       MP_ROM_INT(TIMR_MODE_COUNTER) },
    { MP_ROM_QSTR(MP_QSTR_MODE_CAPTURE),       MP_ROM_INT(TIMR_MODE_CAPTURE) },

    { MP_ROM_QSTR(MP_QSTR_IRQ_NONE),           MP_ROM_INT(TIMR_IRQ_NONE) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_TIMEOUT),        MP_ROM_INT(TIMR_IRQ_TIMEOUT) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_CAPTURE),        MP_ROM_INT(TIMR_IRQ_CAPTURE) },

    { MP_ROM_QSTR(MP_QSTR_TO_ADC),             MP_ROM_INT(TIMER_TRG_TO_EADC) },
    { MP_ROM_QSTR(MP_QSTR_TO_DAC),             MP_ROM_INT(TIMER_TRG_TO_DAC) },
    { MP_ROM_QSTR(MP_QSTR_TO_DMA),             MP_ROM_INT(TIMER_TRG_TO_PDMA) },
    { MP_ROM_QSTR(MP_QSTR_TO_PWM),             MP_ROM_INT(TIMER_TRG_TO_EPWM) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_timer_locals_dict, pyb_timer_locals_dict_table);


const mp_obj_type_t pyb_timer_type = {
    { &mp_type_type },
    .name = MP_QSTR_Timer,
    .print = pyb_timer_print,
    .make_new = pyb_timer_make_new,
    .locals_dict = (mp_obj_t)&pyb_timer_locals_dict,
};
