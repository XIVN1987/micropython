#ifndef __MODS_PYBFLASH_H__
#define __MODS_PYBFLASH_H__

#include "py/obj.h"


extern const mp_obj_type_t pyb_flash_type;


void pyb_flash_init_vfs(fs_user_mount_t *vfs);


#endif
