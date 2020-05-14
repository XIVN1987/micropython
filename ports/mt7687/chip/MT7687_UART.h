#ifndef __MT7687_UART_H__
#define __MT7687_UART_H__


void UART_Init(UART_TypeDef * UARTx, uint32_t baudrate, uint8_t databits, uint8_t parity, uint8_t stopbits);




#define UART_DATA_8BIT          3
#define UART_DATA_7BIT          2
#define UART_DATA_6BIT          1
#define UART_DATA_5BIT          0

#define UART_STOP_1BIT          0
#define UART_STOP_2BIT          1

#define UART_PARITY_NONE        0
#define UART_PARITY_ODD         1
#define UART_PARITY_EVEN        3
#define UART_PARITY_ONE         5
#define UART_PARITY_ZERO        7

#define UART_RXThreshold_1      0
#define UART_RXThreshold_6      1
#define UART_RXThreshold_12     2

#define UART_TXThreshold_1      0
#define UART_TXThreshold_4      1
#define UART_TXThreshold_8      2


#endif // __MT7687_UART_H__
