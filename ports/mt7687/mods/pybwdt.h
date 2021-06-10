#ifndef __MODS_PYBWDT_H__
#define __MODS_PYBWDT_H__

#include "py/obj.h"


void wdt_init(void);


extern const mp_obj_type_t pyb_wdt_type;


#endif
