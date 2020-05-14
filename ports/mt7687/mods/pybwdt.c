#include <stdint.h>

#include "py/obj.h"
#include "py/runtime.h"

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
STATIC const pyb_wdt_obj_t pyb_wdt_obj = { {&pyb_wdt_type} };


/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/
void wdt_init(void)
{
    WDT_Init(WDT_MODE_INTERRUPT, 1000); // 32ms * 1000
    WDT_Start();
}


/******************************************************************************/
/* MicroPython bindings                                                       */

STATIC mp_obj_t pyb_wdt_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    return &pyb_wdt_obj;
}


STATIC mp_obj_t pyb_wdt_feed(mp_obj_t self_in)
{
    WDT_Feed();

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_wdt_feed_obj, pyb_wdt_feed);


STATIC const mp_rom_map_elem_t pyb_wdt_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_feed), MP_ROM_PTR(&pyb_wdt_feed_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_wdt_locals_dict, pyb_wdt_locals_dict_table);


const mp_obj_type_t pyb_wdt_type = {
    { &mp_type_type },
    .name = MP_QSTR_WDT,
    .make_new = pyb_wdt_make_new,
    .locals_dict = (mp_obj_dict_t*)&pyb_wdt_locals_dict,
};
