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
#include "py/compile.h"
#include "py/runtime.h"
#include "py/repl.h"
#include "py/gc.h"

#include "lib/utils/pyexec.h"
#include "lib/mp-readline/readline.h"

#include "lib/oofatfs/ff.h"
#include "lib/oofatfs/diskio.h"
#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#include "FreeRTOS.h"
#include "task.h"

#include "chip/M480.h"

#include "mods/pybrtc.h"
#include "mods/pybuart.h"
#include "mods/pybflash.h"
#include "mods/pybusb_vcom.h"

#include "misc/gccollect.h"


static const char fresh_main_py[] = "# main.py -- put your code here!\r\n";
static const char fresh_boot_py[] = "# boot.py -- run on boot-up\r\n"
                                    "# can run arbitrary Python, but best to keep it minimal\r\n"
                                    #ifdef MICROPY_STDIO_UART
                                    "import os, machine\r\n"
                                    "os.dupterm(machine.UART(0, 115200, rxd='PA0', txd='PA1'))\r\n"
                                    #endif
                                    ;


void systemInit(void);
void USB_Config(void);
void Task_MicroPy(void *argv);
void init_sflash_filesystem(void);


#define TAKS_MICROPY_PRIORITY   10
static StaticTask_t Task_MicroPy_TCB;
#define TASK_MICROPY_STK_DEPTH  (1024 * 16 / sizeof(StackType_t))
static StackType_t  Task_MicroPy_Stk[TASK_MICROPY_STK_DEPTH];

int main(void)
{
    systemInit();

    USB_Config();

    xTaskCreateStatic(Task_MicroPy, "MicroPy", TASK_MICROPY_STK_DEPTH, NULL,
                      TAKS_MICROPY_PRIORITY, Task_MicroPy_Stk, &Task_MicroPy_TCB);

    vTaskStartScheduler();
}


void Task_MicroPy(void *argv)
{
    uint sp = __get_PSP();

soft_reset:
    mp_stack_set_top((void*)sp);
    mp_stack_set_limit(sizeof(Task_MicroPy_Stk) - 1024);

    gc_init(&__HeapBase, &__HeapLimit);

    // MicroPython init
    mp_init();
    mp_obj_list_init(mp_sys_path, 0);
    mp_obj_list_init(mp_sys_argv, 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_)); // current dir (or base dir of the script)

    rtc_init();
    uart_init0();
    readline_init0();

    // initialize the serial flash file system
    init_sflash_filesystem();

    // append the flash paths to the system path
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash));
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_flash_slash_lib));

    // reset config variables; they should be set by boot.py
    MP_STATE_PORT(machine_config_main) = MP_OBJ_NULL;

    // run boot.py
    int ret = pyexec_file("boot.py");
    if (ret & PYEXEC_FORCED_EXIT) {
       goto soft_reset_exit;
    }
    if (!ret) {
        //mperror_signal_error();
    }

    // run main.py
    ret = pyexec_file("main.py");
    if (ret & PYEXEC_FORCED_EXIT) {
        goto soft_reset_exit;
    }
    if (!ret) {
        //mperror_signal_error();
    }

    // main script is finished, so now go into REPL mode.
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

    // soft reset
    goto soft_reset;
}


