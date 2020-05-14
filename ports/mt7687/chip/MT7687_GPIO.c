#include "MT7687.h"


#define IOT_PINMUX_AON_BASE    0x81023000


void PORT_Init(uint32_t pin, uint32_t func)
{
    switch(pin)
    {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
        *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + 0x20)) &= ~(0xF << ((pin- 0)*4));
        *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + 0x20)) |= (func << ((pin- 0)*4));
        break;

    case 24:
    case 25:
    case 26:
    case 27:
    case 28:
    case 29:
    case 30:
    case 31:
        *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + 0x2C)) &= ~(0xF << ((pin-24)*4));
        *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + 0x2C)) |= (func << ((pin-24)*4));
        break;

    case 32:
    case 33:
    case 34:
    case 35:
    case 36:
    case 37:
    case 38:
    case 39:
        *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + 0x30)) &= ~(0xF << ((pin-32)*4));
        *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + 0x30)) |= (func << ((pin-32)*4));
        break;
        
    case 57:
    case 58:
    case 59:
    case 60:
        *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + 0x3C)) &= ~(0xF << ((pin-57)*4));
        *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + 0x3C)) |= (func << ((pin-57)*4));
        break;
    }
}


#define IOT_GPIO_PU1                (0x00)
#define IOT_GPIO_PU1_SET            (IOT_GPIO_PU1 + 0x04)
#define IOT_GPIO_PU1_RESET          (IOT_GPIO_PU1 + 0x08)
#define IOT_GPIO_PU2                (0x10)
#define IOT_GPIO_PU2_SET            (IOT_GPIO_PU2 + 0x04)
#define IOT_GPIO_PU2_RESET          (IOT_GPIO_PU2 + 0x08)
#define IOT_GPIO_PU3                (0x20)
#define IOT_GPIO_PU3_SET            (IOT_GPIO_PU3 + 0x04)
#define IOT_GPIO_PU3_RESET          (IOT_GPIO_PU3 + 0x08)

#define IOT_GPIO_PD1                (0x30)
#define IOT_GPIO_PD1_SET            (IOT_GPIO_PD1 + 0x04)
#define IOT_GPIO_PD1_RESET          (IOT_GPIO_PD1 + 0x08)
#define IOT_GPIO_PD2                (0x40)
#define IOT_GPIO_PD2_SET            (IOT_GPIO_PD2 + 0x04)
#define IOT_GPIO_PD2_RESET          (IOT_GPIO_PD2 + 0x08)
#define IOT_GPIO_PD3                (0x50)
#define IOT_GPIO_PD3_SET            (IOT_GPIO_PD3 + 0x04)
#define IOT_GPIO_PD3_RESET          (IOT_GPIO_PD3 + 0x08)

#define IOT_GPIO_DOUT1              (0x60)
#define IOT_GPIO_DOUT1_SET          (IOT_GPIO_DOUT1 + 0x04)
#define IOT_GPIO_DOUT1_RESET        (IOT_GPIO_DOUT1 + 0x08)
#define IOT_GPIO_DOUT2              (0x70)
#define IOT_GPIO_DOUT2_SET          (IOT_GPIO_DOUT2 + 0x04)
#define IOT_GPIO_DOUT2_RESET        (IOT_GPIO_DOUT2 + 0x08)
#define IOT_GPIO_DOUT3              (0x80)
#define IOT_GPIO_DOUT3_SET          (IOT_GPIO_DOUT3 + 0x04)
#define IOT_GPIO_DOUT3_RESET        (IOT_GPIO_DOUT3 + 0x08)

#define IOT_GPIO_OE1                (0x90)
#define IOT_GPIO_OE1_SET            (IOT_GPIO_OE1 + 0x04)
#define IOT_GPIO_OE1_RESET          (IOT_GPIO_OE1 + 0x08)
#define IOT_GPIO_OE2                (0xA0)
#define IOT_GPIO_OE2_SET            (IOT_GPIO_OE2 + 0x04)
#define IOT_GPIO_OE2_RESET          (IOT_GPIO_OE2 + 0x08)
#define IOT_GPIO_OE3                (0xB0)
#define IOT_GPIO_OE3_SET            (IOT_GPIO_OE3 + 0x04)
#define IOT_GPIO_OE3_RESET          (IOT_GPIO_OE3 + 0x08)

#define IOT_GPIO_DIN1               (0xC0)
#define IOT_GPIO_DIN2               (IOT_GPIO_DIN1 + 0x04)
#define IOT_GPIO_DIN3               (IOT_GPIO_DIN1 + 0x08)

#define IOT_GPIO_PADDRV1            (0xD0)
#define IOT_GPIO_PADDRV2            (IOT_GPIO_PADDRV1 + 0x04)
#define IOT_GPIO_PADDRV3            (IOT_GPIO_PADDRV1 + 0x08)
#define IOT_GPIO_PADDRV4            (IOT_GPIO_PADDRV1 + 0x0C)
#define IOT_GPIO_PADDRV5            (IOT_GPIO_PADDRV1 + 0x10)

#define IOT_GPIO_PAD_CTRL0          (0x81021080)
#define IOT_GPIO_PAD_CTRL1          (IOT_GPIO_PAD_CTRL0 + 0x04)
#define IOT_GPIO_PAD_CTRL2          (IOT_GPIO_PAD_CTRL0 + 0x08)

#define IOT_GPIO_IES0               (0x100)
#define IOT_GPIO_IES1               (IOT_GPIO_IES0 + 0x04)
#define IOT_GPIO_IES2               (IOT_GPIO_IES0 + 0x08)

