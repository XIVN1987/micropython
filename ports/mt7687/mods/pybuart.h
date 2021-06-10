#ifndef __MODS_PYBUART_H__
#define __MODS_PYBUART_H__

typedef enum {
    PYB_UART_1      =  0,
    PYB_UART_2      =  1,
    PYB_NUM_UARTS
} pyb_uart_id_t;

typedef struct _pyb_uart_obj_t pyb_uart_obj_t;
extern const mp_obj_type_t pyb_uart_type;

void uart_init0(void);
uint32_t uart_rx_any(pyb_uart_obj_t *uart_obj);
int uart_rx_char(pyb_uart_obj_t *uart_obj);
bool uart_tx_char(pyb_uart_obj_t *self, int c);
bool uart_tx_strn(pyb_uart_obj_t *uart_obj, const char *str, uint len);

#endif
