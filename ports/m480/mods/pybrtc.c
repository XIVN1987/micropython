#include <stdio.h>

#include "py/runtime.h"

#include "chip/M480.h"

#include "mods/pybrtc.h"

#include "shared/timeutils/timeutils.h"


/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef struct {
    mp_obj_base_t base;
} pyb_rtc_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
static pyb_rtc_obj_t pyb_rtc_obj = { {&pyb_rtc_type} };


/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/
void rtc_init(void)
{
    CLK->PWRCTL  |= CLK_PWRCTL_LXTEN_Msk;       // 32K (LXT) Enabled
    CLK->APBCLK0 |= CLK_APBCLK0_RTCCKEN_Msk;    // RTC Clock Enable

    S_RTC_TIME_DATA_T datetime;
    datetime.u32Year      = 2020;
    datetime.u32Month     = 4;
    datetime.u32Day       = 8;
    datetime.u32DayOfWeek = RTC_WEDNESDAY;
    datetime.u32Hour      = 21;
    datetime.u32Minute    = 20;
    datetime.u32Second    = 30;
    datetime.u32TimeScale = RTC_CLOCK_24;

    RTC_SetDateAndTime(&datetime);
}


uint32_t rtc_get_seconds(void)
{
    S_RTC_TIME_DATA_T datetime;

    RTC_GetDateAndTime(&datetime);

    return timeutils_seconds_since_2000(datetime.u32Year, datetime.u32Month, datetime.u32Day,
                                        datetime.u32Hour, datetime.u32Minute, datetime.u32Second);
}


/******************************************************************************/
/* MicroPython bindings                                                       */

static void rtc_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_rtc_obj_t *self = self_in;

    mp_printf(print, "RTC()");
}


static mp_obj_t rtc_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    return &pyb_rtc_obj;
}


static mp_obj_t rtc_datetime(size_t n_args, const mp_obj_t *args)
{
    if(n_args == 1) // get date and time
    {
        S_RTC_TIME_DATA_T datetime;

        RTC_GetDateAndTime(&datetime);

        mp_obj_t tuple[8];
        tuple[0] = mp_obj_new_int(datetime.u32Year);
        tuple[1] = mp_obj_new_int(datetime.u32Month);
        tuple[2] = mp_obj_new_int(datetime.u32Day);
        tuple[3] = mp_obj_new_int(datetime.u32DayOfWeek);
        tuple[4] = mp_obj_new_int(datetime.u32Hour);
        tuple[5] = mp_obj_new_int(datetime.u32Minute);
        tuple[6] = mp_obj_new_int(datetime.u32Second);
        tuple[7] = mp_obj_new_int(0);

        return mp_obj_new_tuple(8, tuple);
    }
    else            // set date and time
    {
        mp_obj_t *items;
        mp_obj_get_array_fixed_n(args[1], 8, &items);

        S_RTC_TIME_DATA_T datetime;
        datetime.u32Year      = mp_obj_get_int(items[0]);
        datetime.u32Month     = mp_obj_get_int(items[1]);
        datetime.u32Day       = mp_obj_get_int(items[2]);
        datetime.u32DayOfWeek = mp_obj_get_int(items[3]);
        datetime.u32Hour      = mp_obj_get_int(items[4]);
        datetime.u32Minute    = mp_obj_get_int(items[5]);
        datetime.u32Second    = mp_obj_get_int(items[6]);
        datetime.u32TimeScale = RTC_CLOCK_24;

        RTC_SetDateAndTime(&datetime);

        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(rtc_datetime_obj, 1, 2, rtc_datetime);


static const mp_rom_map_elem_t rtc_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_datetime), MP_ROM_PTR(&rtc_datetime_obj) },
};
static MP_DEFINE_CONST_DICT(rtc_locals_dict, rtc_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    pyb_rtc_type,
    MP_QSTR_RTC,
    MP_TYPE_FLAG_NONE,
    print, rtc_print,
    make_new, rtc_make_new,
    locals_dict, &rtc_locals_dict
);
