#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mphal.h"

#include "chip/M480.h"

#include "mods/pybrng.h"


/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef struct {
    mp_obj_base_t base;
} pyb_rng_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
static pyb_rng_obj_t pyb_rng_obj = { {&pyb_rng_type} };


/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/
void rng_init(void)
{
    PRNG_Open(CRPT, PRNG_KEY_SIZE_64, 1, 0xAA);
}


uint32_t rng_get(void)
{
    uint32_t data[8];

	PRNG_Start(CRPT);
    while(CRPT->PRNG_CTL & CRPT_PRNG_CTL_START_Msk) __NOP();

    PRNG_Read(CRPT, data);

    return data[0];
}


/******************************************************************************/
/* MicroPython bindings                                                       */

static void rng_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_rng_obj_t *self = self_in;

    mp_printf(print, "RNG()");
}


static mp_obj_t rng_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    return &pyb_rng_obj;
}


static mp_obj_t pyb_rng_get(mp_obj_t self_in)
{
    return mp_obj_new_int(rng_get());
}
static MP_DEFINE_CONST_FUN_OBJ_1(pyb_rng_get_obj, pyb_rng_get);


static const mp_rom_map_elem_t rng_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_get), MP_ROM_PTR(&pyb_rng_get_obj) },
};
static MP_DEFINE_CONST_DICT(rng_locals_dict, rng_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    pyb_rng_type,
    MP_QSTR_RNG,
    MP_TYPE_FLAG_NONE,
    print, rng_print,
    make_new, rng_make_new,
    locals_dict, &rng_locals_dict
);
