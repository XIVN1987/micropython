#include "MT7687.h"


#define CACHE_OP_Invalidate_All_Lines   0x1
#define CACHE_OP_Flush_All_Lines        0x9

#define FLASH_CMD_PAGE_ERASE            0x20    // 4K
#define FLASH_CMD_PAGE_PROGRAM          0x02
#define FLASH_CMD_WRITE_ENABLE          0x06
#define FLASH_CMD_WRITE_DISABLE         0x04
#define FLASH_CMD_READ_STATUS           0x05


__attribute__((__section__(".tcmTEXT")))
void FLASH_PageWrite(uint32_t addr, uint32_t *data)
{
    uint32_t i, j;

    __disable_irq();

    /* Invalidate all cache lines */
    CACHE->OP &= ~CACHE_OP_OP_Msk;
    CACHE->OP |= (CACHE_OP_Flush_All_Lines << CACHE_OP_OP_Pos) | (1 << CACHE_OP_EN_Pos);
    CACHE->OP &= ~CACHE_OP_OP_Msk;
    CACHE->OP |= (CACHE_OP_Invalidate_All_Lines << CACHE_OP_OP_Pos) | (1 << CACHE_OP_EN_Pos);
    __ISB();

    /* disable cache */
    CACHE->CR &= ~CACHE_CR_MCEN_Msk;

    SFC->CR = 0x08;     // MAC enable

    /* Write Enable*/
    SFC->DATA[0] = FLASH_CMD_WRITE_ENABLE;
    SFC->OLEN = 1;
    SFC->ILEN = 0;
    SFC->CR = 0x0C;
    while(((SFC->CR & SFC_CR_WIPRdy_Msk) == 0) || ((SFC->CR & SFC_CR_WIP_Msk) != 0)) __NOP();
    SFC->CR = 0x08;

    /* Page Erase */
    SFC->DATA[0] = FLASH_CMD_PAGE_ERASE | (((addr >> 16) & 0xFF) << 8) | (((addr >> 8) & 0xFF) << 16) | ((addr & 0xFF) << 24);
    SFC->OLEN = 4;
    SFC->ILEN = 0;
    SFC->CR = 0x0C;
    while(((SFC->CR & SFC_CR_WIPRdy_Msk) == 0) || ((SFC->CR & SFC_CR_WIP_Msk) != 0)) __NOP();
    SFC->CR = 0x08;

    /* Read Status Register */
    SFC->DATA[0] = FLASH_CMD_READ_STATUS;
    SFC->OLEN = 1;
    SFC->ILEN = 1;
    do {
        SFC->CR = 0x0C;
        while(((SFC->CR & SFC_CR_WIPRdy_Msk) == 0) || ((SFC->CR & SFC_CR_WIP_Msk) != 0)) __NOP();
        SFC->CR = 0x08;
    } while((SFC->DATA[0] & 0x100) != 0);

    for(i = 0; i < 32; i++)     // 4K / 128 = 32
    {
        /* Write Enable*/
        SFC->DATA[0] = FLASH_CMD_WRITE_ENABLE;
        SFC->OLEN = 1;
        SFC->ILEN = 0;
        SFC->CR = 0x0C;
        while(((SFC->CR & SFC_CR_WIPRdy_Msk) == 0) || ((SFC->CR & SFC_CR_WIP_Msk) != 0)) __NOP();
        SFC->CR = 0x08;

        /* Page Program */
        SFC->DATA[0] = FLASH_CMD_PAGE_PROGRAM | (((addr >> 16) & 0xFF) << 8) | (((addr >> 8) & 0xFF) << 16) | ((addr & 0xFF) << 24);
        for(j = 0; j < 128/4; j++) SFC->DATA[1+j] = data[128/4*i + j];
        SFC->OLEN = 4 + 128;
        SFC->ILEN = 0;
        SFC->CR = 0x0C;
        while(((SFC->CR & SFC_CR_WIPRdy_Msk) == 0) || ((SFC->CR & SFC_CR_WIP_Msk) != 0)) __NOP();
        SFC->CR = 0x08;

        addr += 128;

        /* Read Status Register */
        SFC->DATA[0] = FLASH_CMD_READ_STATUS;
        SFC->OLEN = 1;
        SFC->ILEN = 1;
        do {
            SFC->CR = 0x0C;
            while(((SFC->CR & SFC_CR_WIPRdy_Msk) == 0) || ((SFC->CR & SFC_CR_WIP_Msk) != 0)) __NOP();
            SFC->CR = 0x08;
        } while((SFC->DATA[0] & 0x100) != 0);
    }

    SFC->CR = 0x00;     // MAC disable

    /* enable cache */
    CACHE->CR |= (1 << CACHE_CR_MCEN_Pos);

    __enable_irq();
}
