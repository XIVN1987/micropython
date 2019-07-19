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

#include "MT7687.h"

#include "py/runtime.h"
#include "py/objlist.h"
#include "py/stream.h"
#include "py/mphal.h"
#include "lib/utils/interrupt_char.h"
#include "pybuart.h"
#include "mpirq.h"
#include "mperror.h"
#include "pybpin.h"
#include "pins.h"
#include "moduos.h"

/// \moduleref pyb
/// \class UART - duplex serial communication bus

/******************************************************************************
 DEFINE CONSTANTS
 *******-***********************************************************************/
#define PYBUART_FRAME_TIME_US(baud)             ((11 * 1000000) / baud)

#define PYBUART_RX_BUFFER_LEN                   (256)

// interrupt triggers
#define UART_TRIGGER_RX_ANY                     (0x01)
#define UART_TRIGGER_RX_HALF                    (0x02)
#define UART_TRIGGER_RX_FULL                    (0x04)
#define UART_TRIGGER_TX_DONE                    (0x08)

/******************************************************************************
 DECLARE PRIVATE FUNCTIONS
 ******************************************************************************/
STATIC void uart_init (pyb_uart_obj_t *self);
STATIC bool uart_rx_wait (pyb_uart_obj_t *self);
STATIC mp_obj_t uart_irq_new (pyb_uart_obj_t *self, byte trigger, mp_int_t priority, mp_obj_t handler);
STATIC void uart_irq_enable (mp_obj_t self_in);
STATIC void uart_irq_disable (mp_obj_t self_in);

/******************************************************************************
 DEFINE PRIVATE TYPES
 ******************************************************************************/
struct _pyb_uart_obj_t {
    mp_obj_base_t base;
    pyb_uart_id_t uart_id;
    UART_TypeDef *UARTx;
    uint baudrate;
    byte databits;
    byte parity;
    byte stopbits;

    byte *read_buf;                     // read buffer pointer
    volatile uint16_t read_buf_head;    // indexes first empty slot
    uint16_t read_buf_tail;             // indexes first full slot (not full if equals head)

    byte irq_trigger;
    bool irq_enabled;
    byte irq_flags;
};

/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_uart_obj_t pyb_uart_obj[PYB_NUM_UARTS] = {
    {.uart_id = PYB_UART_1, .UARTx = UART1, .baudrate = 0, .read_buf = NULL},
    {.uart_id = PYB_UART_2, .UARTx = UART2, .baudrate = 0, .read_buf = NULL}
};

STATIC const mp_irq_methods_t uart_irq_methods;

/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/
void uart_init0 (void) {
    // save references of the UART objects, to prevent the read buffers from being trashed by the gc
    MP_STATE_PORT(pyb_uart_objs)[0] = &pyb_uart_obj[0];
    MP_STATE_PORT(pyb_uart_objs)[1] = &pyb_uart_obj[1];
}

uint32_t uart_rx_any(pyb_uart_obj_t *self) {
    if (self->read_buf_tail != self->read_buf_head) {
        // buffering via irq
        return (self->read_buf_head > self->read_buf_tail) ? self->read_buf_head - self->read_buf_tail :
                PYBUART_RX_BUFFER_LEN - self->read_buf_tail + self->read_buf_head;
    }
    return self->UARTx->LSR & UART_LSR_RDA_Msk ? 1 : 0;
}

int uart_rx_char(pyb_uart_obj_t *self) {
    if (self->read_buf_tail != self->read_buf_head) {
        // buffering via irq
        int data = self->read_buf[self->read_buf_tail];
        self->read_buf_tail = (self->read_buf_tail + 1) % PYBUART_RX_BUFFER_LEN;
        return data;
    } else {
        // no buffering
        return self->UARTx->RBR;
    }
}

