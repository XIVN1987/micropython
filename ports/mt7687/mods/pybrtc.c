#include "py/obj.h"
#include "py/runtime.h"

#include "chip/MT7687.h"

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
STATIC const pyb_rtc_obj_t pyb_rtc_obj = { {&pyb_rtc_type} };


/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/
void rtc_init(void)
{
    RTC_Init();
}


uint32_t rtc_get_seconds(void)
{
    RTC_TimeStruct time;

    RTC_TimeGet(&time);

    return timeutils_seconds_since_2000(time.year, time.month, time.day,
                                        time.hour, time.minute, time.second);
}


/******************************************************************************/
/* MicroPython bindings                                                       */

STATIC mp_obj_t pyb_rtc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    return &pyb_rtc_obj;
}


STATIC mp_obj_t pyb_rtc_datetime(size_t n_args, const mp_obj_t *args)
{
    if(n_args == 1) // get date and time
    {
        RTC_TimeStruct time;

        RTC_TimeGet(&time);

        mp_obj_t tuple[8];
        tuple[0] = mp_obj_new_int(time.year);
        tuple[1] = mp_obj_new_int(time.month);
        tuple[2] = mp_obj_new_int(time.day);
        tuple[3] = mp_obj_new_int(time.dayOfWeek);
        tuple[4] = mp_obj_new_int(time.hour);
        tuple[5] = mp_obj_new_int(time.minute);
        tuple[6] = mp_obj_new_int(time.second);
        tuple[7] = mp_obj_new_int(0);

        return mp_obj_new_tuple(8, tuple);
    }
    else            // set date and time
    {
        mp_obj_t *items;
        mp_obj_get_array_fixed_n(args[1], 8, &items);

        RTC_TimeStruct time;
        time.year      = mp_obj_get_int(items[0]);
        time.month     = mp_obj_get_int(items[1]);
        time.day       = mp_obj_get_int(items[2]);
        time.dayOfWeek = mp_obj_get_int(items[3]);
        time.hour      = mp_obj_get_int(items[4]);
        time.minute    = mp_obj_get_int(items[5]);
        time.second    = mp_obj_get_int(items[6]);

        RTC_TimeSet(&time);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(pyb_rtc_datetime_obj, 1, 2, pyb_rtc_datetime);


STATIC const mp_rom_map_elem_t pyb_rtc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_datetime), MP_ROM_PTR(&pyb_rtc_datetime_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_rtc_locals_dict, pyb_rtc_locals_dict_table);


const mp_obj_type_t pyb_rtc_type = {
    { &mp_type_type },
    .name = MP_QSTR_RTC,
    .make_new = pyb_rtc_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_rtc_locals_dict,
};
