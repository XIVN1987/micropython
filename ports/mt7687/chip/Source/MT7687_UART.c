#include "MT7687.h"


void UART_Init(UART_TypeDef * UARTx, uint32_t baudrate, uint8_t databits, uint8_t parity, uint8_t stopbits)
{
    UARTx->HSP = 0;
    UARTx->LCR |=  (1 << UART_LCR_DLAB_Pos);
    UARTx->DLH = (SystemXTALClock / 16 / baudrate) >> 8;
    UARTx->DLL = (SystemXTALClock / 16 / baudrate) & 0xFF;
    UARTx->LCR &= ~(1 << UART_LCR_DLAB_Pos);

    UARTx->LCR &= ~(UART_LCR_WLS_Msk |
                    UART_LCR_STB_Msk |
                    UART_LCR_PEN_Msk | UART_LCR_EPS_Msk | UART_LCR_SP_Msk);
    UARTx->LCR |= ((databits << UART_LCR_WLS_Pos) |
                   (parity   << UART_LCR_PEN_Pos) |
                   (stopbits << UART_LCR_STB_Pos));

    UARTx->FCR = (1 << UART_FCR_FFEN_Pos) |
                 (1 << UART_FCR_CLRR_Pos) |
                 (1 << UART_FCR_CLRT_Pos) |
                 (UART_RXThreshold_6 << UART_FCR_RFTL_Pos) |
                 (UART_TXThreshold_4 << UART_FCR_TFTL_Pos);
}
