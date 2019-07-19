/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Daniel Campora
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "py/mpconfig.h"
#include "py/stackctrl.h"
#include "py/mphal.h"
#include "py/obj.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "mpirq.h"
#include "mperror.h"
#include "task.h"
#include "gchelper.h"
#include "lib/utils/pyexec.h"
#include "lib/mp-readline/readline.h"
#include "gccollect.h"

#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "lib/utils/pyexec.h"


#include "lib/oofatfs/ff.h"
#include "lib/oofatfs/diskio.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#include "pybuart.h"
#include "pybflash.h"


static const char fresh_main_py[] = "# main.py -- put your code here!\r\n";
static const char fresh_boot_py[] = "# boot.py -- run on boot-up\r\n"
                                    "# can run arbitrary Python, but best to keep it minimal\r\n"
                                    #if MICROPY_STDIO_UART
                                    "import os, machine\r\n"
                                    "os.dupterm(machine.UART(1, 115200))\r\n"
                                    #endif
                                    ;


void TASK_MicroPython (void *pvParameters);
STATIC void init_sflash_filesystem (void);

// This is the static memory (TCB and stack) for the idle task
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

#define MICROPY_TASK_PRIORITY                   (2)
#define MICROPY_TASK_STACK_SIZE                 (16 * 1024) // in bytes
#define MICROPY_TASK_STACK_LEN                  (MICROPY_TASK_STACK_SIZE / sizeof(StackType_t))


// This is the static memory (TCB and stack) for the main MicroPython task
StaticTask_t mpTaskTCB;
StackType_t mpTaskStack[MICROPY_TASK_STACK_LEN];


int main (void) {
//    PORT_Init(GPIO35, GPIO35_GPIO);
//    GPIO_Init(GPIO35, GPIO_DIR_OUT);
//    PORT_Init(GPIO38, GPIO38_GPIO);
//    GPIO_Init(GPIO38, GPIO_DIR_OUT);
//    GPIO_ClrBit(GPIO38);
//    GPIO_SetBit(GPIO35);
//    for(uint i = 0; i < SystemCoreClock; i++) __NOP();
//    GPIO_ClrBit(GPIO35);
//    for(uint i = 0; i < 19200000; i++) __NOP();

    xTaskCreateStatic(TASK_MicroPython, "MicroPy",
        MICROPY_TASK_STACK_LEN, NULL, MICROPY_TASK_PRIORITY, mpTaskStack, &mpTaskTCB);

    vTaskStartScheduler();

   while(true);
}


void TASK_MicroPython (void *pvParameters) {

    // get the top of the stack to initialize the garbage collector
    uint32_t sp = gc_helper_get_sp();

soft_reset:

    mp_thread_init();

    // initialise the stack pointer for the main thread (must be done after mp_thread_init)
    mp_stack_set_top((void*)sp);

    gc_init(&__heap_start__, &__heap_end__);

    // MicroPython init
    mp_init();
    mp_obj_list_init(mp_sys_path, 0);
    mp_obj_list_init(mp_sys_argv, 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_)); // current dir (or base dir of the script)

    mp_irq_init0();
    mperror_init0();
    uart_init0();
    readline_init0();

    // initialize the serial flash file system
    init_sflash_filesystem();

    // append the flash paths to the system path
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash));
//    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash_slash_lib));

    // run boot.py
    int ret = pyexec_file("boot.py");
    if (ret & PYEXEC_FORCED_EXIT) {
        goto soft_reset_exit;
    }
    if (!ret) {
        mperror_signal_error();     // flash the system led
    }

    // at this point everything is fully configured and initialised.

    // run main.py
    ret = pyexec_file("main.py");
    if (ret & PYEXEC_FORCED_EXIT) {
        goto soft_reset_exit;
    }
    if (!ret) {
        mperror_signal_error();     // flash the system led
    }

    // main script is finished, so now go into REPL mode.
    // the REPL mode can change, or it can request a soft reset.
    for ( ; ; ) {
        if (pyexec_mode_kind == PYEXEC_MODE_RAW_REPL) {
            if (pyexec_raw_repl() != 0) {
                break;
            }
        } else {
            if (pyexec_friendly_repl() != 0) {
                break;
            }
        }
    }

soft_reset_exit:
    mp_printf(&mp_plat_print, "PYB: soft reboot\n");

    mp_hal_delay_ms(20);

    goto soft_reset;
}

fs_user_mount_t _vfs_fat;

STATIC void init_sflash_filesystem (void) {
    FILINFO fno;

    // Initialise the local flash filesystem.
    // init the vfs object
    fs_user_mount_t *vfs_fat = &_vfs_fat;
    vfs_fat->flags = 0;
    pyb_flash_init_vfs(vfs_fat);

    // Create it if needed, and mount in on /flash.
    FRESULT res = f_mount(&vfs_fat->fatfs);
    if (res == FR_NO_FILESYSTEM) {
        // no filesystem, so create a fresh one
        uint8_t working_buf[_MAX_SS];
        res = f_mkfs(&vfs_fat->fatfs, FM_FAT | FM_SFD, 0, working_buf, sizeof(working_buf));
        if (res != FR_OK) {
            __fatal_error("failed to create /flash");
        }
    } else if (res != FR_OK) {
        __fatal_error("failed to create /flash");
    }

    // mount the flash device (there should be no other devices mounted at this point)
    // we allocate this structure on the heap because vfs->next is a root pointer
    mp_vfs_mount_t *vfs = m_new_obj_maybe(mp_vfs_mount_t);
    if(vfs == NULL) __fatal_error("failed to mount /flash");
    vfs->str = "/flash";
    vfs->len = 6;
    vfs->obj = MP_OBJ_FROM_PTR(vfs_fat);
    vfs->next = NULL;
    MP_STATE_VM(vfs_mount_table) = vfs;

    MP_STATE_PORT(vfs_cur) = vfs;

    f_chdir(&vfs_fat->fatfs, "/");

    // make sure we have a /flash/boot.py.  Create it if needed.
    res = f_stat(&vfs_fat->fatfs, "/boot.py", &fno);
    if (res != FR_OK) {
        // doesn't exist, create fresh file
        FIL fp;
        f_open(&vfs_fat->fatfs, &fp, "/boot.py", FA_WRITE | FA_CREATE_ALWAYS);
        UINT n;
        f_write(&fp, fresh_boot_py, sizeof(fresh_boot_py) - 1 /* don't count null terminator */, &n);
        f_close(&fp);
    }

    res = f_stat(&vfs_fat->fatfs, "/main.py", &fno);
    if (res != FR_OK) {
        // doesn't exist, create fresh file
        FIL fp;
        f_open(&vfs_fat->fatfs, &fp, "/main.py", FA_WRITE | FA_CREATE_ALWAYS);
        UINT n;
        f_write(&fp, fresh_main_py, sizeof(fresh_main_py) - 1 /* don't count null terminator */, &n);
        f_close(&fp);
    }
}


// We need this when configSUPPORT_STATIC_ALLOCATION is enabled
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                       StackType_t **ppxIdleTaskStackBuffer,
                                       uint32_t *pulIdleTaskStackSize ) {
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

