#include <stdio.h>
#include <stdint.h>

#include "py/gc.h"
#include "py/mpthread.h"

#include "chip/M480.h"

#include "gccollect.h"


void gc_collect(void)
{
    gc_collect_start();

    mp_uint_t sp = __get_PSP();

    gc_collect_root((void**)sp, ((uint32_t)&__StackTop - sp) / sizeof(uint32_t));

    gc_collect_end();
}
