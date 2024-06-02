#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/gc.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include "chip/M480.h"

#include "mods/pybpin.h"
#include "mods/pybtimer.h"


#define TIMR_MODE_TIMER            0
#define TIMR_MODE_COUNTER          1
#define TIMR_MODE_CAPTURE          2

#define TIMR_EDGE_FALLING          0
#define TIMR_EDGE_RISING           1
#define TIMR_EDGE_FALLING_RISING   2
#define TIMR_EDGE_RISING_FALLING   3

#define TIMR_IRQ_TIMEOUT           1
#define TIMR_IRQ_CAPTURE           2


/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef enum {
    PYB_TIMER_0   =  0,
    PYB_TIMER_1   =  1,
    PYB_TIMER_2   =  2,
    PYB_TIMER_3   =  3,
    PYB_NUM_TIMERS
} pyb_timer_id_t;


typedef struct _pyb_timer_obj_t {
    mp_obj_base_t base;
    pyb_timer_id_t timer_id;
    TIMER_T *TIMRx;
    uint32_t period;
    uint32_t capvalue;
    uint8_t  prescale;
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
static pyb_timer_obj_t pyb_timer_obj[PYB_NUM_TIMERS] = {
    { {&pyb_timer_type}, .timer_id = PYB_TIMER_0, .TIMRx = TIMER0, .IRQn = TMR0_IRQn, .period = 0 },
    { {&pyb_timer_type}, .timer_id = PYB_TIMER_1, .TIMRx = TIMER1, .IRQn = TMR1_IRQn, .period = 0 },
    { {&pyb_timer_type}, .timer_id = PYB_TIMER_2, .TIMRx = TIMER2, .IRQn = TMR2_IRQn, .period = 0 },
    { {&pyb_timer_type}, .timer_id = PYB_TIMER_3, .TIMRx = TIMER3, .IRQn = TMR3_IRQn, .period = 0 },
};


/******************************************************************************/
/* MicroPython bindings                                                      */

static void timer_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_timer_obj_t *self = self_in;

    switch(self->mode)
    {
    case TIMR_MODE_TIMER:
        mp_printf(print, "Timer(%u, period=%uuS)", self->timer_id, self->period);
        break;

    case TIMR_MODE_COUNTER:
        mp_printf(print, "Counter(%u, period=%u, edge=%s)", self->timer_id, self->period,
                  self->edge == TIMR_EDGE_FALLING ? "Falling" : "Rising");
        break;

    case TIMR_MODE_CAPTURE:
        mp_printf(print, "Capture(%u, period=%u, edge=%s)", self->timer_id, self->period,
                  self->edge == TIMR_EDGE_FALLING ? "Falling" : (self->edge == TIMR_EDGE_RISING ? "Rising" : (self->edge == TIMR_EDGE_FALLING_RISING ? "Falling-Rising" : "Rising-Falling")));
        break;

    default:
        break;
    }
}