#define IOT_GPIO_SDIO_CLK           (0x48)
#define IOT_GPIO_SDIO_CMD           (IOT_GPIO_SDIO_CLK + 0x04)
#define IOT_GPIO_SDIO_DATA0         (IOT_GPIO_SDIO_CLK + 0x08)
#define IOT_GPIO_SDIO_DATA1         (IOT_GPIO_SDIO_CLK + 0x0C)
#define IOT_GPIO_SDIO_DATA2         (IOT_GPIO_SDIO_CLK + 0x10)
#define IOT_GPIO_SDIO_DATA3         (IOT_GPIO_SDIO_CLK + 0x14)



void GPIO_Init(uint32_t pin, uint32_t dir)
{
    uint32_t bit = pin % 32;

    switch(pin/32)
    {
    case 0:
        if(dir == 1)
        {
            if (bit <= 26) {
                *((volatile uint32_t *) (IOT_GPIO_AON_BASE + IOT_GPIO_IES0)) &= ~(1 << bit);
            } else if (bit == 27) {
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_CLK))   &= ~0x02;
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_CLK))   |= 0x800;
            } else if (bit == 28) {
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_CMD))   &= ~0x02;
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_CMD))   |= 0x800;
            } else if (bit == 29) {
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_DATA3)) &= ~0x02;
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_DATA3)) |= 0x800;
            } else if (bit == 30) {
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_DATA2)) &= ~0x02;
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_DATA2)) |= 0x800;
            } else if (bit == 31) {
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_DATA1)) &= ~0x02;
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_DATA1)) |= 0x800;
            }

            *((volatile uint32_t *) (IOT_GPIO_AON_BASE + IOT_GPIO_OE1_SET)) = (1 << bit);
        }
        else
        {
            if (bit <= 26) {
                *((volatile uint32_t *) (IOT_GPIO_AON_BASE + IOT_GPIO_IES0)) |= (1 << bit);
            } else if (bit == 27) {
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_CLK))   |= 0x802;
            } else if (bit == 28) {
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_CMD))   |= 0x802;
            } else if (bit == 29) {
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_DATA3)) |= 0x802;
            } else if (bit == 30) {
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_DATA2)) |= 0x802;
            } else if (bit == 31) {
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_DATA1)) |= 0x802;
            }

            *((volatile uint32_t *) (IOT_GPIO_AON_BASE + IOT_GPIO_OE1_RESET)) = (1 << bit);
        }
        break;

    case 1:
        if(dir == 1) 
        {
            if (bit > 0) {
                *((volatile uint32_t *) (IOT_GPIO_AON_BASE + IOT_GPIO_IES1)) &= ~(1 << bit);
            } else {
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_DATA0)) &= ~0x02;
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_DATA0)) |= 0x800;
            }

            *((volatile uint32_t *) (IOT_GPIO_AON_BASE + IOT_GPIO_OE2_SET)) = (1 << bit);
        }
        else
        {
            if (bit > 0) {
                *((volatile uint32_t *) (IOT_GPIO_AON_BASE + IOT_GPIO_IES1)) |= (1 << bit);
            } else {
                *((volatile uint32_t *) (IOT_PINMUX_AON_BASE + IOT_GPIO_SDIO_DATA0)) |= 0x802;
            }

            *((volatile uint32_t *) (IOT_GPIO_AON_BASE + IOT_GPIO_OE2_RESET)) = (1 << bit);
        }
        break;
    }
}

void GPIO_SetBit(uint32_t pin)
{
    uint32_t bit = pin % 32;
    
    switch(pin/32)
    {
    case 0:
        *((volatile uint32_t *) (IOT_GPIO_AON_BASE + IOT_GPIO_DOUT1_SET)) = (1 << bit);
        break;

    case 1:
        *((volatile uint32_t *) (IOT_GPIO_AON_BASE + IOT_GPIO_DOUT2_SET)) = (1 << bit);
        break;
    }
}

void GPIO_ClrBit(uint32_t pin)
{
    uint32_t bit = pin % 32;

    switch(pin/32)
    {
    case 0:
        *((volatile uint32_t *) (IOT_GPIO_AON_BASE + IOT_GPIO_DOUT1_RESET)) = (1 << bit);
        break;

    case 1:
        *((volatile uint32_t *) (IOT_GPIO_AON_BASE + IOT_GPIO_DOUT2_RESET)) = (1 << bit);
        break;
    }
}

void GPIO_InvBit(uint32_t pin)
{
    uint32_t val = 0;
	uint32_t bit = pin % 32;
	
	switch(pin/32)
    {
    case 0:
        val = (*((volatile uint32_t *) (IOT_GPIO_AON_BASE + IOT_GPIO_DOUT1)) >> bit) & 1;
        break;

    case 1:
        val = (*((volatile uint32_t *) (IOT_GPIO_AON_BASE + IOT_GPIO_DOUT2)) >> bit) & 1;
        break;
    }
	
	if(val) GPIO_ClrBit(pin);
	else    GPIO_SetBit(pin);
}

uint32_t GPIO_GetBit(uint32_t pin)
{
    uint32_t bit = pin % 32;

    switch(pin/32)
    {
    case 0:
        return (*((volatile uint32_t *) (IOT_GPIO_AON_BASE + IOT_GPIO_DIN1)) >> bit) & 1;

    case 1:
        return (*((volatile uint32_t *) (IOT_GPIO_AON_BASE + IOT_GPIO_DIN2)) >> bit) & 1;

    default:
        return 0;
    }
}
