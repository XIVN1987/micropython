#ifndef __MT7687_I2C_H__
#define __MT7687_I2C_H__

#include <stdint.h>


typedef struct {
    uint8_t  Master;		// must be 1
    uint32_t Frequency;		// from 40000 to 400000
} I2C_InitStructure;


void I2C_Init(I2C_TypeDef * I2Cx, I2C_InitStructure * initStruct);
void I2C_StartWrite(I2C_TypeDef * I2Cx, uint8_t slvaddr, uint8_t buf[], uint32_t cnt);
void I2C_StartRead(I2C_TypeDef * I2Cx, uint8_t slvaddr, uint32_t cnt);

bool I2C_TXFIFO_Full(I2C_TypeDef * I2Cx);
void I2C_Write(I2C_TypeDef * I2Cx, uint8_t data);

bool I2C_RXFIFO_Empty(I2C_TypeDef * I2Cx);
uint8_t I2C_Read(I2C_TypeDef * I2Cx);

bool I2C_Busy(I2C_TypeDef * I2Cx);
bool I2C_AddrACK(I2C_TypeDef * I2Cx);
bool I2C_DataACK(I2C_TypeDef * I2Cx);
bool I2C_ArbLost(I2C_TypeDef * I2Cx);


#endif