static mp_obj_t timer_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_id, ARG_period, ARG_mode, ARG_edge, ARG_trigger, ARG_prescale, ARG_irq, ARG_callback, ARG_priority, ARG_pin };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_period,   MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_mode,     MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = TIMR_MODE_TIMER} },
        { MP_QSTR_edge,     MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = TIMR_EDGE_FALLING} },
        { MP_QSTR_trigger,  MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_prescale, MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_irq,      MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = TIMR_IRQ_TIMEOUT} },
        { MP_QSTR_callback, MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_priority, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 3} },
        { MP_QSTR_pin,      MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint timer_id = args[ARG_id].u_int;
    if(timer_id >= PYB_NUM_TIMERS)
    {
        mp_raise_OSError(MP_ENODEV);
    }
    pyb_timer_obj_t *self = &pyb_timer_obj[timer_id];

    self->period = args[ARG_period].u_int;
    if(self->period > 0xFFFFFF)
    {
        mp_raise_ValueError("invalid period value");
    }

    self->mode = args[ARG_mode].u_int;
    if((self->mode != TIMR_MODE_TIMER) && (self->mode != TIMR_MODE_COUNTER) && (self->mode != TIMR_MODE_CAPTURE))
    {
        mp_raise_ValueError("invalid mode value");
    }

    self->edge = args[ARG_edge].u_int;
    if((self->mode == TIMR_MODE_COUNTER) && (self->edge > TIMR_EDGE_RISING))
    {
        mp_raise_ValueError("Counter support EDGE_FALLING and EDGE_RISING");
    }
    if((self->mode == TIMR_MODE_CAPTURE) && (self->edge > TIMR_EDGE_RISING_FALLING))
    {
        mp_raise_ValueError("Capture support EDGE_FALLING, EDGE_RISING, EDGE_FALLING_RISING and EDGE_RISING_FALLING");
    }

    if(args[ARG_pin].u_obj != mp_const_none)
    {
        if(self->mode == TIMR_MODE_COUNTER)
        {
            pin_config_by_func(args[ARG_pin].u_obj, "%s_TM%d", self->timer_id);
        }
        else if(self->mode == TIMR_MODE_CAPTURE)
        {
            pin_config_by_func(args[ARG_pin].u_obj, "%s_TM%d_EXT", self->timer_id);
        }

        if(self->edge == TIMR_EDGE_FALLING)
        {
            pin_config_pull(args[ARG_pin].u_obj, GPIO_PUSEL_PULL_UP);
        }
        else if(self->edge == TIMR_EDGE_RISING)
        {
            pin_config_pull(args[ARG_pin].u_obj, GPIO_PUSEL_PULL_DOWN);
        }
    }

    self->trigger = args[ARG_trigger].u_int;

    if(args[ARG_prescale].u_obj != mp_const_none)
    {
        self->prescale = mp_obj_get_int(args[ARG_prescale].u_obj);
        if((self->prescale < 1) || (self->prescale > 256))
        {
            mp_raise_ValueError("invalid prescale value");
        }
    }
    else
    {
        switch(self->mode)
        {
        case TIMR_MODE_TIMER:   self->prescale = CyclesPerUs;  break;
        case TIMR_MODE_COUNTER: self->prescale = 1;            break;
        case TIMR_MODE_CAPTURE: self->prescale = 1;            break;
        }
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
        NVIC_SetPriority(self->IRQn, self->irq_priority << 1);
        NVIC_EnableIRQ(self->IRQn);
    }

    switch(self->timer_id)
    {
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
    }

    self->TIMRx->CTL &= ~TIMER_CTL_OPMODE_Msk;
    self->TIMRx->CTL |= TIMER_PERIODIC_MODE;

    TIMER_SET_PRESCALE_VALUE(self->TIMRx, self->prescale-1);

    TIMER_SET_CMP_VALUE(self->TIMRx, self->period);

    if(self->irq_trigger & TIMR_IRQ_TIMEOUT) TIMER_EnableInt(self->TIMRx);

    switch(self->mode)
    {
    case TIMR_MODE_TIMER:
        break;

    case TIMR_MODE_COUNTER:
        TIMER_EnableEventCounter(self->TIMRx, self->edge == TIMR_EDGE_FALLING ? TIMER_COUNTER_EVENT_FALLING : TIMER_COUNTER_EVENT_RISING);
        break;

    case TIMR_MODE_CAPTURE:
        TIMER_EnableCaptureInt(self->TIMRx);    // 需要在ISR中清除中断标志，否则CAPDAT不更新
        break;
    }

    TIMER_SetTriggerSource(self->TIMRx, TIMER_TRGSRC_TIMEOUT_EVENT);
    TIMER_SetTriggerTarget(self->TIMRx, self->trigger);

    return self;
}


