#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/gc.h"
#include "misc/gccollect.h"

#include "shared/runtime/pyexec.h"
#include "shared/readline/readline.h"
#include "shared/timeutils/timeutils.h"

#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

#include "chip/M480.h"
#include "mods/pybrng.h"
#include "mods/pybrtc.h"
#include "mods/pybuart.h"

#include "misc/spi_nor.h"


static const char fresh_main_py[] = "# main.py -- put your code here!\r\n";
static const char fresh_boot_py[] = "# boot.py -- run on boot-up\r\n"
                                    "import os, machine\r\n"
                                    "os.dupterm(machine.UART(0, 115200, rxd='PA0', txd='PA1'))\r\n"
                                    ;


void systemInit(void);
void init_flash_filesystem(void);


int main(void)
{
    systemInit();

    rng_init();
    rtc_init();
    
    flash_disk_init();
    
    SysTick_Config(SystemCoreClock / 1000);
    NVIC_SetPriority(SysTick_IRQn, 3);  // 优先级高于它的ISR中不能使用 mp_hal_delay_us() 等延时相关函数

soft_reset:
    gc_init(&__HeapBase, &__StackLimit);

    mp_stack_set_top((char*)&__StackTop);
    mp_stack_set_limit((char*)&__StackTop - (char*)&__StackLimit - 1024);

    // MicroPython init
    mp_init();
    mp_obj_list_init(mp_sys_path, 0);
    mp_obj_list_init(mp_sys_argv, 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_)); // current dir (or base dir of the script)
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR__slash_lib));

    uart_init0();
    readline_init0();

    init_flash_filesystem();

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
    printf("MicroPy: soft reboot\n");

    gc_sweep_all();

    goto soft_reset;
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
    CLK_SetCoreClock(192000000);

    CLK->PCLKDIV = CLK_PCLKDIV_PCLK0DIV2 | CLK_PCLKDIV_PCLK1DIV2;

    SYS_LockReg();

    SystemCoreClock = 192000000;

    CyclesPerUs = SystemCoreClock / 1000000;
}


void init_flash_filesystem(void)
{
    static fs_user_mount_t vfs_fat;

    flash_disk_mount(&vfs_fat);

    FRESULT res = f_mount(&vfs_fat.fatfs);
    if(res == FR_NO_FILESYSTEM)
    {
        // no filesystem, so create a fresh one
        uint8_t working_buf[FF_MAX_SS];
        res = f_mkfs(&vfs_fat.fatfs, FM_FAT | FM_SFD, 0, working_buf, sizeof(working_buf));
        if(res != FR_OK)
        {
            printf("failed to create /");
            while(1) __NOP();
        }
    }

    /* mount the flash device */
    // we allocate this structure on the heap because vfs->next is a root pointer
    mp_vfs_mount_t *vfs = m_new_obj_maybe(mp_vfs_mount_t);

    vfs->str = "/";
    vfs->len = 1;
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
}


uint32_t get_fattime(void)
{
    timeutils_struct_time_t tm;
    timeutils_seconds_since_2000_to_struct_time(rtc_get_seconds(), &tm);

     return ((tm.tm_year - 1980) << 25) | ((tm.tm_mon) << 21)  |
             ((tm.tm_mday) << 16)       | ((tm.tm_hour) << 11) |
             ((tm.tm_min) << 5)         | (tm.tm_sec >> 1);
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
