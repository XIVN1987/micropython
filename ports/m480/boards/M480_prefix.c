// M480_prefix.c becomes the initial portion of the generated pins file.

#include <stdio.h>
#include <stdint.h>

#include "py/mpconfig.h"
#include "py/obj.h"

#include "chip/M480.h"

#include "pybpin.h"


#define AF(_name, _value, _mask, _reg) \
{ \
	.name  = MP_QSTR_ ## _name, \
	.value = (_value), \
    .mask  = (_mask), \
	.reg   = (_reg), \
}


#define PIN(_name, _port, _pbit, _preg, _afs, _IRQn) \
{ \
    { &pin_type }, \
    .name         = MP_QSTR_ ## _name, \
    .port         = (_port), \
    .pbit         = (_pbit), \
    .preg         = (_preg), \
    .af           = 0, \
    .afs          = _afs, \
    .afn          = sizeof(_afs) / sizeof(_afs[0]), \
    .dir          = 0, \
    .pull         = 0, \
    .IRQn         = (_IRQn), \
    .irq_trigger  = 0, \
    .irq_priority = 0, \
    .irq_callback = 0, \
}
