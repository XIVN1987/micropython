#include "py/obj.h"
#include "shared/timeutils/timeutils.h"

#include "pybrtc.h"


// Return the localtime as an 8-tuple.
static mp_obj_t mp_time_localtime_get(void) {
    timeutils_struct_time_t tm;

    // get the seconds from the RTC
    timeutils_seconds_since_2000_to_struct_time(rtc_get_seconds(), &tm);
    mp_obj_t tuple[8] = {
            mp_obj_new_int(tm.tm_year),
            mp_obj_new_int(tm.tm_mon),
            mp_obj_new_int(tm.tm_mday),
            mp_obj_new_int(tm.tm_hour),
            mp_obj_new_int(tm.tm_min),
            mp_obj_new_int(tm.tm_sec),
            mp_obj_new_int(tm.tm_wday),
            mp_obj_new_int(tm.tm_yday)
    };
    return mp_obj_new_tuple(8, tuple);
}

// Returns the number of seconds, as an integer, since the Epoch.
static mp_obj_t mp_time_time_get(void) {
    return mp_obj_new_int(rtc_get_seconds());
}
