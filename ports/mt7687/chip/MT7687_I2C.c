#include "MT7687.h"


void I2C_Init(I2C_TypeDef * I2Cx, I2C_InitStructure * initStruct)
{
    I2Cx->CR = (1 << I2C_CR_MASTER_Pos) |
               (0 << I2C_CR_GMODE_Pos);     // normal mode

    uint32_t phaseVal = SystemXTALClock / initStruct->Frequency / 4 - 1;
    I2Cx->TLOW  = (phaseVal << I2C_TLOW_SETUP_Pos) |
                  (phaseVal << I2C_TLOW_STOP_Pos);
    I2Cx->THIGH = (phaseVal << I2C_THIGH_HOLD_Pos) |
                  (phaseVal << I2C_THIGH_START_Pos);

    I2Cx->PADCR = (16 << I2C_PADCR_DECNT_Pos)  |
                  (1 << I2C_PADCR_SYNCEN_Pos)  |
                  (0 << I2C_PADCR_SCLDRVH_Pos) |
                  (0 << I2C_PADCR_SDADRVH_Pos);
}


// cnt ≤ 8
void I2C_StartWrite(I2C_TypeDef * I2Cx, uint8_t slvaddr, uint8_t buf[], uint32_t cnt)
{
    I2Cx->SLVADDR = slvaddr;

    I2Cx->PACKCR = (0 << I2C_PACKCR_DIR_Pos);   // write

    for(uint32_t i = 0; i < (cnt > 8 ? 8 : cnt); i++)
    {
        I2Cx->FIFODR = buf[i];
    }

    I2Cx->CR |= (1 << I2C_CR_START_Pos);
}


// cnt ≤ 65535
void I2C_StartRead(I2C_TypeDef * I2Cx, uint8_t slvaddr, uint32_t cnt)
{
    I2Cx->SLVADDR = slvaddr;

    I2Cx->PACKCR = (1 << I2C_PACKCR_DIR_Pos);   // read

    I2Cx->RDCNT = cnt;

    I2Cx->CR |= (1 << I2C_CR_START_Pos);
}


// when not full, you can write
bool I2C_TXFIFO_Full(I2C_TypeDef * I2Cx)
{
    return (I2Cx->FIFOSR & I2C_FIFOSR_TXFULL_Msk);
}


void I2C_Write(I2C_TypeDef * I2Cx, uint8_t data)
{
    I2Cx->FIFODR = data;
}


// when not empty, you can read
bool I2C_RXFIFO_Empty(I2C_TypeDef * I2Cx)
{
    return (I2Cx->FIFOSR & I2C_FIFOSR_RXEMP_Msk);
}


uint8_t I2C_Read(I2C_TypeDef * I2Cx)
{
    return I2Cx->FIFODR;
}


bool I2C_Busy(I2C_TypeDef * I2Cx)
{
    return !(I2Cx->SR & I2C_SR_IDLE_Msk);
}


bool I2C_AddrACK(I2C_TypeDef * I2Cx)
{
    return ((I2Cx->ACK & I2C_ACK_ADDR_Msk) == 0);
}


bool I2C_DataACK(I2C_TypeDef * I2Cx)
{
    return ((I2Cx->ACK & I2C_ACK_DATA_Msk) == 0);
}


bool I2C_ArbLost(I2C_TypeDef * I2Cx)
{
    bool res = (I2Cx->SR & I2C_SR_ARBLOST_Msk);

    I2Cx->SR = (1 << I2C_SR_ARBLOST_Pos);

    return res;
}
