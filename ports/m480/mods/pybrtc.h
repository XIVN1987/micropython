#ifndef __MODS_PYBRTC_H__
#define __MODS_PYBRTC_H__

#include "py/obj.h"


void rtc_init(void);
uint32_t rtc_get_seconds(void);


extern const mp_obj_type_t pyb_rtc_type;


#endif
