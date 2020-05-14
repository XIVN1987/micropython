#ifndef __MT7687_SPI_H__
#define __MT7687_SPI_H__

#include <stdint.h>


typedef struct {
    uint8_t  Master;		// must be 1
    uint16_t ClkDiv;		// from 2 to 4097
    uint8_t  Duplex;        // SPI_DUPLEX_HALF or SPI_DUPLEX_FULL
    uint8_t  IdleLevel;		// SPI_LEVEL_LOW or SPI_LEVEL_HIGH
    uint8_t  SampleEdge;	// SPI_EDGE_FIRST or SPI_EDGE_SECOND
    uint8_t  LSBFrist;
    uint8_t  CompleteIE;    // transaction complete interrupt enable
} SPI_InitStructure;


#define SPI_DUPLEX_HALF     0
#define SPI_DUPLEX_FULL     1

#define SPI_LEVEL_LOW		0
#define SPI_LEVEL_HIGH		1

#define SPI_EDGE_FIRST		0
#define SPI_EDGE_SECOND		1


void SPI_Init(SPI_TypeDef * SPIx, SPI_InitStructure * initStruct);
void SPI_Write(SPI_TypeDef * SPIx, uint8_t buf[], uint32_t byte_count);
void SPI_Read(SPI_TypeDef * SPIx, uint32_t byte_count);
void SPI_FetchData(SPI_TypeDef * SPIx, uint8_t buf[], uint32_t byte_count);
void SPI_ReadWrite(SPI_TypeDef * SPIx, uint8_t buf[], uint32_t byte_count);
bool SPI_Busy(SPI_TypeDef * SPIx);


#endif
