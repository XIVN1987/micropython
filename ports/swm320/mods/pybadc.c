#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/binary.h"
#include "py/gc.h"
#include "py/mperrno.h"

#include "bufhelper.h"

#include "pybadc.h"


/******************************************************************************
 DECLARE CONSTANTS
 ******************************************************************************/
#define ADC_NUM_CHANNELS   8


/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef enum {
    PYB_ADC_0  =  0,
    PYB_ADC_1  =  1,
    PYB_NUM_ADCS
} pyb_adc_id_t;

typedef struct {
    mp_obj_base_t base;
    pyb_adc_id_t adc_id;
    ADC_TypeDef *ADCx;
    uint chns_enabled;
    uint samprate;      // sample rate
    uint trigger;
} pyb_adc_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_adc_obj_t pyb_adc_obj[PYB_NUM_ADCS] = {
    { {&pyb_adc_type}, .adc_id = PYB_ADC_0, .ADCx = ADC0, .chns_enabled = 0 },
    { {&pyb_adc_type}, .adc_id = PYB_ADC_1, .ADCx = ADC1, .chns_enabled = 0 },
};


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
STATIC void adc_pin_config(pyb_adc_obj_t *self, uint32_t chns)
{
    if(self->ADCx == ADC0)
    {
        for(uint i = 4; i < 8; i++)
        {
            if(chns & (1 << i))
            {
                switch(i)
                {
                case 4:
                    PORT_Init(PORTA, PIN12, PORTA_PIN12_ADC0_IN4, 0);
                    break;

                case 5:
                    PORT_Init(PORTA, PIN11, PORTA_PIN11_ADC0_IN5, 0);
                    break;

                case 6:
                    PORT_Init(PORTA, PIN10, PORTA_PIN10_ADC0_IN6, 0);
                    break;

                case 7:
                    PORT_Init(PORTA, PIN9,  PORTA_PIN9_ADC0_IN7,  0);
                    break;

                default:
                    break;
                }
            }
        }
    }
    else
    {
        for(uint i = 0; i < 7; i++)
        {
            if(chns & (1 << i))
            {
                switch(i)
                {
                case 0:
                    PORT_Init(PORTC, PIN7, PORTC_PIN7_ADC1_IN0, 0);
                    break;

                case 1:
                    PORT_Init(PORTC, PIN6, PORTC_PIN6_ADC1_IN1, 0);
                    break;

                case 2:
                    PORT_Init(PORTC, PIN5, PORTC_PIN5_ADC1_IN2, 0);
                    break;

                case 3:
                    PORT_Init(PORTC, PIN4, PORTC_PIN4_ADC1_IN3, 0);
                    break;

                case 4:
                    PORT_Init(PORTN, PIN0, PORTN_PIN0_ADC1_IN4, 0);
                    break;

                case 5:
                    PORT_Init(PORTN, PIN1, PORTN_PIN1_ADC1_IN5, 0);
                    break;

                case 6:
                    PORT_Init(PORTN, PIN2, PORTN_PIN2_ADC1_IN6, 0);
                    break;

                default:
                    break;
                }
            }
        }
    }
}


STATIC void adc_channel_config(pyb_adc_obj_t *self, mp_obj_t chns_in)
{
    uint len;
    mp_obj_t *items;
    uint chns = 0x00;

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
    if(chns > 0xFF)
        mp_raise_ValueError("chns invalid");

    self->chns_enabled = chns;

    adc_pin_config(self, chns);
}


/******************************************************************************/
/* MicroPython bindings : adc object                                          */

STATIC void adc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
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


STATIC mp_obj_t adc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_id, ARG_chns, ARG_samprate, ARG_trigger };
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_chns,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = MP_OBJ_NULL} },
        { MP_QSTR_samprate, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 100000} },
        { MP_QSTR_trigger,  MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = ADC_TRIGSRC_SW} },
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // check the peripheral id
    uint adc_id = args[ARG_id].u_int;
    if(adc_id >= PYB_NUM_ADCS)
    {
        mp_raise_OSError(MP_ENODEV);
    }
    pyb_adc_obj_t *self = &pyb_adc_obj[adc_id];

    adc_channel_config(self, args[ARG_chns].u_obj);

    self->samprate = args[ARG_samprate].u_int;
    if(self->samprate > 1000000)
    {
        mp_raise_ValueError("samprate invalid value");
    }

    uint clkdiv = (SystemCoreClock * 8 / 64) / self->samprate / (2 + 12);   // samprate = adc_clksrc / clkdiv / (2 + 12)
    if(clkdiv > 0x1F)  clkdiv = 0x1F;

    self->trigger = args[ARG_trigger].u_int;
    if((self->trigger != ADC_TRIGSRC_SW) && (self->trigger != ADC_TRIGSRC_PWM))
    {
        mp_raise_ValueError("trigger invalid value");
    }

    ADC_InitStructure ADC_initStruct;
    ADC_initStruct.clk_src = ADC_CLKSRC_VCO_DIV64;
    ADC_initStruct.clk_div = clkdiv;
    ADC_initStruct.pga_ref = PGA_REF_EXTERNAL;
    ADC_initStruct.channels = self->chns_enabled;
    ADC_initStruct.samplAvg = ADC_AVG_SAMPLE1;
    ADC_initStruct.trig_src = self->trigger;
    ADC_initStruct.Continue = 0;						//非连续模式，即单次模式
    ADC_initStruct.EOC_IEn = 0;
    ADC_initStruct.OVF_IEn = 0;
    ADC_initStruct.HFULL_IEn = 0;
    ADC_initStruct.FULL_IEn = 0;
    ADC_Init(self->ADCx, &ADC_initStruct);              //配置ADC
    ADC_Open(self->ADCx);								//使能ADC

    return self;
}