bool uart_tx_char(pyb_uart_obj_t *self, int c) {
    while(!(self->UARTx->LSR & UART_LSR_THRE_Msk)) __NOP();

    self->UARTx->THR = c;

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
// assumes init parameters have been set up correctly
STATIC void uart_init (pyb_uart_obj_t *self) {

    // re-allocate the read buffer after resetting the uart (which automatically disables any irqs)
    self->read_buf_head = 0;
    self->read_buf_tail = 0;
    self->read_buf = MP_OBJ_NULL; // free the read buffer before allocating again
    self->read_buf = m_new(byte, PYBUART_RX_BUFFER_LEN);

    switch (self->uart_id) {
    case PYB_UART_1:
        PORT_Init(GPIO2, GPIO2_UART1_RX);
        PORT_Init(GPIO3, GPIO3_UART1_TX);
        break;

    case PYB_UART_2:
        PORT_Init(GPIO36, GPIO36_UART2_RX);
        PORT_Init(GPIO37, GPIO37_UART2_TX);
        break;

    default:
        break;
    }

    UART_Init(self->UARTx, self->baudrate, self->databits, self->parity, self->stopbits);
}

// Waits at most timeout microseconds for at least 1 char to become ready for
// reading (from buf or for direct reading).
// Returns true if something available, false if not.
STATIC bool uart_rx_wait (pyb_uart_obj_t *self) {
    int timeout = PYBUART_FRAME_TIME_US(self->baudrate) * 10;   // 10 charaters time
    for ( ; ; ) {
        if (uart_rx_any(self)) {
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

STATIC mp_obj_t uart_irq_new (pyb_uart_obj_t *self, byte trigger, mp_int_t priority, mp_obj_t handler) {
    // disable the uart interrupts before updating anything
    uart_irq_disable (self);

    switch (self->uart_id) {
    case PYB_UART_1:
        NVIC_SetPriority(UART1_IRQ, priority);
        break;

    case PYB_UART_2:
        NVIC_SetPriority(UART2_IRQ, priority);
        break;

    default:
        break;
    }

    // create the callback
    mp_obj_t _irq = mp_irq_new ((mp_obj_t)self, handler, &uart_irq_methods);

    // enable the interrupts now
    self->irq_trigger = trigger;
    uart_irq_enable (self);
    return _irq;
}

STATIC void UARTGenericIntHandler(uint32_t uart_id) {
    pyb_uart_obj_t *self = &pyb_uart_obj[uart_id];
    uint32_t status = self->UARTx->IIR & UART_IIR_IID_Msk;

    // receive interrupt
    if ((status == UART_IIR_RDA) || (status == UART_IIR_TIMEOUT)) {
        // set the flags
        self->irq_flags = UART_TRIGGER_RX_ANY;

        while (self->UARTx->LSR & UART_LSR_RDA_Msk) {
            int data = self->UARTx->RBR;
            if (MP_STATE_PORT(os_term_dup_obj) && MP_STATE_PORT(os_term_dup_obj)->stream_o == self && data == mp_interrupt_char) {
                // raise an exception when interrupts are finished
                mp_keyboard_interrupt();
            } else { // there's always a read buffer available
                uint16_t next_head = (self->read_buf_head + 1) % PYBUART_RX_BUFFER_LEN;
                if (next_head != self->read_buf_tail) {
                    // only store data if room in buf
                    self->read_buf[self->read_buf_head] = data;
                    self->read_buf_head = next_head;
                }
            }
        }
    }

    // check the flags to see if the user handler should be called
    if ((self->irq_trigger & self->irq_flags) && self->irq_enabled) {
        // call the user defined handler
        mp_irq_handler(mp_irq_find(self));
    }

    // clear the flags
    self->irq_flags = 0;
}

void UART1_Handler(void) {
    UARTGenericIntHandler(PYB_UART_1);
}

void UART2_Handler(void) {
    UARTGenericIntHandler(PYB_UART_2);
}

STATIC void uart_irq_enable (mp_obj_t self_in) {
    pyb_uart_obj_t *self = self_in;
    // check for any of the rx interrupt types
    if (self->irq_trigger & (UART_TRIGGER_RX_ANY | UART_TRIGGER_RX_HALF | UART_TRIGGER_RX_FULL)) {
        self->UARTx->IER |= (1 << UART_IER_RDA_Pos);
    }
    self->irq_enabled = true;
}

STATIC void uart_irq_disable (mp_obj_t self_in) {
    pyb_uart_obj_t *self = self_in;
    self->irq_enabled = false;
}

STATIC int uart_irq_flags (mp_obj_t self_in) {
    pyb_uart_obj_t *self = self_in;
    return self->irq_flags;
}

/******************************************************************************/
/* MicroPython bindings                                                      */

STATIC void pyb_uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_uart_obj_t *self = self_in;
    if (self->baudrate > 0) {
        mp_printf(print, "UART(%u, baudrate=%u, bits=", self->uart_id + 1, self->baudrate);
        switch (self->databits) {
        case UART_DATA_5BIT:
            mp_print_str(print, "5");
            break;
        case UART_DATA_6BIT:
            mp_print_str(print, "6");
            break;
        case UART_DATA_7BIT:
            mp_print_str(print, "7");
            break;
        case UART_DATA_8BIT:
            mp_print_str(print, "8");
            break;
        default:
            break;
        }
        if (self->parity == UART_PARITY_NONE) {
            mp_print_str(print, ", parity=None");
        } else {
            mp_printf(print, ", parity=UART.%q", self->parity == UART_PARITY_EVEN ? MP_QSTR_EVEN : MP_QSTR_ODD);
        }
        mp_printf(print, ", stop=%u)", self->stopbits == UART_STOP_1BIT ? 1 : 2);
    }
    else {
        mp_printf(print, "UART(%u)", self->uart_id);
    }
}

STATIC mp_obj_t pyb_uart_init_helper(pyb_uart_obj_t *self, const mp_arg_val_t *args) {
    // get the baudrate
    if (args[0].u_int <= 0) {
        goto error;
    }
    uint baudrate = args[0].u_int;
    // data bits
    switch (args[1].u_int) {
    case 5:
        self->databits = UART_DATA_5BIT;
        break;
    case 6:
        self->databits = UART_DATA_6BIT;
        break;
    case 7:
        self->databits = UART_DATA_7BIT;
        break;
    case 8:
        self->databits = UART_DATA_8BIT;
        break;
    default:
        goto error;
        break;
    }
    // parity
    if (args[2].u_obj == mp_const_none) {
        self->parity = UART_PARITY_NONE;
    } else {
        uint parity = mp_obj_get_int(args[2].u_obj);
        if (parity == 0) {
            self->parity = UART_PARITY_EVEN;
        } else if (parity == 1) {
            self->parity = UART_PARITY_ODD;
        } else {
            goto error;
        }
    }
    // stop bits
    self->stopbits = (args[3].u_int == 1 ? UART_STOP_1BIT : UART_STOP_2BIT);

    self->baudrate = baudrate;

    // initialize and enable the uart
    uart_init (self);

    // enable the callback
    uart_irq_new (self, UART_TRIGGER_RX_ANY, NVIC_PRIORITY_LVL_3, mp_const_none);
    // disable the irq (from the user point of view)
    uart_irq_disable(self);

    return mp_const_none;

error:
    mp_raise_ValueError(mpexception_value_invalid_arguments);
}

STATIC const mp_arg_t pyb_uart_init_args[] = {
    { MP_QSTR_id,                             MP_ARG_OBJ,  {.u_obj = MP_OBJ_NULL} },
    { MP_QSTR_baudrate,                       MP_ARG_INT,  {.u_int = 9600} },
    { MP_QSTR_bits,                           MP_ARG_INT,  {.u_int = 8} },
    { MP_QSTR_parity,                         MP_ARG_OBJ,  {.u_obj = mp_const_none} },
    { MP_QSTR_stop,                           MP_ARG_INT,  {.u_int = 1} },
};
STATIC mp_obj_t pyb_uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_uart_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(args), pyb_uart_init_args, args);

    // work out the uart id
    uint uart_id;
    if (args[0].u_obj == MP_OBJ_NULL) {
        // default id
        uart_id = PYB_UART_1;
    } else {
        uart_id = mp_obj_get_int(args[0].u_obj) - 1;
    }

    if (uart_id > PYB_UART_2) {
        mp_raise_OSError(MP_ENODEV);
    }

    // get the correct uart instance
    pyb_uart_obj_t *self = &pyb_uart_obj[uart_id];
    self->base.type = &pyb_uart_type;
    self->uart_id = uart_id;

    // start the peripheral
    pyb_uart_init_helper(self, &args[1]);

    return self;
}

STATIC mp_obj_t pyb_uart_init(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_uart_init_args) - 1];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(args), &pyb_uart_init_args[1], args);
    return pyb_uart_init_helper(pos_args[0], args);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_uart_init_obj, 1, pyb_uart_init);

