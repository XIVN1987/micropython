#include <stdint.h>
#include <stdbool.h>

#include "py/obj.h"
#include "misc/random.h"

#include "mods/pybrtc.h"


/******************************************************************************
* LOCAL TYPES
******************************************************************************/
typedef union {
    uint32_t  id32;
    uint16_t  id16[3];
    uint8_t   id8[6];
} rng_id_t;


/******************************************************************************
* LOCAL VARIABLES
******************************************************************************/
static uint32_t s_seed;


/******************************************************************************
* PRIVATE FUNCTIONS
******************************************************************************/
STATIC uint32_t lfsr (uint32_t input)
{
    return (input >> 1) ^ (-(input & 0x01) & 0x00E10000);
}


/******************************************************************************
* PUBLIC FUNCTIONS
******************************************************************************/
void rng_init0 (void)
{
    rng_id_t juggler;

    // get the seconds and the milliseconds from the RTC
    uint32_t seconds  = rtc_get_seconds();
    uint16_t mseconds = 100;

    //wlan_get_mac (juggler.id8);
    juggler.id8[0] = 0x12;

    // flatten the 48-bit board identification to 24 bits
    juggler.id16[0] ^= juggler.id16[2];

    juggler.id8[0]  ^= juggler.id8[3];
    juggler.id8[1]  ^= juggler.id8[4];
    juggler.id8[2]  ^= juggler.id8[5];

    s_seed = juggler.id32 & 0x00FFFFFF;
    s_seed += (seconds & 0x000FFFFF) + mseconds;

    // the seed must not be zero
    if(s_seed == 0)
    {
        s_seed = 1;
    }
}


uint32_t rng_get (void) {
    s_seed = lfsr( s_seed );
    return s_seed;
}