STATIC mp_obj_t adc_samprate(size_t n_args, const mp_obj_t *args)
{
    pyb_adc_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->samprate);
    }
    else                // set
    {
        self->samprate = mp_obj_get_int(args[1]);
        if(self->samprate > 1000000)
        {
            mp_raise_ValueError("samprate invalid value");
        }

        uint clkdiv = (SystemCoreClock * 8 / 64) / self->samprate / (2 + 12);   // samprate = adc_clksrc / clkdiv / (2 + 12)
        if(clkdiv > 0x1F)  clkdiv = 0x1F;

        self->ADCx->CTRL2 &= ~ADC_CTRL2_CLKDIV_Msk;
        self->ADCx->CTRL2 |= (clkdiv << ADC_CTRL2_CLKDIV_Pos);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(adc_samprate_obj, 1, 2, adc_samprate);


STATIC mp_obj_t adc_channel(size_t n_args, const mp_obj_t *args)
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
        adc_channel_config(self, args[1]);

        ADC_ChnSelect(self->ADCx, self->chns_enabled);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(adc_channel_obj, 1, 2, adc_channel);


STATIC mp_obj_t adc_trigger(size_t n_args, const mp_obj_t *args)
{
    pyb_adc_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->trigger);
    }
    else                // set
    {
        self->trigger = mp_obj_get_int(args[1]);

        self->ADCx->CTRL &= ~ADC_CTRL_TRIG_Msk;
        self->ADCx->CTRL |= (self->trigger << ADC_CTRL_TRIG_Pos);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(adc_trigger_obj, 1, 2, adc_trigger);


STATIC mp_obj_t adc_start(mp_obj_t self_in)
{
    pyb_adc_obj_t *self = self_in;

    self->ADCx->START = 1;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(adc_start_obj, adc_start);


STATIC mp_obj_t adc_busy(mp_obj_t self_in)
{
    pyb_adc_obj_t *self = self_in;

    return mp_obj_new_bool(self->ADCx->START & ADC_START_BUSY_Msk);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(adc_busy_obj, adc_busy);


STATIC mp_obj_t adc_any(mp_obj_t self_in, mp_obj_t chn_in)
{
    pyb_adc_obj_t *self = self_in;

    uint chn = mp_obj_get_int(chn_in);

    bool fifo_empty = self->ADCx->CH[chn].STAT & ADC_STAT_EMPTY_Msk;

    return mp_obj_new_bool(!fifo_empty);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(adc_any_obj, adc_any);


STATIC mp_obj_t adc_read(mp_obj_t self_in, mp_obj_t chn_in)
{
    pyb_adc_obj_t *self = self_in;

    uint chn = mp_obj_get_int(chn_in);

    return mp_obj_new_int(self->ADCx->CH[chn].DATA);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(adc_read_obj, adc_read);


STATIC const mp_rom_map_elem_t adc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_samprate),  MP_ROM_PTR(&adc_samprate_obj) },
    { MP_ROM_QSTR(MP_QSTR_channel),   MP_ROM_PTR(&adc_channel_obj) },
    { MP_ROM_QSTR(MP_QSTR_trigger),   MP_ROM_PTR(&adc_trigger_obj) },
    { MP_ROM_QSTR(MP_QSTR_start),     MP_ROM_PTR(&adc_start_obj) },
    { MP_ROM_QSTR(MP_QSTR_busy),      MP_ROM_PTR(&adc_busy_obj) },
    { MP_ROM_QSTR(MP_QSTR_any),       MP_ROM_PTR(&adc_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),      MP_ROM_PTR(&adc_read_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_TRIG_SW),   MP_ROM_INT(ADC_TRIGSRC_SW) },
    { MP_ROM_QSTR(MP_QSTR_TRIG_PWM),  MP_ROM_INT(ADC_TRIGSRC_PWM) },
};
STATIC MP_DEFINE_CONST_DICT(adc_locals_dict, adc_locals_dict_table);


const mp_obj_type_t pyb_adc_type = {
    { &mp_type_type },
    .name = MP_QSTR_ADC,
    .print = adc_print,
    .make_new = adc_make_new,
    .locals_dict = (mp_obj_t)&adc_locals_dict,
};
