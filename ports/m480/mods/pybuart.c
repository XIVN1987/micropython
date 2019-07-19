/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 * Copyright (c) 2015 Daniel Campora
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/objlist.h"
#include "py/stream.h"
#include "py/mphal.h"

#include "chip/M480.h"

#include "mods/moduos.h"
#include "mods/pybpin.h"
#include "mods/pybuart.h"

#include "lib/utils/interrupt_char.h"


/// \moduleref pyb
/// \class UART - duplex serial communication bus

/******************************************************************************
 DEFINE CONSTANTS
 *******-***********************************************************************/
#define PYBUART_FRAME_TIME_US(baud)     ((11 * 1000000) / baud)

#define PYBUART_RX_BUFFER_LEN           (256)


/******************************************************************************
 DEFINE PRIVATE TYPES
 ******************************************************************************/
typedef enum {
    PYB_UART_0      =  0,
    PYB_UART_1      =  1,
    PYB_UART_2      =  2,
    PYB_UART_3      =  3,
    PYB_UART_4      =  4,
    PYB_UART_5      =  5,
    PYB_NUM_UARTS
} pyb_uart_id_t;


struct _pyb_uart_obj_t {
    mp_obj_base_t base;
    pyb_uart_id_t uart_id;
    UART_T *UARTx;
    uint baudrate;
    byte databits;
    byte parity;
    byte stopbits;

    uint8_t  IRQn;
    uint8_t  irq_trigger;
    uint8_t  irq_priority;   // 中断优先级
    mp_obj_t irq_callback;   // 中断处理函数

    byte *read_buf;                     // read buffer pointer
    volatile uint16_t read_buf_head;    // indexes first empty slot
    uint16_t read_buf_tail;             // indexes first full slot (not full if equals head)
};


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_uart_obj_t pyb_uart_obj[PYB_NUM_UARTS] = {
    {.uart_id = PYB_UART_0, .UARTx = UART0, .IRQn = UART0_IRQn, .read_buf = NULL},
    {.uart_id = PYB_UART_1, .UARTx = UART1, .IRQn = UART1_IRQn, .read_buf = NULL},
    {.uart_id = PYB_UART_2, .UARTx = UART2, .IRQn = UART2_IRQn, .read_buf = NULL},
    {.uart_id = PYB_UART_3, .UARTx = UART3, .IRQn = UART3_IRQn, .read_buf = NULL},
    {.uart_id = PYB_UART_4, .UARTx = UART4, .IRQn = UART4_IRQn, .read_buf = NULL},
    {.uart_id = PYB_UART_5, .UARTx = UART5, .IRQn = UART5_IRQn, .read_buf = NULL},
};


/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/
void uart_init0 (void) {
    // save references of the UART objects, to prevent the read buffers from being trashed by the gc
    MP_STATE_PORT(pyb_uart_objs)[0] = &pyb_uart_obj[0];
    MP_STATE_PORT(pyb_uart_objs)[1] = &pyb_uart_obj[1];
    MP_STATE_PORT(pyb_uart_objs)[2] = &pyb_uart_obj[2];
    MP_STATE_PORT(pyb_uart_objs)[3] = &pyb_uart_obj[3];
    MP_STATE_PORT(pyb_uart_objs)[4] = &pyb_uart_obj[4];
    MP_STATE_PORT(pyb_uart_objs)[5] = &pyb_uart_obj[5];
}

int uart_rx_any(pyb_uart_obj_t *self) {
    if (self->read_buf_tail != self->read_buf_head) {
        // buffering via irq
        return (self->read_buf_head > self->read_buf_tail) ? self->read_buf_head - self->read_buf_tail :
                                     PYBUART_RX_BUFFER_LEN + self->read_buf_head - self->read_buf_tail;
    }
    else {
        // no buffering
        if(self->UARTx->FIFOSTS & UART_FIFOSTS_RXPTR_Msk)       return ((self->UARTx->FIFOSTS & UART_FIFOSTS_RXPTR_Msk) >> UART_FIFOSTS_RXPTR_Pos);
        else if(self->UARTx->FIFOSTS & UART_FIFOSTS_RXFULL_Msk) return 16;
        else                                                    return 0;
    }
}


