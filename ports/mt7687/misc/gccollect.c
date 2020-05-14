#include <stdio.h>
#include <stdint.h>

#include "py/gc.h"
#include "py/mpstate.h"
#include "py/mpthread.h"
#include "misc/gccollect.h"
#include "lib/utils/gchelper.h"

/******************************************************************************
DECLARE PUBLIC FUNCTIONS
 ******************************************************************************/

void gc_collect(void) {
    gc_collect_start();

    mp_uint_t regs[10];
    mp_uint_t sp = gc_helper_get_regs_and_sp(regs);

    // trace the stack, including the registers (since they live on the stack in this function)
    gc_collect_root((void**)sp, ((mp_uint_t)MP_STATE_THREAD(stack_top) - sp) / sizeof(uint32_t));

    // trace root pointers from any threads
    mp_thread_gc_others();

    gc_collect_end();
}
