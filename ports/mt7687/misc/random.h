#ifndef __MISC_RANDOM_H__
#define __MISC_RANDOM_H__

void rng_init0 (void);
uint32_t rng_get (void);

MP_DECLARE_CONST_FUN_OBJ_0(pyb_rng_get_obj);

#endif
