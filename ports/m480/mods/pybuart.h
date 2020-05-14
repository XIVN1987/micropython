#ifndef __MODS_PYBUART_H__
#define __MODS_PYBUART_H__


extern const mp_obj_type_t pyb_uart_type;


typedef struct _pyb_uart_obj_t pyb_uart_obj_t;


void uart_init0(void);
uint uart_rx_any(pyb_uart_obj_t *self);
uint uart_rx_char(pyb_uart_obj_t *self);
bool uart_rx_wait(pyb_uart_obj_t *self);
void uart_tx_char(pyb_uart_obj_t *self, int chr);
void uart_tx_strn(pyb_uart_obj_t *self, const char *str, uint len);


#endif
