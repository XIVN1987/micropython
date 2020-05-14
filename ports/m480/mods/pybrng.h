#ifndef __MODS_PYBRNG_H__
#define __MODS_PYBRNG_H__

#include "py/obj.h"


void rng_init(void);
uint32_t rng_get(void);


extern const mp_obj_type_t pyb_rng_type;


#endif
