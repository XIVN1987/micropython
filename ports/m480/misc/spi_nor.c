#include <stdint.h>
#include <string.h>

#include "chip/M480.h"

#include "lib/oofatfs/ff.h"
#include "lib/oofatfs/diskio.h"

#include "spi_nor.h"


static uint8_t FLASH_Block_Cache[FLASH_BLOCK_SIZE] __attribute__((aligned(4)));

static uint32_t FLASH_Block_Index = 0;
static uint32_t FLASH_Block_IndexPre = 0xFFFFFFFF;

static uint32_t FLASH_Cache_Dirty = 0;


DRESULT flash_disk_init(void)
{
    SYS_UnlockReg();

    SYS->GPC_MFPL = (SYS->GPC_MFPL & 0xFF000000)   |
                     SYS_GPC_MFPL_PC0MFP_SPIM_MOSI |
                     SYS_GPC_MFPL_PC1MFP_SPIM_MISO |
                     SYS_GPC_MFPL_PC2MFP_SPIM_CLK  |
                     SYS_GPC_MFPL_PC3MFP_SPIM_SS   |
                     SYS_GPC_MFPL_PC4MFP_SPIM_D3   |
                     SYS_GPC_MFPL_PC5MFP_SPIM_D2;
    
    /* PC0-5 high slew rate up to 80 MHz. */
    PC->SLEWCTL = (PC->SLEWCTL & 0xFFFFF000)     |
                  (1 << GPIO_SLEWCTL_HSREN0_Pos) | (1 << GPIO_SLEWCTL_HSREN1_Pos) |
                  (1 << GPIO_SLEWCTL_HSREN2_Pos) | (1 << GPIO_SLEWCTL_HSREN3_Pos) |
                  (1 << GPIO_SLEWCTL_HSREN4_Pos) | (1 << GPIO_SLEWCTL_HSREN5_Pos);

    CLK_EnableModuleClock(SPIM_MODULE);

    SYS_LockReg();


    SPIM_SET_CLOCK_DIVIDER(2);      // 192MHz / (2 * 2) = 48MHz

    SPIM_SET_RXCLKDLY_RDDLYSEL(0);  // Insert 0 delay cycle. Adjust the sampling clock of received data to latch the correct data
    SPIM_SET_RXCLKDLY_RDEDGE();     // Use SPI input clock rising edge to sample received data

    SPIM_SET_DCNUM(4);              // Dummy for W25Q64 "Fast Read Quad I/O (EBh)" Command
                                    // Note: SPIM doesn't support "Fast Read Quad Output (6Bh)" Command, see SPIM_CTL0.CMDCODE for detail.

    if(SPIM_InitFlash(1) != 0)
        return RES_NOTRDY;

    SPIM_SetQuadEnable(1, 1);       // Note: SPIM_SetQuadEnable() source code is modified by adding "dataBuf[1] &= ~0x40;",
                                    //       for clearing "Complement Protect (CMP)" bit of "Status Register-2" register.

    memset(FLASH_Block_Cache, 0, FLASH_BLOCK_SIZE);

    FLASH_Block_IndexPre = 0xFFFFFFFF;

    return RES_OK;
}


DRESULT flash_disk_read(BYTE *buff, DWORD sector, UINT count)
{
    if((sector + count > FLASH_SECTOR_COUNT) || (count == 0))
    {
        return RES_PARERR;
    }

    uint32_t index_in_block;
    for(uint32_t i = 0; i < count; i++)
    {
        index_in_block    = (sector + i) % FLASH_SECT_PER_BLK;
        FLASH_Block_Index = (sector + i) / FLASH_SECT_PER_BLK;

        if(FLASH_Block_Index != FLASH_Block_IndexPre)
        {
            flash_disk_flush();

            FLASH_Block_IndexPre = FLASH_Block_Index;

            uint32_t addr = FLASH_BASE_ADDR + FLASH_BLOCK_SIZE * FLASH_Block_Index;
            SPIM_DMA_Read(addr, 0, FLASH_BLOCK_SIZE, FLASH_Block_Cache, CMD_DMA_FAST_QUAD_READ, 1);
        }

        // Copy the requested sector from the block cache
        memcpy(buff, &FLASH_Block_Cache[index_in_block * FLASH_SECTOR_SIZE], FLASH_SECTOR_SIZE);
        buff += FLASH_SECTOR_SIZE;
    }

    return RES_OK;
}


DRESULT flash_disk_write(const BYTE *buff, DWORD sector, UINT count)
{
    if((sector + count > FLASH_SECTOR_COUNT) || (count == 0))
    {
        return RES_PARERR;
    }

    uint32_t index_in_block;
    for(uint32_t i = 0; i < count; i++)
    {
        index_in_block    = (sector + i) % FLASH_SECT_PER_BLK;
        FLASH_Block_Index = (sector + i) / FLASH_SECT_PER_BLK;

        if(FLASH_Block_Index != FLASH_Block_IndexPre)
        {
            flash_disk_flush();

            FLASH_Block_IndexPre = FLASH_Block_Index;

            uint32_t addr = FLASH_BASE_ADDR + FLASH_BLOCK_SIZE * FLASH_Block_Index;
            SPIM_DMA_Read(addr, 0, FLASH_BLOCK_SIZE, FLASH_Block_Cache, CMD_DMA_FAST_QUAD_READ, 1);
        }

        // Copy the input sector to the block cache
        memcpy(&FLASH_Block_Cache[index_in_block * FLASH_SECTOR_SIZE], buff, FLASH_SECTOR_SIZE);
        buff += FLASH_SECTOR_SIZE;

        FLASH_Cache_Dirty = 1;
    }

    return RES_OK;
}


