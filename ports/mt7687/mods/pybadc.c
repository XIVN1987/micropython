#include <stdio.h>
#include <string.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mperrno.h"

#include "misc/bufhelper.h"

#include "mods/pybadc.h"


/******************************************************************************
 DECLARE CONSTANTS
 ******************************************************************************/
#define ADC_NUM_CHANNELS    4


/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef enum {
    PYB_ADC_0  =  0,
    PYB_NUM_ADCS
} pyb_adc_id_t;


typedef struct {
    mp_obj_base_t base;
    pyb_adc_id_t adc_id;
    ADC_TypeDef *ADCx;
    bool chn_enabled[ADC_NUM_CHANNELS];
} pyb_adc_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_adc_obj_t pyb_adc_obj[PYB_NUM_ADCS] = { {.adc_id = PYB_ADC_0, .ADCx = ADC, .chn_enabled = {0}} };


/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/
void adc_chn_select(pyb_adc_obj_t *self, uint32_t chns)
{
    ADC_ChnlSelect(self->ADCx, chns);

    for(uint32_t i = 0; i < ADC_NUM_CHANNELS; i++)
    {
        if(chns & (1 << i))
        {
            switch((uint32_t)self->ADCx)
            {
            case (uint32_t)ADC:
                switch(i)
                {
                case 0:
                    //PORT_Init(PORTA, PIN12, PORTA_PIN12_ADC0_IN4, 0);
                    break;

                case 1:
                    //PORT_Init(PORTA, PIN11, PORTA_PIN11_ADC0_IN5, 0);
                    break;

                case 2:
                    //PORT_Init(PORTA, PIN10, PORTA_PIN10_ADC0_IN6, 0);
                    break;

                case 3:
                    //PORT_Init(PORTA, PIN9,  PORTA_PIN9_ADC0_IN7,  0);
                    break;

                default:
                    break;
                }
                break;

            default:
                break;
            }

            self->chn_enabled[i] = true;
        }
        else
        {
            self->chn_enabled[i] = false;
        }
    }
}


/******************************************************************************/
/* MicroPython bindings : adc object                                         */

STATIC void adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_adc_obj_t *self = self_in;

    mp_printf(print, "ADC(%d, chns: [", self->adc_id);
    for(uint32_t i = 0, first = 1; i < ADC_NUM_CHANNELS; i++)
    {
        if(self->chn_enabled[i])
        {
            if(first) mp_printf(print, "%d", i);
            else      mp_printf(print, ", %d", i);

            first = 0;
        }
    }
    mp_printf(print, "])");
}


STATIC const mp_arg_t pyb_adc_init_args[] = {
    { MP_QSTR_id,         MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_chns,       MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_mode,       MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = ADC_MODE_ONE_TIME} },
    { MP_QSTR_interval,   MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 1000000} },
    { MP_QSTR_samplAVG,   MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = ADC_AVG_1SAMPLE} }
};
STATIC mp_obj_t adc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_adc_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(args), pyb_adc_init_args, args);

    // check the peripheral id
    uint32_t adc_id = args[0].u_int;
    if(adc_id > PYB_NUM_ADCS - 1) {
        mp_raise_OSError(MP_ENODEV);
    }

    // setup the object
    pyb_adc_obj_t *self = &pyb_adc_obj[adc_id];
    self->base.type = &pyb_adc_type;

    ADC_InitStructure ADC_initStruct;

    ADC_initStruct.Mode = args[2].u_int;
    ADC_initStruct.Channels = args[1].u_int;
    ADC_initStruct.SamplAvg = args[4].u_int;
    ADC_initStruct.Interval = args[3].u_int;
    ADC_Init(self->ADCx, &ADC_initStruct);              //配置ADC

    adc_chn_select(self, ADC_initStruct.Channels);

    return self;
}


STATIC mp_obj_t pyb_adc_start(mp_obj_t self_in) {
    pyb_adc_obj_t *self = self_in;

    ADC_Start(self->ADCx);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_adc_start_obj, pyb_adc_start);