int uart_rx_char(pyb_uart_obj_t *self) {
    if (self->read_buf_tail != self->read_buf_head) {
        // buffering via irq
        int data = self->read_buf[self->read_buf_tail];
        self->read_buf_tail = (self->read_buf_tail + 1) % PYBUART_RX_BUFFER_LEN;
        return data;
    } else {
        // no buffering
        return self->UARTx->DAT & UART_DAT_DAT_Msk;
    }
}


bool uart_tx_char(pyb_uart_obj_t *self, int c) {
    while(UART_IS_TX_FULL(self->UARTx)) __NOP();

    self->UARTx->DAT = c;

    return true;
}


bool uart_tx_strn(pyb_uart_obj_t *self, const char *str, uint len) {
    for (const char *top = str + len; str < top; str++) {
        if (!uart_tx_char(self, *str)) {
            return false;
        }
    }
    return true;
}


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
// Waits at most timeout microseconds for at least 1 char to become ready for reading
// Returns true if something available, false if not.
bool uart_rx_wait (pyb_uart_obj_t *self) {
    int timeout = PYBUART_FRAME_TIME_US(self->baudrate) * 10;   // 10 charaters time
    for ( ; ; ) {
        if(uart_rx_any(self)) {
            return true; // we have at least 1 char ready for reading
        }

        if (timeout > 0) {
            mp_hal_delay_us(1);
            timeout--;
        }
        else {
            return false;
        }
    }
}


/******************************************************************************/
/* MicroPython bindings                                                      */

STATIC void pyb_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_uart_obj_t *self = self_in;

    mp_printf(print, "UART(%u, baudrate=%u, bits=%u, ", self->uart_id, self->baudrate, self->databits);

    switch(self->parity) {
    case UART_PARITY_NONE:  mp_printf(print, "parity=None, "); break;
    case UART_PARITY_EVEN:  mp_printf(print, "parity=Even, "); break;
    case UART_PARITY_ODD:   mp_printf(print, "parity=Odd, ");  break;
    case UART_PARITY_SPACE: mp_printf(print, "parity=Zero, "); break;
    case UART_PARITY_MARK:  mp_printf(print, "parity=One, ");  break;
    default: break;
    }

    mp_printf(print, "stop=%u)", self->stopbits);
}