DRESULT flash_disk_flush(void)
{
    // Write back the cache if it's dirty
    if(FLASH_Cache_Dirty)
    {
        uint32_t addr = FLASH_BASE_ADDR + FLASH_BLOCK_SIZE * FLASH_Block_IndexPre;

        SPIM_EraseBlock(addr, 0, OPCODE_SE_4K, 1, 1);

        SPIM_DMA_Write(addr, 0, FLASH_BLOCK_SIZE, FLASH_Block_Cache, CMD_QUAD_PAGE_PROGRAM_WINBOND);

        FLASH_Cache_Dirty = 0;
    }

    return RES_OK;
}


bool flash_disk_test(void)
{
    uint8_t buffer[FLASH_SECTOR_SIZE];

    SPIM_EraseBlock(FLASH_BASE_ADDR, 0, OPCODE_SE_4K, 1, 1);

    SPIM_DMA_Read(FLASH_BASE_ADDR, 0, FLASH_SECTOR_SIZE, buffer, CMD_DMA_FAST_QUAD_READ, 1);

    for(int i = 0; i < FLASH_SECTOR_SIZE; i++)
        if(buffer[i] != 0xFF)
            return false;

    for(int i = 0; i < FLASH_SECTOR_SIZE; i++)
        buffer[i] = i;

    SPIM_DMA_Write(FLASH_BASE_ADDR, 0, FLASH_SECTOR_SIZE, buffer, CMD_QUAD_PAGE_PROGRAM_WINBOND);
    
    memset(buffer, 0x00, FLASH_SECTOR_SIZE);
    SPIM_DMA_Read(FLASH_BASE_ADDR, 0, FLASH_SECTOR_SIZE, buffer, CMD_DMA_FAST_QUAD_READ, 1);

    for(int i = 0; i < FLASH_SECTOR_SIZE; i++)
        if(buffer[i] != (i & 0xFF))
            return false;
    
    return true;
}



/******************************************************************************/
// provide ioctl to fs_user_mount_t.mp_vfs_blockdev_t

#include "py/obj.h"
#include "py/runtime.h"

#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"


typedef struct {
    mp_obj_base_t base;
} pyb_flash_obj_t;

static pyb_flash_obj_t pyb_flash_obj = { {&mp_type_type } };


static mp_obj_t flash_writeblocks(mp_obj_t self, mp_obj_t block_num, mp_obj_t buf)
{
    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_READ);

    DRESULT res = flash_disk_write(bufinfo.buf, mp_obj_get_int(block_num), bufinfo.len / FLASH_SECTOR_SIZE);

    return MP_OBJ_NEW_SMALL_INT(res != RES_OK); // return of 0 means success
}
static MP_DEFINE_CONST_FUN_OBJ_3(flash_writeblocks_obj, flash_writeblocks);



static mp_obj_t flash_ioctl(mp_obj_t self, mp_obj_t cmd_in, mp_obj_t arg_in)
{
    mp_int_t cmd = mp_obj_get_int(cmd_in);
    switch(cmd)
    {
    case MP_BLOCKDEV_IOCTL_INIT:
        return MP_OBJ_NEW_SMALL_INT(flash_disk_init() != RES_OK);
    
    case MP_BLOCKDEV_IOCTL_DEINIT:
        flash_disk_flush();
        return MP_OBJ_NEW_SMALL_INT(0);

    case MP_BLOCKDEV_IOCTL_SYNC:
        flash_disk_flush();
        return MP_OBJ_NEW_SMALL_INT(0);
    
    case MP_BLOCKDEV_IOCTL_BLOCK_COUNT:
        return MP_OBJ_NEW_SMALL_INT(FLASH_SECTOR_COUNT);

    case MP_BLOCKDEV_IOCTL_BLOCK_SIZE:
        return MP_OBJ_NEW_SMALL_INT(FLASH_SECTOR_SIZE);

    default: return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_3(flash_ioctl_obj, flash_ioctl);


void flash_disk_mount(fs_user_mount_t *vfs)
{
    vfs->base.type = &mp_fat_vfs_type;
    vfs->blockdev.flags = MP_BLOCKDEV_FLAG_NATIVE | MP_BLOCKDEV_FLAG_HAVE_IOCTL;
    vfs->blockdev.readblocks[2]  = (mp_obj_t)flash_disk_read;       // native version
    vfs->blockdev.writeblocks[0] = (mp_obj_t)&flash_writeblocks_obj;// 若不提供此函数，vfs_fat_diskio.c 中的 disk_ioctl 会返回 STA_PROTECT
    vfs->blockdev.writeblocks[1] = (mp_obj_t)&pyb_flash_obj;
    vfs->blockdev.writeblocks[2] = (mp_obj_t)flash_disk_write;      // native version
    vfs->blockdev.u.ioctl[0]     = (mp_obj_t)&flash_ioctl_obj;
    vfs->blockdev.u.ioctl[1]     = (mp_obj_t)&pyb_flash_obj;
    vfs->fatfs.drv = vfs;
}
