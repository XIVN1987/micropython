#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mperrno.h"

#include "misc/bufhelper.h"

#include "chip/M480.h"

#include "mods/pybpin.h"
#include "mods/pybadc.h"


#define ADC_NUM_SAMPLES     16
#define ADC_NUM_CHANNELS    16


/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef enum {
    PYB_ADC_0     =  0,
    PYB_NUM_ADCS
} pyb_adc_id_t;


typedef struct {
    mp_obj_base_t base;
    pyb_adc_id_t adc_id;
    EADC_T *ADCx;
    uint trigger;
    uint samprate;
    uint chns_enabled;
} pyb_adc_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
static pyb_adc_obj_t pyb_adc_obj[PYB_NUM_ADCS] = {
    { {&pyb_adc_type}, .adc_id = PYB_ADC_0, .ADCx = EADC, .chns_enabled = 0},
};


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
static void adc_channel_config(pyb_adc_obj_t *self, mp_obj_t chns_in, uint trigger)
{
    uint len;
    mp_obj_t *items;
    uint chns = 0x0000;

    mp_obj_get_array(chns_in, &len, &items);

    for(uint i = 0; i < len; i++)
    {
        if(mp_obj_is_int(items[i]))
        {
            chns |= (1 << mp_obj_get_int(items[i]));
        }
        else
        {
            mp_raise_TypeError("chns invalid");
        }
    }
    if(chns > 0xFFFF)
    {
        mp_raise_ValueError("chns invalid");
    }
    self->chns_enabled = chns;

    for(uint i = 0; i < ADC_NUM_CHANNELS; i++)
    {
        if(self->chns_enabled & (1 << i))
        {
            char pin_name[8] = {0}, af_name[32] = {0};
            snprintf(pin_name, 8, "PB%u", i);
            snprintf(af_name, 16, "PB%u_EADC0_CH%u", i, i);
            pin_config_by_name(pin_name, af_name);

            EADC_ConfigSampleModule(self->ADCx, i, trigger, i);
        }
    }
}


static void adc_trigger_config(pyb_adc_obj_t *self, uint trigger, mp_obj_t trigger_pin)
{
    if((trigger != EADC_SOFTWARE_TRIGGER) &&
       (trigger != EADC_RISING_EDGE_TRIGGER) && (trigger != EADC_FALLING_EDGE_TRIGGER) && (trigger != EADC_FALLING_RISING_EDGE_TRIGGER) &&
       (trigger != EADC_TIMER0_TRIGGER) && (trigger != EADC_TIMER1_TRIGGER) && (trigger != EADC_TIMER2_TRIGGER) && (trigger != EADC_TIMER3_TRIGGER))
    {
        mp_raise_ValueError("invalid trigger value");
    }
    self->trigger = trigger;

    if((trigger == EADC_RISING_EDGE_TRIGGER) || (trigger == EADC_FALLING_EDGE_TRIGGER) || (trigger == EADC_FALLING_RISING_EDGE_TRIGGER))
    {
        if(trigger_pin != mp_const_none)
        {
            pin_config_by_func(trigger_pin, "%s_EADC0_ST", 0);
        }
        else
        {
            mp_raise_ValueError("must specify trigger pin");
        }
    }
}


/******************************************************************************/
/* MicroPython bindings : adc object                                         */

static void adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_adc_obj_t *self = self_in;

    mp_printf(print, "ADC(%d, chns=[", self->adc_id);
    for(uint i = 0, first = 1; i < ADC_NUM_CHANNELS; i++)
    {
        if(self->chns_enabled & (1 << i))
        {
            if(first) mp_printf(print, "%d", i);
            else      mp_printf(print, ", %d", i);

            first = 0;
        }
    }
    mp_printf(print, "])");
}


static mp_obj_t adc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_id, ARG_chns, ARG_samprate, ARG_trigger, ARG_trigger_pin };
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,          MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_chns,        MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_samprate,    MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 1000000} },
        { MP_QSTR_trigger,     MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = EADC_SOFTWARE_TRIGGER} },
        { MP_QSTR_trigger_pin, MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint adc_id = args[ARG_id].u_int;
    if(adc_id >= PYB_NUM_ADCS)
    {
        mp_raise_OSError(MP_ENODEV);
    }
    pyb_adc_obj_t *self = &pyb_adc_obj[adc_id];

    self->samprate = args[ARG_samprate].u_int;
    if(self->samprate > 5000000)
    {
        mp_raise_ValueError("samprate invalid value");
    }

    uint clkdiv = SystemCoreClock / self->samprate / (2 + 12);  // samprate = SystemCoreClock / clkdiv / (2 + 12)
    if(clkdiv > 256)  clkdiv = 256;

    switch(self->adc_id)
    {
    case PYB_ADC_0:
        CLK_EnableModuleClock(EADC_MODULE);
        CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(clkdiv));
        break;

    default:
        break;
    }

    EADC_Open(self->ADCx, EADC_CTL_DIFFEN_SINGLE_END);

    adc_trigger_config(self, args[ARG_trigger].u_int, args[ARG_trigger_pin].u_obj);

    adc_channel_config(self, args[ARG_chns].u_obj, self->trigger);

    return self;
}


