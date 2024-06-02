#ifndef __SPI_NOR_H__
#define __SPI_NOR_H__


#include "extmod/vfs_fat.h"
#include "lib/oofatfs/diskio.h"


#define FLASH_BASE_ADDR             0x200000
#define FLASH_END_ADDR              0x800000    // Flash from FLASH_BASE_ADDR to FLASH_END_ADDR used by MicroPy

#define FLASH_BLOCK_SIZE            4096
#define FLASH_BLOCK_COUNT           ((FLASH_END_ADDR - FLASH_BASE_ADDR) / FLASH_BLOCK_SIZE)
#define FLASH_SECTOR_SIZE           512
#define FLASH_SECTOR_COUNT          ((FLASH_END_ADDR - FLASH_BASE_ADDR) / FLASH_SECTOR_SIZE)
#define FLASH_SECT_PER_BLK          (FLASH_BLOCK_SIZE / FLASH_SECTOR_SIZE)


DRESULT flash_disk_init(void);
DRESULT flash_disk_flush(void);
DRESULT flash_disk_read(BYTE *buff, DWORD sector, UINT count);
DRESULT flash_disk_write(const BYTE *buff, DWORD sector, UINT count);

bool flash_disk_test(void);

void flash_disk_mount(fs_user_mount_t *vfs);


#endif
