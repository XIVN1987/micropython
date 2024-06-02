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
    uint8_t dma_trigger;
} pyb_dac_obj_t;


/******************************************************************************
 DEFINE PRIVATE DATA
 ******************************************************************************/
static pyb_dac_obj_t pyb_dac_obj[PYB_NUM_DACS] = {
    { {&pyb_dac_type}, .dac_id = PYB_DAC_0, .DACx = DAC0, .dma_chn = 14, .dma_trigger = PDMA_DAC0_TX },
    { {&pyb_dac_type}, .dac_id = PYB_DAC_1, .DACx = DAC1, .dma_chn = 15, .dma_trigger = PDMA_DAC1_TX },
};


/******************************************************************************/
// MicroPython bindings

static void dac_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_dac_obj_t *self = MP_OBJ_TO_PTR(self_in);

    mp_printf(print, "DAC(%u)", self->dac_id);
}


static mp_obj_t dac_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_id, ARG_trigger, ARG_data };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,      MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_trigger, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = DAC_WRITE_DAT_TRIGGER} },
        { MP_QSTR_data,    MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint dac_id = args[ARG_id].u_int;
    if(dac_id >= PYB_NUM_DACS)
    {
        mp_raise_OSError(MP_ENODEV);
    }
    pyb_dac_obj_t *self = &pyb_dac_obj[dac_id];

    self->trigger = args[ARG_trigger].u_int;
    if((self->trigger != DAC_WRITE_DAT_TRIGGER) && (self->trigger != DAC_RISING_EDGE_TRIGGER) && (self->trigger != DAC_FALLING_EDGE_TRIGGER) &&
       (self->trigger != DAC_TIMER0_TRIGGER) && (self->trigger != DAC_TIMER1_TRIGGER) && (self->trigger != DAC_TIMER2_TRIGGER) && (DAC_TIMER3_TRIGGER))
    {
        mp_raise_ValueError("invalid trigger value");
    }

    switch(self->dac_id)
    {
    case PYB_DAC_0:
        pin_config_by_name("PB12", "PB12_DAC0_OUT");
        break;

    case PYB_DAC_1:
        pin_config_by_name("PB13", "PB13_DAC1_OUT");
        break;
    }

    CLK_EnableModuleClock(DAC_MODULE);

    DAC_Open(self->DACx, 0, self->trigger);

    DAC_SetDelayTime(self->DACx, 1);   // DAC conversion settling time is 1us

    if(self->trigger != DAC_WRITE_DAT_TRIGGER)
    {
        if((self->trigger == DAC_RISING_EDGE_TRIGGER) || (self->trigger == DAC_FALLING_EDGE_TRIGGER))
        {
            switch(self->dac_id)
            {
            case PYB_DAC_0:
                pin_config_by_name("PA10", "PA10_DAC0_ST");
                break;

            case PYB_DAC_1:
                pin_config_by_name("PA11", "PA11_DAC1_ST");
                break;
            }
        }

        if(args[ARG_data].u_obj == mp_const_none)
        {
            mp_raise_ValueError("invalid data value");
        }

        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(args[ARG_data].u_obj, &bufinfo, MP_BUFFER_READ);

        CLK_EnableModuleClock(PDMA_MODULE);

        PDMA_Open(PDMA, (1 << self->dma_chn));

        PDMA_SetBurstType(PDMA, self->dma_chn, PDMA_REQ_SINGLE, 0);

        PDMA_SetTransferAddr(PDMA, self->dma_chn, (uint32_t)bufinfo.buf, PDMA_SAR_INC, (uint32_t)&self->DACx->DAT, PDMA_DAR_FIX);

        PDMA_SetTransferCnt(PDMA, self->dma_chn, PDMA_WIDTH_16, bufinfo.len);
        PDMA_SetTransferMode(PDMA, self->dma_chn, self->dma_trigger, 0, 0);

        self->DACx->CTL |= (1 << DAC_CTL_DMAEN_Pos);
    }

    return self;    
}


static mp_obj_t dac_write(mp_obj_t self_in, mp_obj_t val)
{
    pyb_dac_obj_t *self = self_in;

    DAC_WRITE_DATA(self->DACx, 0, mp_obj_get_int(val));

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(dac_write_obj, dac_write);


static mp_obj_t dac_dma_write(mp_obj_t self_in, mp_obj_t buf)
{
    pyb_dac_obj_t *self = self_in;

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_READ);

    PDMA_SetTransferAddr(PDMA, self->dma_chn, (uint32_t)bufinfo.buf, PDMA_SAR_INC, (uint32_t)&self->DACx->DAT, PDMA_DAR_FIX);

    PDMA_SetTransferCnt(PDMA, self->dma_chn, PDMA_WIDTH_16, bufinfo.len);
    PDMA_SetTransferMode(PDMA, self->dma_chn, self->dma_trigger, 0, 0);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(dac_dma_write_obj, dac_dma_write);


static mp_obj_t dac_dma_done(mp_obj_t self_in)
{
    pyb_dac_obj_t *self = self_in;

    if(PDMA->TDSTS & (1 << self->dma_chn))
    {
        PDMA->TDSTS = (1 << self->dma_chn);

        return mp_const_true;
    }
    else
    {
        return mp_const_false;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_1(dac_dma_done_obj, dac_dma_done);


static const mp_rom_map_elem_t dac_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_write),           MP_ROM_PTR(&dac_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_dma_write),       MP_ROM_PTR(&dac_dma_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_dma_done),        MP_ROM_PTR(&dac_dma_done_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_TRIG_WRITE),      MP_ROM_INT(DAC_WRITE_DAT_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER0),     MP_ROM_INT(DAC_TIMER0_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER1),     MP_ROM_INT(DAC_TIMER1_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER2),     MP_ROM_INT(DAC_TIMER2_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER3),     MP_ROM_INT(DAC_TIMER3_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_RISING),     MP_ROM_INT(DAC_RISING_EDGE_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_FALLING),    MP_ROM_INT(DAC_FALLING_EDGE_TRIGGER) },
};
static MP_DEFINE_CONST_DICT(dac_locals_dict, dac_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    pyb_dac_type,
    MP_QSTR_DAC,
    MP_TYPE_FLAG_NONE,
    print, dac_print,
    make_new, dac_make_new,
    locals_dict, &dac_locals_dict
);