void init_sflash_filesystem(void)
{
    static fs_user_mount_t vfs_fat;
    FILINFO fno;

    // Initialise the local flash filesystem.
    // init the vfs object
    vfs_fat.flags = 0;
    pyb_flash_init_vfs(&vfs_fat);

    // Create it if needed, and mount in on /flash.
    FRESULT res = f_mount(&vfs_fat.fatfs);
    if (res == FR_NO_FILESYSTEM) {
        // no filesystem, so create a fresh one
        uint8_t working_buf[FF_MAX_SS];
        res = f_mkfs(&vfs_fat.fatfs, FM_FAT | FM_SFD, 0, working_buf, sizeof(working_buf));
        if (res != FR_OK) {
            //printf("failed to create /flash");
            while(1) __NOP();
        }
    }

    // mount the flash device (there should be no other devices mounted at this point)
    // we allocate this structure on the heap because vfs->next is a root pointer
    mp_vfs_mount_t *vfs = m_new_obj_maybe(mp_vfs_mount_t);
    if (vfs == NULL) {
        goto fail;
    }
    vfs->str = "/flash";
    vfs->len = 6;
    vfs->obj = MP_OBJ_FROM_PTR(&vfs_fat);
    vfs->next = NULL;
    MP_STATE_VM(vfs_mount_table) = vfs;

    MP_STATE_PORT(vfs_cur) = vfs;

    // create /flash/sys, /flash/lib if they don't exist
    if (FR_OK != f_chdir(&vfs_fat.fatfs, "/sys")) {
        f_mkdir(&vfs_fat.fatfs, "/sys");
    }
    if (FR_OK != f_chdir(&vfs_fat.fatfs, "/lib")) {
        f_mkdir(&vfs_fat.fatfs, "/lib");
    }

    f_chdir(&vfs_fat.fatfs, "/");

    // make sure we have a /flash/boot.py.  Create it if needed.
    res = f_stat(&vfs_fat.fatfs, "/boot.py", &fno);
    if (res != FR_OK) {
        // doesn't exist, create fresh file
        FIL fp;
        f_open(&vfs_fat.fatfs, &fp, "/boot.py", FA_WRITE | FA_CREATE_ALWAYS);
        UINT n;
        f_write(&fp, fresh_boot_py, sizeof(fresh_boot_py) - 1 /* don't count null terminator */, &n);
        f_close(&fp);
    }

    res = f_stat(&vfs_fat.fatfs, "/main.py", &fno);
    if (res != FR_OK) {
        // doesn't exist, create fresh file
        FIL fp;
        f_open(&vfs_fat.fatfs, &fp, "/main.py", FA_WRITE | FA_CREATE_ALWAYS);
        UINT n;
        f_write(&fp, fresh_main_py, sizeof(fresh_main_py) - 1 /* don't count null terminator */, &n);
        f_close(&fp);
    }

    return;

fail:
    //printf("failed to create /flash");
    while(1) __NOP();
}


void systemInit(void)
{
    SYS_UnlockReg();

    SYS->GPF_MFPH &= ~(SYS_GPF_MFPL_PF2MFP_Msk     | SYS_GPF_MFPL_PF3MFP_Msk);
    SYS->GPF_MFPH |=  (SYS_GPF_MFPL_PF2MFP_XT1_OUT | SYS_GPF_MFPL_PF3MFP_XT1_IN);

    CLK_EnableXtalRC(CLK_PWRCTL_HXTEN_Msk);		// 使能HXT（外部晶振，12MHz）
    CLK_WaitClockReady(CLK_STATUS_HXTSTB_Msk);	// 等待HXT稳定

    /* Set access cycle for CPU @ 192MHz */
    FMC->CYCCTL = (FMC->CYCCTL & ~FMC_CYCCTL_CYCLE_Msk) | (8 << FMC_CYCCTL_CYCLE_Pos);
    CLK_SetCoreClock(192000000);				// 用PLL产生指定频率作为系统时钟
                                                // 若HXT使能，则PLL时钟源选择HXT，须根据实际情况修改__HXT的值

    CLK->PCLKDIV = CLK_PCLKDIV_PCLK0DIV1 | CLK_PCLKDIV_PCLK1DIV1;   // APB都与CPU同频

    SYS_LockReg();

    SystemCoreClock = 192000000;

    CyclesPerUs = SystemCoreClock / 1000000;
}


void USB_Config(void)
{
    SYS_UnlockReg();

    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA12MFP_Msk      | SYS_GPA_MFPH_PA13MFP_Msk     | SYS_GPA_MFPH_PA14MFP_Msk     | SYS_GPA_MFPH_PA15MFP_Msk);
    SYS->GPA_MFPH |=  (SYS_GPA_MFPH_PA12MFP_USB_VBUS | SYS_GPA_MFPH_PA13MFP_USB_D_N | SYS_GPA_MFPH_PA14MFP_USB_D_P | SYS_GPA_MFPH_PA15MFP_USB_OTG_ID);

    SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_USBEN_Msk | SYS_USBPHY_SBO_Msk;

    CLK->CLKDIV0 = (CLK->CLKDIV0 & ~CLK_CLKDIV0_USBDIV_Msk) | CLK_CLKDIV0_USB(4);
    CLK_EnableModuleClock(USBD_MODULE);

    SYS_LockReg();


    USBD_Open(&gsInfo, VCOM_ClassRequest, NULL);

    VCOM_Init();	// Endpoint configuration

    USBD_Start();

    NVIC_EnableIRQ(USBD_IRQn);
}

