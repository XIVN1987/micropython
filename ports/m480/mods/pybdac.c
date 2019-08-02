/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include "chip/M480.h"

#include "mods/pybpin.h"
#include "mods/pybdac.h"


/// \moduleref pyb
/// \class DAC - digital to analog conversion
///


/******************************************************************************
 DEFINE PRIVATE TYPES
 ******************************************************************************/
typedef enum {
    PYB_DAC_0   = 0,
    PYB_DAC_1   = 1,
    PYB_NUM_DACS
} pyb_dac_id_t;


typedef struct _pyb_dac_obj_t {
    mp_obj_base_t base;
    pyb_dac_id_t dac_id;
    DAC_T *DACx;

    uint8_t trigger;           // from PIN、TIMER
    uint8_t dma_chn;
} pyb_dac_obj_t;


/******************************************************************************
 DEFINE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_dac_obj_t pyb_dac_obj[PYB_NUM_DACS] = {
    { .dac_id = PYB_DAC_0, .DACx = DAC0 },
    { .dac_id = PYB_DAC_1, .DACx = DAC1 },
};


/******************************************************************************/
// MicroPython bindings

STATIC void pyb_dac_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_dac_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_printf(print, "DAC(%u)", self->dac_id);
}


STATIC mp_obj_t pyb_dac_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_id, ARG_trigger, ARG_data };
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,      MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_trigger, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = DAC_WRITE_DAT_TRIGGER} },
        { MP_QSTR_data,    MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint dac_id = args[0].u_int;
    if(dac_id >= PYB_NUM_DACS) {
        mp_raise_OSError(MP_ENODEV);
    }

    pyb_dac_obj_t *self = &pyb_dac_obj[dac_id];
    self->base.type = &pyb_dac_type;

    self->trigger = args[ARG_trigger].u_int;
    if((self->trigger != DAC_WRITE_DAT_TRIGGER) && (self->trigger != DAC_RISING_EDGE_TRIGGER) && (self->trigger != DAC_FALLING_EDGE_TRIGGER) &&
       (self->trigger != DAC_TIMER0_TRIGGER) && (self->trigger != DAC_TIMER1_TRIGGER) && (self->trigger != DAC_TIMER2_TRIGGER) && (DAC_TIMER3_TRIGGER))
    {
        goto invalid_args;
    }

    switch(self->dac_id) {
    case PYB_DAC_0:
        pin_config_by_name("PB12", "PB12_DAC0_OUT");
        break;

    case PYB_DAC_1:
        pin_config_by_name("PB13", "PB13_DAC1_OUT");
        break;

    default:
        break;
    }

    switch(self->dac_id) {
    case PYB_DAC_0:
    case PYB_DAC_1:
        CLK_EnableModuleClock(DAC_MODULE);
        break;

    default:
        break;
    }

    DAC_SetDelayTime(self->DACx, 1);

    DAC_Open(self->DACx, 0, self->trigger);

    if(self->trigger != DAC_WRITE_DAT_TRIGGER)
    {
        if((self->trigger == DAC_RISING_EDGE_TRIGGER) || (self->trigger == DAC_FALLING_EDGE_TRIGGER))
        {
            switch(self->dac_id) {
            case PYB_DAC_0:
                pin_config_by_name("PA10", "PA10_DAC0_ST");
                break;

            case PYB_DAC_1:
                pin_config_by_name("PA11", "PA11_DAC1_ST");
                break;

            default:
                break;
            }
        }

        if(args[ARG_data].u_obj == mp_const_none)
        {
            goto invalid_args;
        }

        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[ARG_data].u_obj, &bufinfo, MP_BUFFER_READ);

        CLK_EnableModuleClock(PDMA_MODULE);

        PDMA_Open(PDMA, (1 << 0));

        PDMA_SetTransferCnt(PDMA, 0, PDMA_WIDTH_16, bufinfo.len);

        PDMA_SetTransferAddr(PDMA, 0, (uint32_t)bufinfo.buf, PDMA_SAR_INC, (uint32_t)&self->DACx->DAT, PDMA_DAR_FIX);

        PDMA_SetTransferMode(PDMA, 0, PDMA_DAC0_TX, 0, 0);

        PDMA_SetBurstType(PDMA, 0, PDMA_REQ_SINGLE, 0);

        self->DACx->CTL |= (1 << DAC_CTL_DMAEN_Pos);
    }

    return self;

invalid_args:
    mp_raise_ValueError("invalid argument(s) value");
}


STATIC mp_obj_t pyb_dac_write(mp_obj_t self_in, mp_obj_t val) {
    pyb_dac_obj_t *self = self_in;

    DAC_WRITE_DATA(self->DACx, 0, mp_obj_get_int(val));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_dac_write_obj, pyb_dac_write);


STATIC mp_obj_t pyb_dac_dma_write(mp_obj_t self_in, mp_obj_t data) {
    pyb_dac_obj_t *self = self_in;

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(data, &bufinfo, MP_BUFFER_READ);

    PDMA_SetTransferAddr(PDMA, self->dma_chn, (uint32_t)bufinfo.buf, PDMA_SAR_INC, (uint32_t)&self->DACx->DAT, PDMA_DAR_FIX);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_dac_dma_write_obj, pyb_dac_dma_write);


STATIC const mp_rom_map_elem_t pyb_dac_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_write),           MP_ROM_PTR(&pyb_dac_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_dma_write),       MP_ROM_PTR(&pyb_dac_dma_write_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_TRIG_NONE),       MP_ROM_INT(DAC_WRITE_DAT_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER0),     MP_ROM_INT(DAC_TIMER0_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER1),     MP_ROM_INT(DAC_TIMER1_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER2),     MP_ROM_INT(DAC_TIMER2_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER3),     MP_ROM_INT(DAC_TIMER3_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_RISING),     MP_ROM_INT(DAC_RISING_EDGE_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_FALLING),    MP_ROM_INT(DAC_FALLING_EDGE_TRIGGER) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_dac_locals_dict, pyb_dac_locals_dict_table);


const mp_obj_type_t pyb_dac_type = {
    { &mp_type_type },
    .name = MP_QSTR_DAC,
    .print = pyb_dac_print,
    .make_new = pyb_dac_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_dac_locals_dict,
};
