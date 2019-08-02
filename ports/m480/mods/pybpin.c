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

#include "py/mperrno.h"
#include "py/runtime.h"
#include "py/gc.h"

#include "chip/M480.h"

#include "mods/pybpin.h"


#define GPIO_INT_DISABLE    0


/// \moduleref pyb
/// \class Pin - control I/O pins
///

const pin_af_t *pin_af_find_by_name(pin_obj_t *self, qstr name);
const pin_af_t *pin_af_find_by_value(pin_obj_t *self, uint value);

/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/
pin_obj_t *pin_find_by_name(mp_obj_t name) {
    mp_map_t *named_pins = mp_obj_dict_get_map((mp_obj_t)&pins_locals_dict);

    mp_map_elem_t *named_elem = mp_map_lookup(named_pins, name, MP_MAP_LOOKUP);
    if(named_elem != NULL && named_elem->value != NULL) {
        return (pin_obj_t *)named_elem->value;
    }

    mp_raise_OSError(MP_ENODEV);
}

pin_obj_t *pin_find_by_port_bit(GPIO_T *port, uint pbit) {
    mp_map_t *named_pins = mp_obj_dict_get_map((mp_obj_t)&pins_locals_dict);

    for(uint i = 0; i < named_pins->used; i++) {
        if((((pin_obj_t *)named_pins->table[i].value)->port == port) &&
            (((pin_obj_t *)named_pins->table[i].value)->pbit == pbit)) {
            return (pin_obj_t *)named_pins->table[i].value;
        }
    }

    mp_raise_OSError(MP_ENODEV);
}

void pin_config_by_func(mp_obj_t obj, const char *format, uint id)
{
    if(MP_OBJ_IS_STR(obj))
    {
        const char *pin_name = mp_obj_str_get_str(obj);

        char af_name[64] = {0};
        if(strchr(format+1, '%'))
            snprintf(af_name, 64, format, pin_name, id);
        else
            snprintf(af_name, 64, format, pin_name);

        pin_config_by_name(pin_name, af_name);
    }
    else
    {
        mp_raise_OSError(MP_ENODEV);
    }
}

void pin_config_by_name(const char *pin_name, const char *af_name)
{
    pin_obj_t *pin = pin_find_by_name(mp_obj_new_str(pin_name, strlen(pin_name)));

    const pin_af_t *af = pin_af_find_by_name(pin, qstr_from_str(af_name));

    pin_config_by_value(pin, af->value, GPIO_MODE_INPUT, GPIO_PUSEL_DISABLE);
}

void pin_config_by_value(pin_obj_t *self, uint af_value, uint dir, uint pull) {
    const pin_af_t *af = pin_af_find_by_value(self, af_value);

    *af->reg &= ~af->mask;
    *af->reg |= af->value;

    self->af  = af->name;

    if(af_value == 0)
    {
        self->dir  = dir;
        self->pull = pull;

        GPIO_SetMode(self->port, self->pbit, self->dir);

        GPIO_SetPullCtl(self->port, self->pbit, self->pull);

        GPIO_SetSlewCtl(self->port, self->pbit, GPIO_SLEWCTL_FAST);
    }
}

void pin_config_pull(mp_obj_t name, uint pull)
{
    pin_obj_t *pin = pin_find_by_name(name);

    GPIO_SetPullCtl(pin->port, pin->pbit, pull);
}


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
const pin_af_t *pin_af_find_by_name(pin_obj_t *self, qstr name)
{
    for(uint i = 0; i < self->afn; i++)
    {
        if(self->afs[i].name == name)
        {
            return &self->afs[i];
        }
    }

    mp_raise_OSError(MP_ENODEV);
}

//注意：一个值可能对应多个名字，因此查到的af未必正确，但程序执行可保证正确
const pin_af_t *pin_af_find_by_value(pin_obj_t *self, uint value)
{
    for(uint i = 0; i < self->afn; i++)
    {
        if(self->afs[i].value == value)
        {
            return &self->afs[i];
        }
    }

    mp_raise_OSError(MP_ENODEV);
}


/******************************************************************************/
// MicroPython bindings

