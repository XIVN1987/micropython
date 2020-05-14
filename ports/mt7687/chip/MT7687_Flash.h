#ifndef __MT7687_FLASH_H__
#define __MT7687_FLASH_H__

__attribute__((__section__(".tcmTEXT")))
void FLASH_PageWrite(uint32_t addr, uint32_t *data);

#endif //__MT7687_FLASH_H__
