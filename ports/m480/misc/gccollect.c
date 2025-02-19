#include <stdio.h>
#include <stdint.h>

#include "py/gc.h"
#include "py/mpthread.h"

#include "chip/M480.h"

#include "shared/runtime/gchelper.h"
#include "gccollect.h"


void gc_collect(void)
{
    gc_collect_start();

    // trace the stack and registers
    gc_helper_collect_regs_and_stack();

    // trace root pointers from any threads
    #if MICROPY_PY_THREAD
    mp_thread_gc_others();
    #endif

    gc_collect_end();
}
