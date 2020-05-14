#include <stdint.h>

#include "py/obj.h"
#include "py/runtime.h"

#include "chip/M480.h"

#include "mods/pybwdt.h"


/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef struct {
    mp_obj_base_t base;
} pyb_wdt_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_wdt_obj_t pyb_wdt_obj = { {&pyb_wdt_type} };


/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/
void wdt_init(void)
{
    WDT_Open(WDT_TIMEOUT_2POW16, WDT_RESET_DELAY_1026CLK, TRUE, TRUE);
    WDT_EnableInt();
}


/******************************************************************************/
/* MicroPython bindings                                                       */

STATIC void wdt_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_wdt_obj_t *self = self_in;

    mp_printf(print, "WDT()");
}


STATIC mp_obj_t wdt_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    return &pyb_wdt_obj;
}


STATIC mp_obj_t wdt_feed(mp_obj_t self_in)
{
    WDT->RSTCNT = 0x00005AA5;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(wdt_feed_obj, wdt_feed);


STATIC const mp_rom_map_elem_t wdt_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_feed), MP_ROM_PTR(&wdt_feed_obj) },
};
STATIC MP_DEFINE_CONST_DICT(wdt_locals_dict, wdt_locals_dict_table);


const mp_obj_type_t pyb_wdt_type = {
    { &mp_type_type },
    .name = MP_QSTR_WDT,
    .print = wdt_print,
    .make_new = wdt_make_new,
    .locals_dict = (mp_obj_dict_t*)&wdt_locals_dict,
};