STATIC mp_obj_t pyb_uart_deinit(mp_obj_t self_in) {
    pyb_uart_obj_t *self = self_in;

    self->UARTx->IER = 0;

    // invalidate the baudrate
    self->baudrate = 0;
    // free the read buffer
    m_del(byte, self->read_buf, PYBUART_RX_BUFFER_LEN);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_deinit_obj, pyb_uart_deinit);

STATIC mp_obj_t pyb_uart_any(mp_obj_t self_in) {
    pyb_uart_obj_t *self = self_in;

    return mp_obj_new_int(uart_rx_any(self));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_any_obj, pyb_uart_any);

STATIC mp_obj_t pyb_uart_sendbreak(mp_obj_t self_in) {
    pyb_uart_obj_t *self = self_in;

    // send a break signal for at least 2 complete frames
    self->UARTx->LCR |=  UART_LCR_SB_Msk;
    mp_hal_delay_us(PYBUART_FRAME_TIME_US(self->baudrate) * 2);
    self->UARTx->LCR &= ~UART_LCR_SB_Msk;

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_uart_sendbreak_obj, pyb_uart_sendbreak);

/// \method irq(trigger, priority, handler, wake)
STATIC mp_obj_t pyb_uart_irq(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    mp_arg_val_t args[mp_irq_INIT_NUM_ARGS];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, mp_irq_INIT_NUM_ARGS, mp_irq_init_args, args);

    // check if any parameters were passed
    pyb_uart_obj_t *self = pos_args[0];

    // convert the priority to the correct value
    uint priority = mp_irq_translate_priority (args[1].u_int);

    // check the trigger
    uint trigger = mp_obj_get_int(args[0].u_obj);
    if (!trigger || trigger > (UART_TRIGGER_RX_ANY | UART_TRIGGER_RX_HALF | UART_TRIGGER_RX_FULL | UART_TRIGGER_TX_DONE)) {
        goto invalid_args;
    }

    // register a new callback
    return uart_irq_new (self, trigger, priority, args[2].u_obj);

