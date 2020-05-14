#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/gc.h"
#include "misc/gccollect.h"

#include "lib/utils/pyexec.h"
#include "lib/mp-readline/readline.h"
#include "lib/timeutils/timeutils.h"

#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#include "mods/pybuart.h"
#include "mods/pybflash.h"
#include "mods/pybrtc.h"


static const char fresh_main_py[] = "# main.py -- put your code here!\r\n";
static const char fresh_boot_py[] = "# boot.py -- run on boot-up\r\n"
                                    "# can run arbitrary Python, but best to keep it minimal\r\n"
                                    "import os, machine\r\n"
                                    "os.dupterm(machine.UART(0, 115200, txd='PA3', rxd='PA2'))\r\n"
                                    ;

void init_flash_filesystem(void);

int main (void)
{
    SystemInit();

    SysTick_Config(SystemCoreClock / 1000);
    NVIC_SetPriority(SysTick_IRQn, 3);  // 优先级高于它的ISR中不能使用 mp_hal_delay_us() 等延时相关函数

soft_reset:

    gc_init(&__HeapBase, &__HeapLimit);

    mp_stack_set_top((char*)&__StackTop);
    mp_stack_set_limit((char*)&__StackTop - (char*)&__StackLimit - 1024);

    // MicroPython init
    mp_init();
    mp_obj_list_init(mp_sys_path, 0);
    mp_obj_list_init(mp_sys_argv, 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_)); // current dir (or base dir of the script)

    uart_init0();
    readline_init0();

    init_flash_filesystem();

    // append the flash paths to the system path
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash));
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash_slash_lib));

    // run boot.py
    int ret = pyexec_file("boot.py");
    if(ret & PYEXEC_FORCED_EXIT)
    {
        goto soft_reset_exit;
    }

    // run main.py
    ret = pyexec_file("main.py");
    if(ret & PYEXEC_FORCED_EXIT)
    {
        goto soft_reset_exit;
    }

    // main script is finished, so now go into REPL mode.
    while(1)
    {
        if(pyexec_mode_kind == PYEXEC_MODE_RAW_REPL)
        {
            if(pyexec_raw_repl() != 0)
            {
                break;
            }
        }
        else
        {
            if(pyexec_friendly_repl() != 0)
            {
                break;
            }
        }
    }

soft_reset_exit:

    printf("MPY: soft reboot\n");

    // do some deinit

    goto soft_reset;
}


void init_flash_filesystem(void)
{
    /* Initialise the local flash filesystem */
    static fs_user_mount_t vfs_fat;

    vfs_fat.blockdev.flags = 0;
    pyb_flash_init_vfs(&vfs_fat);

    FRESULT res = f_mount(&vfs_fat.fatfs);
    if(res == FR_NO_FILESYSTEM)
    {
        // no filesystem, so create a fresh one
        uint8_t working_buf[FF_MAX_SS];
        res = f_mkfs(&vfs_fat.fatfs, FM_FAT | FM_SFD, 0, working_buf, sizeof(working_buf));
        if(res != FR_OK)
        {
            printf("failed to create /flash");
            while(1) __NOP();
        }
    }

    /* mount the flash device */
    // we allocate this structure on the heap because vfs->next is a root pointer
    mp_vfs_mount_t *vfs = m_new_obj_maybe(mp_vfs_mount_t);

    vfs->str = "/flash";
    vfs->len = 6;
    vfs->obj = MP_OBJ_FROM_PTR(&vfs_fat);
    vfs->next = NULL;
    MP_STATE_VM(vfs_mount_table) = vfs;

    MP_STATE_PORT(vfs_cur) = vfs;

    f_chdir(&vfs_fat.fatfs, "/");

    /* create boot.py and main.py */
    FILINFO fno;
    FIL fp;
    UINT n;

    res = f_stat(&vfs_fat.fatfs, "/boot.py", &fno);
    if(res != FR_OK)
    {
        f_open(&vfs_fat.fatfs, &fp, "/boot.py", FA_WRITE | FA_CREATE_ALWAYS);
        f_write(&fp, fresh_boot_py, sizeof(fresh_boot_py) - 1 /* don't count null terminator */, &n);
        f_close(&fp);
    }

    res = f_stat(&vfs_fat.fatfs, "/main.py", &fno);
    if(res != FR_OK)
    {
        f_open(&vfs_fat.fatfs, &fp, "/main.py", FA_WRITE | FA_CREATE_ALWAYS);
        f_write(&fp, fresh_main_py, sizeof(fresh_main_py) - 1 /* don't count null terminator */, &n);
        f_close(&fp);
    }

    return;
}


DWORD get_fattime(void)
{
    timeutils_struct_time_t tm;
    timeutils_seconds_since_2000_to_struct_time(rtc_get_seconds(), &tm);

    return ((tm.tm_year - 1980) << 25) | ((tm.tm_mon) << 21)  | ((tm.tm_mday) << 16) |
            ((tm.tm_hour) << 11) | ((tm.tm_min) << 5) | (tm.tm_sec >> 1);
}


void NORETURN __fatal_error(const char *msg)
{
   while(1) __NOP();
}

void __assert_func(const char *file, int line, const char *func, const char *expr)
{
    printf("Assertion failed: %s, file %s, line %d\n", expr, file, line);
    __fatal_error(NULL);
}

void nlr_jump_fail(void *val)
{
    __fatal_error(NULL);
}
