#ifndef __MODS_PYBPIN_H__
#define __MODS_PYBPIN_H__


typedef struct {
    const mp_obj_base_t base;
    const qstr          name;
    const uint8_t       index;
    uint8_t             alt;
    uint8_t             dir;
    uint8_t             mode;
    uint8_t             value;
    uint8_t             irq_trigger;
    uint8_t             irq_flags;
} pin_obj_t;

#include "pins.h"

extern const mp_obj_type_t pyb_pin_type;

extern const mp_obj_dict_t pins_locals_dict;

#endif