static mp_obj_t adc_samprate(size_t n_args, const mp_obj_t *args)
{
    pyb_adc_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->samprate);
    }
    else                // set
    {
        self->samprate = mp_obj_get_int(args[1]);
        if(self->samprate > 5000000)
        {
            mp_raise_ValueError("samprate invalid value");
        }

        uint clkdiv = SystemCoreClock / self->samprate / (2 + 12);  // samprate = SystemCoreClock / clkdiv / (2 + 12)
        if(clkdiv > 256)  clkdiv = 256;

        switch(self->adc_id)
        {
        case PYB_ADC_0:
            CLK_SetModuleClock(EADC_MODULE, 0, CLK_CLKDIV0_EADC(clkdiv));
            break;

        default:
            break;
        }

        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(adc_samprate_obj, 1, 2, adc_samprate);


static mp_obj_t adc_channel(size_t n_args, const mp_obj_t *args)
{
    pyb_adc_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        mp_obj_t items[ADC_NUM_CHANNELS];

        uint i = 0;
        for(uint j = 0; j < ADC_NUM_CHANNELS; j++)
        {
            if(self->chns_enabled & (1 << j))
            {
                items[i++] = mp_obj_new_int(j);
            }
        }
        return mp_obj_new_tuple(i, items);
    }
    else                // set
    {
        adc_channel_config(self, args[1], self->trigger);

        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(adc_channel_obj, 1, 2, adc_channel);


static mp_obj_t adc_trigger(size_t n_args, const mp_obj_t *args)
{
    pyb_adc_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->trigger);
    }
    else                // set
    {
        adc_trigger_config(self, mp_obj_get_int(args[1]), (n_args == 3) ? args[2] : mp_const_none);

        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(adc_trigger_obj, 1, 3, adc_trigger);


static mp_obj_t adc_start(mp_obj_t self_in)
{
    pyb_adc_obj_t *self = self_in;

    EADC_START_CONV(self->ADCx, self->chns_enabled);

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_1(adc_start_obj, adc_start);


static mp_obj_t adc_busy(mp_obj_t self_in)
{
    pyb_adc_obj_t *self = self_in;

    return MP_OBJ_NEW_SMALL_INT(EADC_IS_BUSY(self->ADCx));
}
static MP_DEFINE_CONST_FUN_OBJ_1(adc_busy_obj, adc_busy);


static mp_obj_t adc_any(mp_obj_t self_in, mp_obj_t chn_in)
{
    pyb_adc_obj_t *self = self_in;

    uint chn = mp_obj_get_int(chn_in);

    bool valid = self->ADCx->STATUS0 & (1 << chn);

    return mp_obj_new_bool(valid);
}
static MP_DEFINE_CONST_FUN_OBJ_2(adc_any_obj, adc_any);


static mp_obj_t adc_read(mp_obj_t self_in, mp_obj_t chn_in)
{
    pyb_adc_obj_t *self = self_in;

    uint chn = mp_obj_get_int(chn_in);

    uint val = self->ADCx->DAT[chn] & EADC_DAT_RESULT_Msk;

    return mp_obj_new_int(val);
}
static MP_DEFINE_CONST_FUN_OBJ_2(adc_read_obj, adc_read);


static const mp_rom_map_elem_t adc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_samprate),     MP_ROM_PTR(&adc_samprate_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel),      MP_ROM_PTR(&adc_channel_obj) },
    { MP_ROM_QSTR(MP_QSTR_trigger),      MP_ROM_PTR(&adc_trigger_obj) },
    { MP_ROM_QSTR(MP_QSTR_start),        MP_ROM_PTR(&adc_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_busy),         MP_ROM_PTR(&adc_busy_obj) },
    { MP_ROM_QSTR(MP_QSTR_any),          MP_ROM_PTR(&adc_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),         MP_ROM_PTR(&adc_read_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_TRIG_SW),      MP_ROM_INT(EADC_SOFTWARE_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_FALLING), MP_ROM_INT(EADC_FALLING_EDGE_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_RISING),  MP_ROM_INT(EADC_RISING_EDGE_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_EDGE),    MP_ROM_INT(EADC_FALLING_RISING_EDGE_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER0),  MP_ROM_INT(EADC_TIMER0_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER1),  MP_ROM_INT(EADC_TIMER1_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER2),  MP_ROM_INT(EADC_TIMER2_TRIGGER) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_TIMER3),  MP_ROM_INT(EADC_TIMER3_TRIGGER) },
};
static MP_DEFINE_CONST_DICT(adc_locals_dict, adc_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    pyb_adc_type,
    MP_QSTR_ADC,
    MP_TYPE_FLAG_NONE,
    print, adc_print,
    make_new, adc_make_new,
    locals_dict, &adc_locals_dict
);
