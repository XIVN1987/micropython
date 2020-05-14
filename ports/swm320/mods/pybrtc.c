#include "py/obj.h"
#include "py/runtime.h"

#include "chip/SWM320.h"

#include "mods/pybrtc.h"

#include "lib/timeutils/timeutils.h"


/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef struct {
    mp_obj_base_t base;
} pyb_rtc_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_rtc_obj_t pyb_rtc_obj = { {&pyb_rtc_type} };


/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/
void rtc_init(void)
{
    RTC_InitStructure RTC_initStruct;

    RTC_initStruct.Year = 2016;
    RTC_initStruct.Month = 5;
    RTC_initStruct.Date = 5;
    RTC_initStruct.Hour = 15;
    RTC_initStruct.Minute = 5;
    RTC_initStruct.Second = 5;
    RTC_initStruct.SecondIEn = 0;
    RTC_initStruct.MinuteIEn = 0;
    RTC_Init(RTC, &RTC_initStruct);

    RTC_Start(RTC);
}


uint32_t rtc_get_seconds(void)
{
    RTC_DateTime time;

    RTC_GetDateTime(RTC, &time);

    return timeutils_seconds_since_2000(time.Year, time.Month, time.Day,
                                        time.Hour, time.Minute, time.Second);
}


/******************************************************************************/
/* MicroPython bindings                                                       */

STATIC void rtc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_rtc_obj_t *self = self_in;

    mp_printf(print, "RTC()");
}


STATIC mp_obj_t rtc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    return &pyb_rtc_obj;
}


STATIC mp_obj_t rtc_datetime(size_t n_args, const mp_obj_t *args)
{
    if(n_args == 1) // get date and time
    {
        RTC_DateTime time;

        RTC_GetDateTime(RTC, &time);

        mp_obj_t tuple[8];
        tuple[0] = mp_obj_new_int(time.Year);
        tuple[1] = mp_obj_new_int(time.Month);
        tuple[2] = mp_obj_new_int(time.Date);
        tuple[3] = mp_obj_new_int(time.Day);
        tuple[4] = mp_obj_new_int(time.Hour);
        tuple[5] = mp_obj_new_int(time.Minute);
        tuple[6] = mp_obj_new_int(time.Second);
        tuple[7] = mp_obj_new_int(0);

        return mp_obj_new_tuple(8, tuple);
    }
    else            // set date and time
    {
        mp_obj_t *items;
        mp_obj_get_array_fixed_n(args[1], 8, &items);

        RTC_DateTime time;
        time.Year   = mp_obj_get_int(items[0]);
        time.Month  = mp_obj_get_int(items[1]);
        time.Date   = mp_obj_get_int(items[2]);
        time.Hour   = mp_obj_get_int(items[4]);
        time.Minute = mp_obj_get_int(items[5]);
        time.Second = mp_obj_get_int(items[6]);

        RTC_SetDateTime(RTC, &time);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(rtc_datetime_obj, 1, 2, rtc_datetime);


STATIC const mp_rom_map_elem_t rtc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_datetime), MP_ROM_PTR(&rtc_datetime_obj) },
};
STATIC MP_DEFINE_CONST_DICT(rtc_locals_dict, rtc_locals_dict_table);


const mp_obj_type_t pyb_rtc_type = {
    { &mp_type_type },
    .name = MP_QSTR_RTC,
    .print = rtc_print,
    .make_new = rtc_make_new,
    .locals_dict = (mp_obj_dict_t*)&rtc_locals_dict,
};