STATIC mp_obj_t pyb_adc_stop(mp_obj_t self_in) {
    pyb_adc_obj_t *self = self_in;

    ADC_Stop(self->ADCx);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_adc_stop_obj, pyb_adc_stop);


STATIC mp_obj_t pyb_adc_dataAvailable(mp_obj_t self_in) {
    pyb_adc_obj_t *self = self_in;

    uint32_t cnt = ADC_DataAvailable(self->ADCx);

    return mp_obj_new_bool(cnt);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_adc_dataAvailable_obj, pyb_adc_dataAvailable);


STATIC mp_obj_t pyb_adc_read(mp_obj_t self_in) {
    pyb_adc_obj_t *self = self_in;

    uint32_t chnum;
    uint32_t value = ADC_Read(self->ADCx, &chnum);

    mp_obj_t datas[2];
    datas[0] = mp_obj_new_int(value);
    datas[1] = mp_obj_new_int(chnum);

    return mp_obj_new_tuple(2, datas);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_adc_read_obj, pyb_adc_read);


STATIC mp_obj_t pyb_adc_chn_select(mp_obj_t self_in, mp_obj_t chns_in)
{
    pyb_adc_obj_t *self = self_in;

    uint32_t chns = mp_obj_get_int(chns_in);

    adc_chn_select(self, chns);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_adc_chn_select_obj, pyb_adc_chn_select);


STATIC const mp_rom_map_elem_t adc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_start),         MP_ROM_PTR(&pyb_adc_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_stop),          MP_ROM_PTR(&pyb_adc_stop_obj) },
    { MP_ROM_QSTR(MP_QSTR_dataAvailable), MP_ROM_PTR(&pyb_adc_dataAvailable_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),          MP_ROM_PTR(&pyb_adc_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_chn_select),    MP_ROM_PTR(&pyb_adc_chn_select_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_ADC_CH0),       MP_ROM_INT(ADC_CH0) },
    { MP_ROM_QSTR(MP_QSTR_ADC_CH1),       MP_ROM_INT(ADC_CH1) },
    { MP_ROM_QSTR(MP_QSTR_ADC_CH2),       MP_ROM_INT(ADC_CH2) },
    { MP_ROM_QSTR(MP_QSTR_ADC_CH3),       MP_ROM_INT(ADC_CH3) },
    { MP_ROM_QSTR(MP_QSTR_ONE_TIME),      MP_ROM_INT(ADC_MODE_ONE_TIME) },
    { MP_ROM_QSTR(MP_QSTR_PERIODIC),      MP_ROM_INT(ADC_MODE_PERIODIC) },
    { MP_ROM_QSTR(MP_QSTR_AVG_1SAMPLE),   MP_ROM_INT(ADC_AVG_1SAMPLE) },
    { MP_ROM_QSTR(MP_QSTR_AVG_2SAMPLE),   MP_ROM_INT(ADC_AVG_2SAMPLE) },
    { MP_ROM_QSTR(MP_QSTR_AVG_4SAMPLE),   MP_ROM_INT(ADC_AVG_4SAMPLE) },
    { MP_ROM_QSTR(MP_QSTR_AVG_8SAMPLE),   MP_ROM_INT(ADC_AVG_8SAMPLE) },
    { MP_ROM_QSTR(MP_QSTR_AVG_16SAMPLE),  MP_ROM_INT(ADC_AVG_16SAMPLE) },
    { MP_ROM_QSTR(MP_QSTR_AVG_32SAMPLE),  MP_ROM_INT(ADC_AVG_32SAMPLE) },
    { MP_ROM_QSTR(MP_QSTR_AVG_64SAMPLE),  MP_ROM_INT(ADC_AVG_64SAMPLE) },
};

STATIC MP_DEFINE_CONST_DICT(adc_locals_dict, adc_locals_dict_table);

const mp_obj_type_t pyb_adc_type = {
    { &mp_type_type },
    .name = MP_QSTR_ADC,
    .print = adc_print,
    .make_new = adc_make_new,
    .locals_dict = (mp_obj_t)&adc_locals_dict,
};