STATIC void pin_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pin_obj_t *self = self_in;

    if(self->af == 0)
    {
        mp_printf(print, "Pin('%q', dir=%s, pull=%s)", self->name, (self->dir == GPIO_MODE_INPUT) ? "In" : (self->dir == GPIO_MODE_OUTPUT ? "Out" : "Open-drain"),
                                                                   (self->pull == GPIO_PUSEL_PULL_UP) ? "Up" : (self->pull == GPIO_PUSEL_PULL_DOWN ? "Down" : "None"));
    }
    else
    {
        mp_printf(print, "Pin('%q', af=%q)", self->name, self->af);
    }
}


STATIC mp_obj_t pin_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_id, ARG_dir, ARG_af, ARG_pull, ARG_irq, ARG_callback, ARG_priority };
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_dir,                        MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_af,       MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_pull,     MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = GPIO_PUSEL_DISABLE} },
        { MP_QSTR_irq,      MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = GPIO_INT_DISABLE} },
        { MP_QSTR_callback, MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_priority, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 0} }
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    pin_obj_t *self = pin_find_by_name(args[ARG_id].u_obj);

    uint af = args[ARG_af].u_int;

    uint dir = args[ARG_dir].u_int;
    if((dir != GPIO_MODE_INPUT) && (dir != GPIO_MODE_OUTPUT) && (dir != GPIO_MODE_OPEN_DRAIN))
    {
        goto invalid_args;
    }

    uint pull = args[ARG_pull].u_int;
    if((pull != GPIO_PUSEL_PULL_UP) && (pull != GPIO_PUSEL_PULL_DOWN) && (pull != GPIO_PUSEL_DISABLE))
    {
        goto invalid_args;
    }

    pin_config_by_value(self, af, dir, pull);

    self->irq_trigger = args[ARG_irq].u_int;
    if((self->irq_trigger != GPIO_INT_RISING) && (self->irq_trigger != GPIO_INT_FALLING) && (self->irq_trigger != GPIO_INT_BOTH_EDGE) &&
       (self->irq_trigger != GPIO_INT_LOW) && (self->irq_trigger != GPIO_INT_HIGH) && (self->irq_trigger != GPIO_INT_DISABLE))
    {
        goto invalid_args;
    }

    self->irq_priority = args[ARG_priority].u_int;
    if(self->irq_priority > 15)
    {
        goto invalid_args;
    }

    self->irq_callback = args[ARG_callback].u_obj;
    if(self->irq_callback != mp_const_none)
    {
        if(self->irq_trigger == GPIO_INT_DISABLE)
        {
            goto invalid_args;
        }

        GPIO_EnableInt(self->port, self->pbit, self->irq_trigger);

        NVIC_SetPriority(self->IRQn, self->irq_priority);
        NVIC_EnableIRQ(self->IRQn);
    }

    return self;

invalid_args:
    mp_raise_ValueError("invalid argument(s) value");
}


STATIC mp_obj_t pin_value(size_t n_args, const mp_obj_t *args) {
    pin_obj_t *self = args[0];

    if (n_args == 1) {
        // get
        return MP_OBJ_NEW_SMALL_INT(*self->preg);
    } else {
        // set
        if (mp_obj_is_true(args[1])) {
            *self->preg = 1;
        } else {
            *self->preg = 0;
        }

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pin_value_obj, 1, 2, pin_value);


STATIC mp_obj_t pin_low(mp_obj_t self_in) {
    pin_obj_t *self = self_in;

    *self->preg = 0;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pin_low_obj, pin_low);


STATIC mp_obj_t pin_high(mp_obj_t self_in) {
    pin_obj_t *self = self_in;

    *self->preg = 1;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pin_high_obj, pin_high);


STATIC mp_obj_t pin_toggle(mp_obj_t self_in) {
    pin_obj_t *self = self_in;

    *self->preg = 1 - *self->preg;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pin_toggle_obj, pin_toggle);


STATIC mp_obj_t pin_irq_enable(mp_obj_t self_in) {
    pin_obj_t *self = self_in;

    GPIO_EnableInt(self->port, self->pbit, self->irq_trigger);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pin_irq_enable_obj, pin_irq_enable);


STATIC mp_obj_t pin_irq_disable(mp_obj_t self_in) {
    pin_obj_t *self = self_in;

    GPIO_DisableInt(self->port, self->pbit);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pin_irq_disable_obj, pin_irq_disable);


STATIC void GPX_Handler(GPIO_T *GPx, uint n) {
    for(uint i = 0; i < n; i++)
    {
        if(GPIO_GET_INT_FLAG(GPx, i))
        {
            GPIO_CLR_INT_FLAG(GPx, i);

            pin_obj_t *self = pin_find_by_port_bit(GPx, i);

            /* 执行中断回调 */
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

                    printf("uncaught exception on %s ISR\n", qstr_str(self->name));

                    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
                }
                gc_unlock();
            }
        }

        if(GPx->INTSRC == 0) break;
    }
}

