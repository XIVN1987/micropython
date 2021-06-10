#include "MT7687.h"


void SPI_Init(SPI_TypeDef * SPIx, SPI_InitStructure * initStruct)
{
    SPIx->CR1 = (1                        << SPI_CR1_MODE_Pos) |
                (initStruct->LSBFrist     << SPI_CR1_LSBF_Pos) |
                (initStruct->IdleLevel    << SPI_CR1_CPOL_Pos) |
                (initStruct->SampleEdge   << SPI_CR1_CPHA_Pos) |
                (initStruct->CompleteIE   << SPI_CR1_IE_Pos) |
                (initStruct->Duplex       << SPI_CR1_DUPLEX_Pos) |
                ((initStruct->ClkDiv - 2) << SPI_CR1_CLKDIV_Pos);
}


// used in SPI_DUPLEX_HALF mode, 32-bytes at most
void SPI_Write(SPI_TypeDef * SPIx, uint8_t buf[], uint32_t byte_count)
{
    uint32_t i;

    for(i = 0; i < byte_count; i += 4)
    {
        SPIx->DR[i/4] = (buf[i] << 24) | (buf[i+1] << 16) | (buf[i+2] << 8) | buf[i+3];
    }

    SPIx->CR2 = (0                << SPI_CR2_CMDBIT_Pos) |
                ((byte_count * 8) << SPI_CR2_MOBIT_Pos) |
                (0                << SPI_CR2_MIBIT_Pos);

    SPIx->CSR = (1 << SPI_CSR_START_Pos);
}


// used in SPI_DUPLEX_HALF mode, 32-bytes at most
void SPI_Read(SPI_TypeDef * SPIx, uint32_t byte_count)
{
    SPIx->CR2 = (0                << SPI_CR2_CMDBIT_Pos) |
                (0                << SPI_CR2_MOBIT_Pos) |
                ((byte_count * 8) << SPI_CR2_MIBIT_Pos);

    SPIx->CSR = (1 << SPI_CSR_START_Pos);
}


// for SPI_DUPLEX_HALF mode, 32-bytes at most
// for SPI_DUPLEX_FULL mode, 16-bytes at most
void SPI_FetchData(SPI_TypeDef * SPIx, uint8_t buf[], uint32_t byte_count)
{
    uint32_t i;
    uint32_t base;

    if(SPIx->CR1 & SPI_CR1_DUPLEX_Msk)
        base = 4;
    else
        base = 0;

    for(i = 0; i < byte_count; i += 4)
    {
        uint32_t reg = SPIx->DR[base + i/4];

        buf[i] = reg >> 24;

        if(i+1 < byte_count)
        {
            buf[i+1] = reg >> 16;

            if(i+2 < byte_count)
            {
                buf[i+2] = reg >> 8;

                if(i+3 < byte_count)
                {
                    buf[i+3] = reg >> 0;
                }
            }
        }
    }
}


// used in SPI_DUPLEX_FULL mode, 16-bytes at most
void SPI_ReadWrite(SPI_TypeDef * SPIx, uint8_t buf[], uint32_t byte_count)
{
    uint32_t i;

    for(i = 0; i < byte_count; i += 4)
    {
        SPIx->DR[i/4] = (buf[i] << 24) | (buf[i+1] << 16) | (buf[i+2] << 8) | buf[i+3];
    }

    SPIx->CR2 = (0                << SPI_CR2_CMDBIT_Pos) |
                ((byte_count * 8) << SPI_CR2_MOBIT_Pos) |
                ((byte_count * 8) << SPI_CR2_MIBIT_Pos);

    SPIx->CSR = (1 << SPI_CSR_START_Pos);
}


bool SPI_Busy(SPI_TypeDef * SPIx)
{
    return (SPIx->CSR & SPI_CSR_BUSY_Msk);
}