STATIC const mp_arg_t pyb_uart_init_args[] = {
    { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 1} },
    { MP_QSTR_baudrate, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 115200} },
    { MP_QSTR_bits,     MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 8} },
    { MP_QSTR_parity,   MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    { MP_QSTR_stop,     MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 1} },
    { MP_QSTR_irq,      MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 0} },
    { MP_QSTR_callback, MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    { MP_QSTR_priority, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 8} },
    { MP_QSTR_rxd,      MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    { MP_QSTR_txd,      MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    { MP_QSTR_rts,      MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    { MP_QSTR_cts,      MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
};
STATIC mp_obj_t pyb_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_uart_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(args), pyb_uart_init_args, args);

    uint uart_id = args[0].u_int;
    if(uart_id >= PYB_NUM_UARTS) {
        mp_raise_OSError(MP_ENODEV);
    }

    pyb_uart_obj_t *self = &pyb_uart_obj[uart_id];
    self->base.type = &pyb_uart_type;

    self->baudrate = args[1].u_int;

    self->databits = args[2].u_int;
    if((self->databits < 5) || (self->databits > 8))
    {
        goto invalid_args;
    }

    if(args[3].u_obj == mp_const_none)
    {
        self->parity = UART_PARITY_NONE;
    }
    else if(MP_OBJ_IS_INT(args[3].u_obj))
    {
        switch(mp_obj_get_int(args[3].u_obj)) {
        case UART_PARITY_NONE:  self->parity = UART_PARITY_NONE;  break;
        case UART_PARITY_ODD:   self->parity = UART_PARITY_ODD;   break;
        case UART_PARITY_EVEN:  self->parity = UART_PARITY_EVEN;  break;
        case UART_PARITY_SPACE: self->parity = UART_PARITY_SPACE; break;
        case UART_PARITY_MARK:  self->parity = UART_PARITY_MARK;  break;
        default: goto invalid_args;
        }
    }
    else if(MP_OBJ_IS_STR(args[3].u_obj))
    {
        const char *str = mp_obj_str_get_str(args[3].u_obj);
        if((strcmp(str, "none") == 0) || (strcmp(str, "None") == 0) || (strcmp(str, "NONE") == 0))
        {
            self->parity = UART_PARITY_NONE;
        }
        else if((strcmp(str, "odd") == 0) || (strcmp(str, "Odd") == 0) || (strcmp(str, "ODD") == 0))
        {
            self->parity = UART_PARITY_ODD;
        }
        else if((strcmp(str, "even") == 0) || (strcmp(str, "Even") == 0) || (strcmp(str, "EVEN") == 0))
        {
            self->parity = UART_PARITY_EVEN;
        }
        else if((strcmp(str, "space") == 0) || (strcmp(str, "Space") == 0) || (strcmp(str, "SPACE") == 0))
        {
            self->parity = UART_PARITY_SPACE;
        }
        else if((strcmp(str, "mark") == 0) || (strcmp(str, "Mark") == 0) || (strcmp(str, "MARK") == 0))
        {
            self->parity = UART_PARITY_MARK;
        }
        else
        {
            goto invalid_args;
        }
    }
    else
    {
        goto invalid_args;
    }

    self->stopbits = args[4].u_int;
    if((self->stopbits < 1) || (self->stopbits > 2))
    {
        goto invalid_args;
    }

    self->irq_trigger = args[5].u_int;

    self->irq_callback = args[6].u_obj;

    self->irq_priority = args[7].u_int;
    if(self->irq_priority > 15)
    {
        goto invalid_args;
    }

    self->read_buf_head = 0;
    self->read_buf_tail = 0;
    self->read_buf = MP_OBJ_NULL; // free the read buffer before allocating again
    self->read_buf = m_new(byte, PYBUART_RX_BUFFER_LEN);

    if(args[8].u_obj != mp_const_none)
    {
        pin_config_by_func(args[8].u_obj, "%s_UART%d_RXD", self->uart_id);
    }

    if(args[9].u_obj != mp_const_none)
    {
        pin_config_by_func(args[9].u_obj, "%s_UART%d_TXD", self->uart_id);
    }

    if(args[10].u_obj != mp_const_none)
    {
        pin_config_by_func(args[10].u_obj, "%s_UART%d_nRTS", self->uart_id);
    }

    if(args[11].u_obj != mp_const_none)
    {
        pin_config_by_func(args[11].u_obj, "%s_UART%d_nCTS", self->uart_id);
    }

    switch(self->uart_id) {
    case PYB_UART_0:
        CLK_EnableModuleClock(UART0_MODULE);
        CLK_SetModuleClock(UART0_MODULE, CLK_CLKSEL1_UART0SEL_HXT, CLK_CLKDIV0_UART0(1));
        break;

    case PYB_UART_1:
        CLK_EnableModuleClock(UART1_MODULE);
        CLK_SetModuleClock(UART1_MODULE, CLK_CLKSEL1_UART1SEL_HXT, CLK_CLKDIV0_UART1(1));
        break;

    case PYB_UART_2:
        CLK_EnableModuleClock(UART2_MODULE);
        CLK_SetModuleClock(UART2_MODULE, CLK_CLKSEL3_UART2SEL_HXT, CLK_CLKDIV4_UART2(1));
        break;

    case PYB_UART_3:
        CLK_EnableModuleClock(UART3_MODULE);
        CLK_SetModuleClock(UART3_MODULE, CLK_CLKSEL3_UART3SEL_HXT, CLK_CLKDIV4_UART3(1));
        break;

    case PYB_UART_4:
        CLK_EnableModuleClock(UART4_MODULE);
        CLK_SetModuleClock(UART4_MODULE, CLK_CLKSEL3_UART4SEL_HXT, CLK_CLKDIV4_UART4(1));
        break;

    case PYB_UART_5:
        CLK_EnableModuleClock(UART5_MODULE);
        CLK_SetModuleClock(UART5_MODULE, CLK_CLKSEL3_UART5SEL_HXT, CLK_CLKDIV4_UART5(1));
        break;

    default:
        break;
    }

    uint databits = UART_WORD_LEN_8;
    switch(self->databits) {
    case 8:  databits = UART_WORD_LEN_8; break;
    case 7:  databits = UART_WORD_LEN_7; break;
    case 6:  databits = UART_WORD_LEN_6; break;
    case 5:  databits = UART_WORD_LEN_5; break;
    default: databits = UART_WORD_LEN_8; break;
    }
    uint stopbits = UART_STOP_BIT_1;
    switch(self->stopbits) {
    case 1:  stopbits = UART_STOP_BIT_1; break;
    case 2:  stopbits = UART_STOP_BIT_2; break;
    default: stopbits = UART_STOP_BIT_1; break;
    }
    UART_Open(self->UARTx, self->baudrate);
    self->UARTx->LINE = databits | self->parity | stopbits;

    UART_SetTimeoutCnt(self->UARTx, 100);       // 10 character
    self->UARTx->FIFO &= ~UART_FIFO_RFITL_Msk;
    self->UARTx->FIFO |= UART_FIFO_RFITL_8BYTES;
    UART_EnableInt(self->UARTx, UART_INTEN_RDAIEN_Msk | UART_INTEN_RXTOIEN_Msk);

    NVIC_SetPriority(self->IRQn, self->irq_priority);
    NVIC_EnableIRQ(self->IRQn);

    return self;

invalid_args:
    mp_raise_ValueError("invalid argument(s) value");
}


STATIC mp_obj_t pyb_uart_any(mp_obj_t self_in) {
    pyb_uart_obj_t *self = self_in;

    return mp_obj_new_int(uart_rx_any(self));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_any_obj, pyb_uart_any);


STATIC mp_uint_t pyb_uart_read(mp_obj_t self_in, void *buf_in, mp_uint_t size, int *errcode) {
    pyb_uart_obj_t *self = self_in;
    byte *buf = buf_in;

    // make sure we want at least 1 char
    if (size == 0) {
        return 0;
    }

    // wait for first char to become available
    if (!uart_rx_wait(self)) {
        // return MP_EAGAIN error to indicate non-blocking (then read() method returns None)
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    // read the data
    byte *orig_buf = buf;
    for ( ; ; ) {
        *buf++ = uart_rx_char(self);
        if (--size == 0 || !uart_rx_wait(self)) {
            // return number of bytes read
            return buf - orig_buf;
        }
    }
}


STATIC mp_uint_t pyb_uart_write(mp_obj_t self_in, const void *buf_in, mp_uint_t size, int *errcode) {
    pyb_uart_obj_t *self = self_in;
    const char *buf = buf_in;

    // write the data
    if (!uart_tx_strn(self, buf, size)) {
        mp_raise_OSError(MP_EIO);
    }
    return size;
}


STATIC mp_uint_t pyb_uart_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t arg, int *errcode) {
    pyb_uart_obj_t *self = self_in;
    mp_uint_t ret;

    if (request == MP_STREAM_POLL) {
        mp_uint_t flags = arg;
        ret = 0;
        if ((flags & MP_STREAM_POLL_RD) && uart_rx_any(self)) {
            ret |= MP_STREAM_POLL_RD;
        }
        if ((flags & MP_STREAM_POLL_WR) && (!UART_IS_TX_FULL(self->UARTx))) {
            ret |= MP_STREAM_POLL_WR;
        }
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}


STATIC void UARTX_Handler(uint32_t uart_id) {
    pyb_uart_obj_t *self = &pyb_uart_obj[uart_id];

    if(UART_GET_INT_FLAG(self->UARTx, UART_INTSTS_RDAIF_Msk | UART_INTSTS_RXTOIF_Msk))
    {
        while((self->UARTx->FIFOSTS & UART_FIFOSTS_RXEMPTY_Msk) == 0)
        {
            uint chr = self->UARTx->DAT & UART_DAT_DAT_Msk;

            if (MP_STATE_PORT(os_term_dup_obj) && MP_STATE_PORT(os_term_dup_obj)->stream_o == self && chr == mp_interrupt_char) {
                // raise an exception when interrupts are finished
                mp_keyboard_interrupt();
            } else {
                uint next_head = (self->read_buf_head + 1) % PYBUART_RX_BUFFER_LEN;
                if (next_head != self->read_buf_tail) {
                    self->read_buf[self->read_buf_head] = chr;
                    self->read_buf_head = next_head;
                }
            }
        }
    }
}

void UART0_IRQHandler(void) {
    UARTX_Handler(PYB_UART_0);
}

void UART1_IRQHandler(void) {
    UARTX_Handler(PYB_UART_1);
}

void UART2_IRQHandler(void) {
    UARTX_Handler(PYB_UART_2);
}

void UART3_IRQHandler(void) {
    UARTX_Handler(PYB_UART_3);
}

void UART4_IRQHandler(void) {
    UARTX_Handler(PYB_UART_4);
}

void UART5_IRQHandler(void) {
    UARTX_Handler(PYB_UART_5);
}

STATIC const mp_stream_p_t uart_stream_p = {
    .read = pyb_uart_read,
    .write = pyb_uart_write,
    .ioctl = pyb_uart_ioctl,
    .is_text = false,
};


STATIC const mp_rom_map_elem_t pyb_uart_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_any),         MP_ROM_PTR(&pyb_uart_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),        MP_ROM_PTR(&mp_stream_read_obj) },                  // read([nbytes])
    { MP_ROM_QSTR(MP_QSTR_readline),    MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },   // readline()
    { MP_ROM_QSTR(MP_QSTR_readinto),    MP_ROM_PTR(&mp_stream_readinto_obj) },              // readinto(buf[, nbytes])
    { MP_ROM_QSTR(MP_QSTR_write),       MP_ROM_PTR(&mp_stream_write_obj) },                 // write(buf)

    // class constants
    { MP_ROM_QSTR(MP_QSTR_PARITY_NONE), MP_ROM_INT(UART_PARITY_NONE) },
    { MP_ROM_QSTR(MP_QSTR_PARITY_ODD),  MP_ROM_INT(UART_PARITY_ODD) },
    { MP_ROM_QSTR(MP_QSTR_PARITY_EVEN), MP_ROM_INT(UART_PARITY_EVEN) },
    { MP_ROM_QSTR(MP_QSTR_PARITY_ONE),  MP_ROM_INT(UART_PARITY_MARK) },
    { MP_ROM_QSTR(MP_QSTR_PARITY_ZERO), MP_ROM_INT(UART_PARITY_SPACE) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_uart_locals_dict, pyb_uart_locals_dict_table);

const mp_obj_type_t pyb_uart_type = {
    { &mp_type_type },
    .name = MP_QSTR_UART,
    .print = pyb_uart_print,
    .make_new = pyb_uart_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &uart_stream_p,
    .locals_dict = (mp_obj_t)&pyb_uart_locals_dict,
};