void GPA_IRQHandler(void)
{
    GPX_Handler(GPA, 16);
}

void GPB_IRQHandler(void)
{
    GPX_Handler(GPB, 16);
}

void GPC_IRQHandler(void)
{
    GPX_Handler(GPC, 16);
}

void GPD_IRQHandler(void)
{
    GPX_Handler(GPD, 16);
}

void GPE_IRQHandler(void)
{
    GPX_Handler(GPE, 14);
}

void GPF_IRQHandler(void)
{
    GPX_Handler(GPF, 16);
}

void GPG_IRQHandler(void)
{
    GPX_Handler(GPG, 12);
}

void GPH_IRQHandler(void)
{
    GPX_Handler(GPH, 16);
}


STATIC const mp_rom_map_elem_t pin_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_value),                   MP_ROM_PTR(&pin_value_obj) },
    { MP_ROM_QSTR(MP_QSTR_low),                     MP_ROM_PTR(&pin_low_obj) },
    { MP_ROM_QSTR(MP_QSTR_high),                    MP_ROM_PTR(&pin_high_obj) },
    { MP_ROM_QSTR(MP_QSTR_toggle),                  MP_ROM_PTR(&pin_toggle_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq_enable),              MP_ROM_PTR(&pin_irq_enable_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq_disable),             MP_ROM_PTR(&pin_irq_disable_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_IN),                      MP_ROM_INT(GPIO_MODE_INPUT) },
    { MP_ROM_QSTR(MP_QSTR_OUT),                     MP_ROM_INT(GPIO_MODE_OUTPUT) },
    { MP_ROM_QSTR(MP_QSTR_OPEN_DRAIN),              MP_ROM_INT(GPIO_MODE_OPEN_DRAIN) },
    { MP_ROM_QSTR(MP_QSTR_PULL_UP),                 MP_ROM_INT(GPIO_PUSEL_PULL_UP) },
    { MP_ROM_QSTR(MP_QSTR_PULL_DOWN),               MP_ROM_INT(GPIO_PUSEL_PULL_DOWN) },
    { MP_ROM_QSTR(MP_QSTR_PULL_NONE),               MP_ROM_INT(GPIO_PUSEL_DISABLE) },
    { MP_ROM_QSTR(MP_QSTR_RISE_EDGE),               MP_ROM_INT(GPIO_INT_RISING) },
    { MP_ROM_QSTR(MP_QSTR_FALL_EDGE),               MP_ROM_INT(GPIO_INT_FALLING) },
    { MP_ROM_QSTR(MP_QSTR_BOTH_EDGE),               MP_ROM_INT(GPIO_INT_BOTH_EDGE) },
    { MP_ROM_QSTR(MP_QSTR_LOW_LEVEL),               MP_ROM_INT(GPIO_INT_LOW) },
    { MP_ROM_QSTR(MP_QSTR_HIGH_LEVEL),              MP_ROM_INT(GPIO_INT_HIGH) },

#include "genhdr/pins_af_const.h"
};
STATIC MP_DEFINE_CONST_DICT(pin_locals_dict, pin_locals_dict_table);

const mp_obj_type_t pin_type = {
    { &mp_type_type },
    .name = MP_QSTR_Pin,
    .print = pin_print,
    .make_new = pin_make_new,
    .locals_dict = (mp_obj_t)&pin_locals_dict,
};
