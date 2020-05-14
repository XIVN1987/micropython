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

#include "chip/M480.h"
#include "mods/pybrtc.h"
#include "mods/pybrng.h"
#include "mods/pybuart.h"
#include "mods/pybflash.h"
#include "mods/pybusb_vcom.h"


static const char fresh_main_py[] = "# main.py -- put your code here!\r\n";
static const char fresh_boot_py[] = "# boot.py -- run on boot-up\r\n"
                                    "import os, machine\r\n"
                                    "os.dupterm(machine.USB_VCP())\r\n"
                                    "os.dupterm(machine.UART(0, 115200, rxd='PA0', txd='PA1'))\r\n"
                                    ;


void systemInit(void);
void USB_Config(void);
void init_flash_filesystem(void);


int main(void)
{
    systemInit();

    USB_Config();

    rtc_init();
    rng_init();

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

    SYS_UnlockReg();

    SYS->IPRST0 = SYS_IPRST0_PDMARST_Msk |
                  SYS_IPRST0_EMACRST_Msk |
                  SYS_IPRST0_SDH0RST_Msk |
                  SYS_IPRST0_SDH1RST_Msk;

    SYS->IPRST1 = SYS_IPRST1_GPIORST_Msk  |
                  SYS_IPRST1_TMR0RST_Msk  |
                  SYS_IPRST1_TMR1RST_Msk  |
                  SYS_IPRST1_TMR2RST_Msk  |
                  SYS_IPRST1_TMR3RST_Msk  |
                  SYS_IPRST1_I2C0RST_Msk  |
                  SYS_IPRST1_I2C1RST_Msk  |
                  SYS_IPRST1_I2C2RST_Msk  |
                  SYS_IPRST1_SPI0RST_Msk  |
                  SYS_IPRST1_SPI1RST_Msk  |
                  SYS_IPRST1_SPI2RST_Msk  |
                  SYS_IPRST1_UART1RST_Msk |
                  SYS_IPRST1_UART2RST_Msk |
                  SYS_IPRST1_UART3RST_Msk |
                  SYS_IPRST1_UART4RST_Msk |
                  SYS_IPRST1_UART5RST_Msk |
                  SYS_IPRST1_CAN0RST_Msk  |
                  SYS_IPRST1_CAN1RST_Msk  |
                  SYS_IPRST1_EADCRST_Msk;

    SYS->IPRST2 = SYS_IPRST2_SPI3RST_Msk  |
                  SYS_IPRST2_DACRST_Msk   |
                  SYS_IPRST2_EPWM0RST_Msk |
                  SYS_IPRST2_EPWM1RST_Msk |
                  SYS_IPRST2_BPWM0RST_Msk |
                  SYS_IPRST2_BPWM1RST_Msk;

    for(uint i = 0; i < 1000; i++) __NOP();

    SYS->IPRST0 = 0;
    SYS->IPRST1 = 0;
    SYS->IPRST2 = 0;

    SYS_LockReg();

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
