#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "py/mphal.h"
#include "py/runtime.h"
#include "py/mpconfig.h"

#include "chip/M480.h"

#include "lib/oofatfs/ff.h"
#include "lib/timeutils/timeutils.h"

#include "mods/pybrtc.h"



uint32_t get_fattime(void) {
    timeutils_struct_time_t tm;
    timeutils_seconds_since_2000_to_struct_time(rtc_get(), &tm);

     return ((tm.tm_year - 1980) << 25) | ((tm.tm_mon) << 21)  |
             ((tm.tm_mday) << 16)       | ((tm.tm_hour) << 11) |
             ((tm.tm_min) << 5)         | (tm.tm_sec >> 1);
}


void NORETURN __fatal_error(const char *msg) {
   while(1) __NOP();
}


void __assert_func(const char *file, int line, const char *func, const char *expr) {
    (void) func;
    (void) file;
    (void) line;
    (void) expr;
    //printf("Assertion failed: %s, file %s, line %d\n", expr, file, line);
    __fatal_error(NULL);
}


void nlr_jump_fail(void *val) {
    __fatal_error(NULL);
}
