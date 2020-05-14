#include <stdint.h>
#include <string.h>

#include "py/objtuple.h"
#include "py/objstr.h"
#include "py/runtime.h"
#include "lib/timeutils/timeutils.h"
#include "lib/oofatfs/ff.h"
#include "lib/oofatfs/diskio.h"
#include "genhdr/mpversion.h"

#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#include "misc/random.h"
#include "mods/pybuart.h"


/// \module os - basic "operating system" services
///
/// The `os` module contains functions for filesystem access and `urandom`.
///
/// The filesystem has `/` as the root directory, and the available physical
/// drives are accessible from here.  They are currently:
///
///     /flash      -- the flash filesystem
///
/// On boot up, the current directory is `/flash`.


/******************************************************************************/
// MicroPython bindings
//

STATIC const qstr os_uname_info_fields[] = {
    MP_QSTR_sysname, MP_QSTR_release, MP_QSTR_version, MP_QSTR_machine
};

STATIC const MP_DEFINE_STR_OBJ(os_uname_info_sysname_obj, MICROPY_HW_BOARD_NAME);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_release_obj, MICROPY_SW_VERSION_NUMBER);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_version_obj, MICROPY_GIT_TAG " on " MICROPY_BUILD_DATE);
STATIC const MP_DEFINE_STR_OBJ(os_uname_info_machine_obj, MICROPY_HW_BOARD_NAME " with " MICROPY_HW_MCU_NAME);
STATIC MP_DEFINE_ATTRTUPLE(
    os_uname_info_obj,
    os_uname_info_fields,
    5,
    (mp_obj_t)&os_uname_info_sysname_obj,
    (mp_obj_t)&os_uname_info_release_obj,
    (mp_obj_t)&os_uname_info_version_obj,
    (mp_obj_t)&os_uname_info_machine_obj
);


STATIC mp_obj_t os_uname(void)
{
    return (mp_obj_t)&os_uname_info_obj;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(os_uname_obj, os_uname);


STATIC mp_obj_t os_sync(void)
{
//    sflash_disk_flush();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(os_sync_obj, os_sync);


STATIC mp_obj_t os_urandom(mp_obj_t num)
{
    mp_int_t n = mp_obj_get_int(num);
    vstr_t vstr;
    vstr_init_len(&vstr, n);
    for(uint i = 0; i < n; i++)
    {
        vstr.buf[i] = rng_get();
    }
    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(os_urandom_obj, os_urandom);


bool mp_uos_dupterm_is_builtin_stream(mp_const_obj_t stream)
{
    const mp_obj_type_t *type = mp_obj_get_type(stream);

    return (type == &pyb_uart_type);
}


STATIC mp_obj_t os_dupterm(size_t n_args, const mp_obj_t *args)
{
    if(n_args == 0)
    {
        return MP_STATE_VM(dupterm_objs[0]);
    }
    else
    {
        MP_STATE_VM(dupterm_objs[0]) = args[0];

        return mp_const_none;
    }
}
MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(os_dupterm_obj, 0, 1, os_dupterm);


STATIC const mp_rom_map_elem_t os_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__),        MP_ROM_QSTR(MP_QSTR_uos) },

    { MP_ROM_QSTR(MP_QSTR_uname),           MP_ROM_PTR(&os_uname_obj) },

    { MP_ROM_QSTR(MP_QSTR_chdir),           MP_ROM_PTR(&mp_vfs_chdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_getcwd),          MP_ROM_PTR(&mp_vfs_getcwd_obj) },
    { MP_ROM_QSTR(MP_QSTR_ilistdir),        MP_ROM_PTR(&mp_vfs_ilistdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_listdir),         MP_ROM_PTR(&mp_vfs_listdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_mkdir),           MP_ROM_PTR(&mp_vfs_mkdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_rename),          MP_ROM_PTR(&mp_vfs_rename_obj) },
    { MP_ROM_QSTR(MP_QSTR_remove),          MP_ROM_PTR(&mp_vfs_remove_obj) },
    { MP_ROM_QSTR(MP_QSTR_rmdir),           MP_ROM_PTR(&mp_vfs_rmdir_obj) },
    { MP_ROM_QSTR(MP_QSTR_stat),            MP_ROM_PTR(&mp_vfs_stat_obj) },
    { MP_ROM_QSTR(MP_QSTR_unlink),          MP_ROM_PTR(&mp_vfs_remove_obj) },     // unlink aliases to remove

    { MP_ROM_QSTR(MP_QSTR_sync),            MP_ROM_PTR(&os_sync_obj) },
    { MP_ROM_QSTR(MP_QSTR_urandom),         MP_ROM_PTR(&os_urandom_obj) },

    // MicroPython additions
    // removed: mkfs
    // renamed: unmount -> umount
    { MP_ROM_QSTR(MP_QSTR_mount),           MP_ROM_PTR(&mp_vfs_mount_obj) },
    { MP_ROM_QSTR(MP_QSTR_umount),          MP_ROM_PTR(&mp_vfs_umount_obj) },
    { MP_ROM_QSTR(MP_QSTR_VfsFat),          MP_ROM_PTR(&mp_fat_vfs_type) },
    { MP_ROM_QSTR(MP_QSTR_dupterm),         MP_ROM_PTR(&os_dupterm_obj) },
};
STATIC MP_DEFINE_CONST_DICT(os_module_globals, os_module_globals_table);


const mp_obj_module_t mp_module_uos = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&os_module_globals,
};