invalid_args:
    mp_raise_ValueError(mpexception_value_invalid_arguments);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_uart_irq_obj, 1, pyb_uart_irq);

STATIC const mp_rom_map_elem_t pyb_uart_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_init),        MP_ROM_PTR(&pyb_uart_init_obj) },
    { MP_ROM_QSTR(MP_QSTR_deinit),      MP_ROM_PTR(&pyb_uart_deinit_obj) },
    { MP_ROM_QSTR(MP_QSTR_any),         MP_ROM_PTR(&pyb_uart_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_sendbreak),   MP_ROM_PTR(&pyb_uart_sendbreak_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq),         MP_ROM_PTR(&pyb_uart_irq_obj) },

    /// \method read([nbytes])
    { MP_ROM_QSTR(MP_QSTR_read),        MP_ROM_PTR(&mp_stream_read_obj) },
    /// \method readline()
    { MP_ROM_QSTR(MP_QSTR_readline),    MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    /// \method readinto(buf[, nbytes])
    { MP_ROM_QSTR(MP_QSTR_readinto),    MP_ROM_PTR(&mp_stream_readinto_obj) },
    /// \method write(buf)
    { MP_ROM_QSTR(MP_QSTR_write),       MP_ROM_PTR(&mp_stream_write_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_RX_ANY),      MP_ROM_INT(UART_TRIGGER_RX_ANY) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_uart_locals_dict, pyb_uart_locals_dict_table);

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
        if ((flags & MP_STREAM_POLL_WR) && (self->UARTx->LSR & UART_LSR_THRE_Msk)) {
            ret |= MP_STREAM_POLL_WR;
        }
    } else {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }
    return ret;
}

STATIC const mp_stream_p_t uart_stream_p = {
    .read = pyb_uart_read,
    .write = pyb_uart_write,
    .ioctl = pyb_uart_ioctl,
    .is_text = false,
};

STATIC const mp_irq_methods_t uart_irq_methods = {
    .init = pyb_uart_irq,
    .enable = uart_irq_enable,
    .disable = uart_irq_disable,
    .flags = uart_irq_flags
};

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
