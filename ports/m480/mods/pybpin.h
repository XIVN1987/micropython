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
#ifndef MICROPY_INCLUDED_M480_MODS_PYBPIN_H
#define MICROPY_INCLUDED_M480_MODS_PYBPIN_H

#include "chip/M480.h"


typedef struct {
	const qstr        name;
	uint32_t          value;
	uint32_t          mask;
	volatile uint32_t *reg;
} pin_af_t;


typedef struct {
    const mp_obj_base_t base;
    const qstr          name;
	GPIO_T             *port;
	uint32_t            pbit;
	volatile uint32_t  *preg;           // Pin Data Input/Output Register
    qstr                af;             // pin_af_t name
    const pin_af_t     *afs;
    uint8_t             afn;            // alternative function count
    uint8_t             dir;
	uint8_t             pull;
	uint8_t             IRQn;
    uint8_t             irq_trigger;
    uint8_t             irq_priority;   // 中断优先级
    mp_obj_t            irq_callback;   // 中断处理函数
} pin_obj_t;

#include "pins.h"

extern const mp_obj_type_t pin_type;

extern const mp_obj_dict_t pins_locals_dict;


pin_obj_t *pin_find_by_name(mp_obj_t name);
pin_obj_t *pin_find_by_port_bit(GPIO_T *port, uint pbit);
void pin_config_by_func(mp_obj_t obj, const char *format, uint id);
void pin_config_by_name(const char *pin_name, const char *af_name);
void pin_config_by_value(pin_obj_t *self, uint af_value, uint dir, uint pull);

void pin_config_pull(mp_obj_t name, uint pull);

#endif //MICROPY_INCLUDED_M480_MODS_PYBPIN_H