static mp_obj_t timer_prescale(size_t n_args, const mp_obj_t *args)
{
    pyb_timer_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->prescale);
    }
    else                // set
    {
        self->prescale = mp_obj_get_int(args[1]);
        TIMER_SET_PRESCALE_VALUE(self->TIMRx, self->prescale-1);

        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(timer_prescale_obj, 1, 2, timer_prescale);


static mp_obj_t timer_period(size_t n_args, const mp_obj_t *args)
{
    pyb_timer_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->period);
    }
    else                // set
    {
        self->period = mp_obj_get_int(args[1]);
        TIMER_SET_CMP_VALUE(self->TIMRx, self->period);

        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(timer_period_obj, 1, 2, timer_period);


static mp_obj_t timer_value(mp_obj_t self_in)
{
    pyb_timer_obj_t *self = self_in;

    return mp_obj_new_int(TIMER_GetCounter(self->TIMRx));
}
static MP_DEFINE_CONST_FUN_OBJ_1(timer_value_obj, timer_value);


static mp_obj_t timer_start(mp_obj_t self_in)
{
    pyb_timer_obj_t *self = self_in;

    TIMER_Start(self->TIMRx);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(timer_start_obj, timer_start);


static mp_obj_t timer_stop(mp_obj_t self_in)
{
    pyb_timer_obj_t *self = self_in;

    TIMER_Stop(self->TIMRx);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(timer_stop_obj, timer_stop);


static mp_obj_t timer_cap_start(size_t n_args, const mp_obj_t *args)
{
    pyb_timer_obj_t *self = args[0];

    if(n_args == 2)
        self->edge = mp_obj_get_int(args[1]);

    uint edge;
    switch(self->edge)
    {
    case TIMR_EDGE_FALLING:         edge = TIMER_CAPTURE_EVENT_FALLING;         break;
    case TIMR_EDGE_RISING:          edge = TIMER_CAPTURE_EVENT_RISING;          break;
    case TIMR_EDGE_FALLING_RISING:  edge = TIMER_CAPTURE_EVENT_FALLING_RISING;  break;
    case TIMR_EDGE_RISING_FALLING:  edge = TIMER_CAPTURE_EVENT_RISING_FALLING;  break;
    }
    TIMER_EnableCapture(self->TIMRx, TIMER_CAPTURE_FREE_COUNTING_MODE, edge);

    self->capvalue = 0;

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(timer_cap_start_obj, 1, 2, timer_cap_start);


static mp_obj_t timer_cap_done(mp_obj_t self_in)
{
    pyb_timer_obj_t *self = self_in;

    return mp_obj_new_bool(self->capvalue);
}
static MP_DEFINE_CONST_FUN_OBJ_1(timer_cap_done_obj, timer_cap_done);


static mp_obj_t timer_cap_value(mp_obj_t self_in)
{
    pyb_timer_obj_t *self = self_in;

    return mp_obj_new_int(self->capvalue);
}
static MP_DEFINE_CONST_FUN_OBJ_1(timer_cap_value_obj, timer_cap_value);


static mp_obj_t timer_irq_flags(mp_obj_t self_in)
{
    pyb_timer_obj_t *self = self_in;

    return mp_obj_new_int(self->irq_flags);
}
static MP_DEFINE_CONST_FUN_OBJ_1(timer_irq_flags_obj, timer_irq_flags);


static mp_obj_t timer_irq_enable(mp_obj_t self_in, mp_obj_t irq_trigger)
{
    pyb_timer_obj_t *self = self_in;

    uint trigger = mp_obj_get_int(irq_trigger);

    if(trigger & TIMR_IRQ_TIMEOUT)
    {
        self->irq_trigger |= TIMR_IRQ_TIMEOUT;

        TIMER_ClearIntFlag(self->TIMRx);
        TIMER_EnableInt(self->TIMRx);
    }

    if(trigger & TIMR_IRQ_CAPTURE)
    {
        self->irq_trigger |= TIMR_IRQ_CAPTURE;
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(timer_irq_enable_obj, timer_irq_enable);


static mp_obj_t timer_irq_disable(mp_obj_t self_in, mp_obj_t irq_trigger)
{
    pyb_timer_obj_t *self = self_in;

    uint trigger = mp_obj_get_int(irq_trigger);

    if(trigger & TIMR_IRQ_TIMEOUT)
    {
        self->irq_trigger &= ~TIMR_IRQ_TIMEOUT;

        TIMER_DisableInt(self->TIMRx);
    }

    if(trigger & TIMR_IRQ_CAPTURE)
    {
        self->irq_trigger &= ~TIMR_IRQ_CAPTURE;
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(timer_irq_disable_obj, timer_irq_disable);


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

        if(TIMER_GetCaptureData(self->TIMRx))   // 非零，第二边沿
        {
            self->capvalue = TIMER_GetCaptureData(self->TIMRx);

            if(self->irq_trigger & TIMR_IRQ_CAPTURE)
                self->irq_flags |= TIMR_IRQ_CAPTURE;

            TIMER_DisableCapture(self->TIMRx);
        }
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


static const mp_rom_map_elem_t timer_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_prescale),            MP_ROM_PTR(&timer_prescale_obj) },
    { MP_ROM_QSTR(MP_QSTR_period),              MP_ROM_PTR(&timer_period_obj) },
    { MP_ROM_QSTR(MP_QSTR_value),               MP_ROM_PTR(&timer_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_start),               MP_ROM_PTR(&timer_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop),                MP_ROM_PTR(&timer_stop_obj) },

    { MP_ROM_QSTR(MP_QSTR_cap_start),           MP_ROM_PTR(&timer_cap_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_cap_done),            MP_ROM_PTR(&timer_cap_done_obj) },
    { MP_ROM_QSTR(MP_QSTR_cap_value),           MP_ROM_PTR(&timer_cap_value_obj) },

    { MP_ROM_QSTR(MP_QSTR_irq_flags),           MP_ROM_PTR(&timer_irq_flags_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq_enable),          MP_ROM_PTR(&timer_irq_enable_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq_disable),         MP_ROM_PTR(&timer_irq_disable_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_TIMER),               MP_ROM_INT(TIMR_MODE_TIMER) },
    { MP_ROM_QSTR(MP_QSTR_COUNTER),             MP_ROM_INT(TIMR_MODE_COUNTER) },
    { MP_ROM_QSTR(MP_QSTR_CAPTURE),             MP_ROM_INT(TIMR_MODE_CAPTURE) },

    { MP_ROM_QSTR(MP_QSTR_EDGE_FALLING),        MP_ROM_INT(TIMR_EDGE_FALLING) },
    { MP_ROM_QSTR(MP_QSTR_EDGE_RISING),         MP_ROM_INT(TIMR_EDGE_RISING) },
    { MP_ROM_QSTR(MP_QSTR_EDGE_FALLING_RISING), MP_ROM_INT(TIMR_EDGE_FALLING_RISING) },
    { MP_ROM_QSTR(MP_QSTR_EDGE_RISING_FALLING), MP_ROM_INT(TIMR_EDGE_RISING_FALLING) },

    { MP_ROM_QSTR(MP_QSTR_IRQ_TIMEOUT),         MP_ROM_INT(TIMR_IRQ_TIMEOUT) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_CAPTURE),         MP_ROM_INT(TIMR_IRQ_CAPTURE) },

    { MP_ROM_QSTR(MP_QSTR_TO_ADC),              MP_ROM_INT(TIMER_TRG_TO_EADC) },
    { MP_ROM_QSTR(MP_QSTR_TO_DAC),              MP_ROM_INT(TIMER_TRG_TO_DAC) },
    { MP_ROM_QSTR(MP_QSTR_TO_DMA),              MP_ROM_INT(TIMER_TRG_TO_PDMA) },
    { MP_ROM_QSTR(MP_QSTR_TO_PWM),              MP_ROM_INT(TIMER_TRG_TO_EPWM) },
};
static MP_DEFINE_CONST_DICT(timer_locals_dict, timer_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    pyb_timer_type,
    MP_QSTR_Timer,
    MP_TYPE_FLAG_NONE,
    print, timer_print,
    make_new, timer_make_new,
    locals_dict, &timer_locals_dict
);
