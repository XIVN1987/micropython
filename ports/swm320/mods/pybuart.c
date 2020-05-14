#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/gc.h"
#include "py/objlist.h"
#include "py/stream.h"
#include "py/mphal.h"

#include "lib/utils/interrupt_char.h"

#include "mods/pybpin.h"
#include "mods/pybuart.h"


/// \moduleref pyb
/// \class UART - duplex serial communication bus

/******************************************************************************
 DEFINE CONSTANTS
 *******-***********************************************************************/
#define UART_FRAME_TIME_US(baud)    ((11 * 1000000) / baud)

#define UART_RX_BUFFER_LEN          (256)

#define UART_IRQ_RXAny     1
#define UART_IRQ_TXEmpty   2
#define UART_IRQ_TIMEOUT   4


/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef enum {
    PYB_UART_0   =  0,
    PYB_UART_1   =  1,
    PYB_UART_2   =  2,
    PYB_UART_3   =  3,
    PYB_NUM_UARTS
} pyb_uart_id_t;

struct _pyb_uart_obj_t {
    mp_obj_base_t base;
    pyb_uart_id_t uart_id;
    UART_TypeDef *UARTx;
    uint32_t baudrate;
    uint8_t databits;
    uint8_t parity;
    uint8_t stopbits;

    uint8_t  IRQn;
    uint8_t  irq_flags;
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
    { {&pyb_uart_type}, .uart_id = PYB_UART_0, .UARTx = UART0, .IRQn = UART0_IRQn, .read_buf = NULL },
    { {&pyb_uart_type}, .uart_id = PYB_UART_1, .UARTx = UART1, .IRQn = UART1_IRQn, .read_buf = NULL },
    { {&pyb_uart_type}, .uart_id = PYB_UART_2, .UARTx = UART2, .IRQn = UART2_IRQn, .read_buf = NULL },
    { {&pyb_uart_type}, .uart_id = PYB_UART_3, .UARTx = UART3, .IRQn = UART3_IRQn, .read_buf = NULL },
};


/******************************************************************************
 DEFINE PUBLIC FUNCTIONS
 ******************************************************************************/
void uart_init0(void)
{
    // save references of the UART objects, to prevent the read buffers from being trashed by the gc
    MP_STATE_PORT(pyb_uart_objs)[0] = &pyb_uart_obj[0];
    MP_STATE_PORT(pyb_uart_objs)[1] = &pyb_uart_obj[1];
    MP_STATE_PORT(pyb_uart_objs)[2] = &pyb_uart_obj[2];
    MP_STATE_PORT(pyb_uart_objs)[3] = &pyb_uart_obj[3];
}


uint uart_rx_any(pyb_uart_obj_t *self)
{
    if(self->read_buf_head == self->read_buf_tail)
        return 0;
    else if(self->read_buf_head > self->read_buf_tail)
        return self->read_buf_head - self->read_buf_tail;
    else
        return UART_RX_BUFFER_LEN + self->read_buf_head - self->read_buf_tail;
}


uint uart_rx_char(pyb_uart_obj_t *self)
{
    uint data = self->read_buf[self->read_buf_tail];
    self->read_buf_tail = (self->read_buf_tail + 1) % UART_RX_BUFFER_LEN;

    return data;
}


// Waits at most timeout microseconds for at least 1 char to become ready for reading
// Returns true if something available, false if not.
bool uart_rx_wait(pyb_uart_obj_t *self)
{
    uint timeout = UART_FRAME_TIME_US(self->baudrate) * 2;   // 2 charaters time

    while(1)
    {
        if(uart_rx_any(self))
        {
            return true; // we have at least 1 char ready for reading
        }

        if(timeout > 0)
        {
            mp_hal_delay_us(1);
            timeout--;
        }
        else
        {
            return false;
        }
    }
}


void uart_tx_char(pyb_uart_obj_t *self, int chr)
{
    while(UART_IsTXFIFOFull(self->UARTx)) __NOP();

    self->UARTx->DATA = chr;
}


void uart_tx_strn(pyb_uart_obj_t *self, const char *str, uint len)
{
    for(const char *top = str + len; str < top; str++)
    {
        uart_tx_char(self, *str);
    }
}


/******************************************************************************/
/* MicroPython bindings                                                       */

STATIC void uart_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_uart_obj_t *self = self_in;

    mp_printf(print, "UART(%u, baudrate=%u, bits=%u, ", self->uart_id, self->baudrate, self->databits);
    switch (self->parity)
    {
    case UART_PARITY_NONE:  mp_printf(print, "parity=None, "); break;
    case UART_PARITY_EVEN:  mp_printf(print, "parity=Even, "); break;
    case UART_PARITY_ODD:   mp_printf(print, "parity=Odd, ");  break;
    case UART_PARITY_ZERO:  mp_printf(print, "parity=Zero, "); break;
    case UART_PARITY_ONE:   mp_printf(print, "parity=One, ");  break;
    default:break;
    }
    mp_printf(print, "stop=%u)", self->stopbits);
}


STATIC mp_obj_t uart_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_id, ARG_baudrate, ARG_bits, ARG_parity, ARG_stop, ARG_irq, ARG_callback, ARG_priority, ARG_timeout, ARG_rxd, ARG_txd };
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_baudrate, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 9600} },
        { MP_QSTR_bits,     MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_parity,   MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_stop,     MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 1} },
        { MP_QSTR_irq,      MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_callback, MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_priority, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 4} },
        { MP_QSTR_timeout,  MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 10} },
        { MP_QSTR_rxd,      MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_txd,      MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint uart_id = args[ARG_id].u_int;
    if(uart_id >= PYB_NUM_UARTS)
    {
        mp_raise_OSError(MP_ENODEV);
    }
    pyb_uart_obj_t *self = &pyb_uart_obj[uart_id];

    self->baudrate = args[ARG_baudrate].u_int;

    self->databits = args[ARG_bits].u_int;
    if((self->databits < 8) || (self->databits > 9))
    {
        mp_raise_ValueError("invalid bits value");
    }

    if(args[ARG_parity].u_obj == mp_const_none)
    {
        self->parity = UART_PARITY_NONE;
    }
    else if(MP_OBJ_IS_INT(args[ARG_parity].u_obj))
    {
        self->parity = mp_obj_get_int(args[ARG_parity].u_obj);
    }
    else if(MP_OBJ_IS_STR(args[ARG_parity].u_obj))
    {
        const char *str = mp_obj_str_get_str(args[ARG_parity].u_obj);
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
        else if((strcmp(str, "one") == 0) || (strcmp(str, "One") == 0) || (strcmp(str, "ONE") == 0))
        {
            self->parity = UART_PARITY_ONE;
        }
        else if((strcmp(str, "zero") == 0) || (strcmp(str, "Zero") == 0) || (strcmp(str, "ZERO") == 0))
        {
            self->parity = UART_PARITY_ZERO;
        }
        else
        {
            mp_raise_ValueError("invalid parity value");
        }
    }

    self->stopbits = args[ARG_stop].u_int;
    if((self->stopbits < 1) || (self->stopbits > 2))
    {
        mp_raise_ValueError("invalid stop value");
    }

    self->irq_trigger  = args[ARG_irq].u_int;
    self->irq_callback = args[ARG_callback].u_obj;
    self->irq_priority = args[ARG_priority].u_int;
    if(self->irq_priority > 7)
    {
        mp_raise_ValueError("invalid priority value");
    }
    else
    {
        NVIC_SetPriority(self->IRQn, self->irq_priority);
    }

    self->read_buf_head = 0;
    self->read_buf_tail = 0;
    self->read_buf = MP_OBJ_NULL; // free the read buffer before allocating again
    self->read_buf = m_new(byte, UART_RX_BUFFER_LEN);

    pin_obj_t *pin_rxd = pin_find_by_name(args[ARG_rxd].u_obj);
    if((pin_rxd->pbit % 2) == 1)
        mp_raise_ValueError("UART RXD need be Even number pin, like PA0, PA2, PA4");

    pin_obj_t *pin_txd = pin_find_by_name(args[ARG_txd].u_obj);
    if((pin_txd->pbit % 2) == 0)
        mp_raise_ValueError("UART TXD need be Odd  number pin, like PA1, PA3, PA5");

    switch(self->uart_id) {
    case PYB_UART_0:
        pin_config(pin_txd, FUNMUX1_UART0_TXD, 0, 0);
        pin_config(pin_rxd, FUNMUX0_UART0_RXD, 0, 0);
        break;

    case PYB_UART_1:
        pin_config(pin_txd, FUNMUX1_UART1_TXD, 0, 0);
        pin_config(pin_rxd, FUNMUX0_UART1_RXD, 0, 0);
        break;

    case PYB_UART_2:
        pin_config(pin_txd, FUNMUX1_UART2_TXD, 0, 0);
        pin_config(pin_rxd, FUNMUX0_UART2_RXD, 0, 0);
        break;

    case PYB_UART_3:
        pin_config(pin_txd, FUNMUX1_UART3_TXD, 0, 0);
        pin_config(pin_rxd, FUNMUX0_UART3_RXD, 0, 0);
        break;

    default:
        break;
    }

    uint databits;
    switch(self->databits)
    {
    case 8:  databits = UART_DATA_8BIT;  break;
    case 9:  databits = UART_DATA_9BIT;  break;
    default: databits = UART_DATA_8BIT;  break;
    }

    uint stopbits;
    switch(self->stopbits)
    {
    case 1:  stopbits = UART_STOP_1BIT;  break;
    case 2:  stopbits = UART_STOP_2BIT;  break;
    default: stopbits = UART_STOP_1BIT;  break;
    }

    uint timeout = args[ARG_timeout].u_int;
    if(timeout > 0xFF)
    {
        mp_raise_ValueError("invalid timeout value");
    }

    UART_InitStructure UART_initStruct;
    UART_initStruct.Baudrate = self->baudrate;
    UART_initStruct.DataBits = databits;
    UART_initStruct.Parity = self->parity;
    UART_initStruct.StopBits = stopbits;
    UART_initStruct.RXThreshold = 4;
    UART_initStruct.RXThresholdIEn = 1;
    UART_initStruct.TimeoutTime = timeout;   //timeout个字符时间内未接收到新的数据则触发超时中断
    UART_initStruct.TimeoutIEn = 1;
    UART_initStruct.TXThreshold = 2;
    UART_initStruct.TXThresholdIEn = 0;
    UART_Init(self->UARTx, &UART_initStruct);
    UART_Open(self->UARTx);

    return self;
}


STATIC mp_obj_t uart_baudrate(size_t n_args, const mp_obj_t *args)
{
    pyb_uart_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->baudrate);
    }
    else                // set
    {
        self->baudrate = mp_obj_get_int(args[1]);

        UART_SetBaudrate(self->UARTx, self->baudrate);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(uart_baudrate_obj, 1, 2, uart_baudrate);


STATIC mp_obj_t uart_any(mp_obj_t self_in)
{
    pyb_uart_obj_t *self = self_in;

    return mp_obj_new_int(uart_rx_any(self));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(uart_any_obj, uart_any);


STATIC mp_obj_t uart_full(mp_obj_t self_in)
{
    pyb_uart_obj_t *self = self_in;

    return mp_obj_new_bool(UART_IsTXFIFOFull(self->UARTx));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(uart_full_obj, uart_full);


// used when uart_any() return non-zero
STATIC mp_obj_t uart_readchar(mp_obj_t self_in)
{
    pyb_uart_obj_t *self = self_in;

    return MP_OBJ_NEW_SMALL_INT(uart_rx_char(self));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(uart_readchar_obj, uart_readchar);


STATIC mp_obj_t uart_writechar(mp_obj_t self_in, mp_obj_t char_in)
{
    pyb_uart_obj_t *self = self_in;

    uint chr = mp_obj_get_int(char_in);

    uart_tx_char(self, chr);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(uart_writechar_obj, uart_writechar);


STATIC mp_obj_t uart_irq_flags(mp_obj_t self_in)
{
    pyb_uart_obj_t *self = self_in;

    return mp_obj_new_int(self->irq_flags);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(uart_irq_flags_obj, uart_irq_flags);


STATIC mp_obj_t uart_irq_enable(mp_obj_t self_in, mp_obj_t irq_trigger)
{
    pyb_uart_obj_t *self = self_in;

    uint trigger = mp_obj_get_int(irq_trigger);

    if(trigger & UART_IRQ_RXAny)
    {
        self->irq_trigger |= UART_IRQ_RXAny;
    }

    if(trigger & UART_IRQ_TXEmpty)
    {
        self->irq_trigger |= UART_IRQ_TXEmpty;

        UART_INTTXThresholdEn(self->UARTx);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(uart_irq_enable_obj, uart_irq_enable);


STATIC mp_obj_t uart_irq_disable(mp_obj_t self_in, mp_obj_t irq_trigger)
{
    pyb_uart_obj_t *self = self_in;

    uint trigger = mp_obj_get_int(irq_trigger);

    if(trigger & UART_IRQ_RXAny)
    {
        self->irq_trigger &= ~UART_IRQ_RXAny;
    }

    if(trigger & UART_IRQ_TXEmpty)
    {
        self->irq_trigger &= ~UART_IRQ_TXEmpty;

        UART_INTTXThresholdDis(self->UARTx);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(uart_irq_disable_obj, uart_irq_disable);


STATIC void fetch_one_char(pyb_uart_obj_t *self)
{
    uint32_t chr;

    if(UART_ReadByte(self->UARTx, &chr) == 0)
    {
        if(MP_STATE_VM(dupterm_objs[0]) && MP_STATE_VM(dupterm_objs[0]) == self && chr == mp_interrupt_char)
        {
            // raise an exception when interrupts are finished
            mp_keyboard_interrupt();
        }
        else
        {
            uint next_head = (self->read_buf_head + 1) % UART_RX_BUFFER_LEN;
            if(next_head != self->read_buf_tail)
            {
                self->read_buf[self->read_buf_head] = chr;
                self->read_buf_head = next_head;
            }
        }
    }
}

STATIC void UARTX_Handler(uint32_t uart_id)
{
    pyb_uart_obj_t *self = &pyb_uart_obj[uart_id];

    if(UART_INTRXThresholdStat(self->UARTx))
    {
        while((self->UARTx->FIFO & UART_FIFO_RXLVL_Msk) > 1)    // 留一个数据，确保总能触发超时中断
        {
            fetch_one_char(self);
        }

        if(self->irq_trigger & UART_IRQ_RXAny)
            self->irq_flags |= UART_IRQ_RXAny;
    }
    else if(UART_INTTimeoutStat(self->UARTx))
    {
        while(UART_IsRXFIFOEmpty(self->UARTx) == 0)
        {
            fetch_one_char(self);
        }

        if(self->irq_trigger & UART_IRQ_RXAny)
            self->irq_flags |= UART_IRQ_RXAny | UART_IRQ_TIMEOUT;
    }

    if(UART_INTTXThresholdStat(self->UARTx))
    {
        if(self->irq_trigger & UART_IRQ_TXEmpty)
            self->irq_flags |= UART_IRQ_TXEmpty;
    }

    if(self->irq_callback != mp_const_none)
    {
        gc_lock();
        nlr_buf_t nlr;
        if(nlr_push(&nlr) == 0)
        {
            mp_call_function_1(self->irq_callback, self);
            nlr_pop();
        }
        else
        {
            self->irq_callback = mp_const_none;

            printf("uncaught exception in UART(%u) interrupt handler\n", self->uart_id);
            mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
        }
        gc_unlock();
    }

    self->irq_flags = 0;
}

void UART0_Handler(void)
{
    UARTX_Handler(PYB_UART_0);
}

void UART1_Handler(void)
{
    UARTX_Handler(PYB_UART_1);
}

void UART2_Handler(void)
{
    UARTX_Handler(PYB_UART_2);
}

void UART3_Handler(void)
{
    UARTX_Handler(PYB_UART_3);
}


STATIC const mp_rom_map_elem_t uart_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_baudrate),     MP_ROM_PTR(&uart_baudrate_obj) },
    { MP_ROM_QSTR(MP_QSTR_any),          MP_ROM_PTR(&uart_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),         MP_ROM_PTR(&mp_stream_read_obj) },                  // read([nbytes])
    { MP_ROM_QSTR(MP_QSTR_readline),     MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },   // readline()
    { MP_ROM_QSTR(MP_QSTR_readinto),     MP_ROM_PTR(&mp_stream_readinto_obj) },              // readinto(buf[, nbytes])
    { MP_ROM_QSTR(MP_QSTR_write),        MP_ROM_PTR(&mp_stream_write_obj) },                 // write(buf)

    { MP_ROM_QSTR(MP_QSTR_full),         MP_ROM_PTR(&uart_full_obj) },
    { MP_ROM_QSTR(MP_QSTR_readchar),     MP_ROM_PTR(&uart_readchar_obj) },
    { MP_ROM_QSTR(MP_QSTR_writechar),    MP_ROM_PTR(&uart_writechar_obj) },

    { MP_ROM_QSTR(MP_QSTR_irq_flags),    MP_ROM_PTR(&uart_irq_flags_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq_enable),   MP_ROM_PTR(&uart_irq_enable_obj) },
    { MP_ROM_QSTR(MP_QSTR_irq_disable),  MP_ROM_PTR(&uart_irq_disable_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_PARITY_NONE),  MP_ROM_INT(UART_PARITY_NONE) },
    { MP_ROM_QSTR(MP_QSTR_PARITY_ODD),   MP_ROM_INT(UART_PARITY_ODD) },
    { MP_ROM_QSTR(MP_QSTR_PARITY_EVEN),  MP_ROM_INT(UART_PARITY_EVEN) },
    { MP_ROM_QSTR(MP_QSTR_PARITY_ONE),   MP_ROM_INT(UART_PARITY_ONE) },
    { MP_ROM_QSTR(MP_QSTR_PARITY_ZERO),  MP_ROM_INT(UART_PARITY_ZERO) },

    { MP_ROM_QSTR(MP_QSTR_IRQ_RXAny),    MP_ROM_INT(UART_IRQ_RXAny) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_TXEmpty),  MP_ROM_INT(UART_IRQ_TXEmpty) },
    { MP_ROM_QSTR(MP_QSTR_IRQ_TIMEOUT),  MP_ROM_INT(UART_IRQ_TIMEOUT) },
};
STATIC MP_DEFINE_CONST_DICT(uart_locals_dict, uart_locals_dict_table);


STATIC uint uart_read(mp_obj_t self_in, void *buf_in, uint len, int *errcode)
{
    pyb_uart_obj_t *self = self_in;
    byte *buf = buf_in;

    // make sure we want at least 1 char
    if(len == 0)
        return 0;

    // wait for first char to become available
    if(!uart_rx_wait(self))
    {
        // return MP_EAGAIN error to indicate non-blocking (then read() method returns None)
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }

    // read the data
    byte *orig_buf = buf;
    while(1)
    {
        *buf++ = uart_rx_char(self);
        if(--len == 0 || !uart_rx_wait(self))
        {
            // return number of bytes read
            return buf - orig_buf;
        }
    }
}


STATIC uint uart_write(mp_obj_t self_in, const void *buf_in, uint len, int *errcode)
{
    pyb_uart_obj_t *self = self_in;
    const char *buf = buf_in;

    uart_tx_strn(self, buf, len);

    return len;
}


STATIC uint uart_ioctl(mp_obj_t self_in, uint request, uint arg, int *errcode)
{
    pyb_uart_obj_t *self = self_in;
    uint ret = 0;

    if(request == MP_STREAM_POLL)
    {
        uint flags = arg;

        if((flags & MP_STREAM_POLL_RD) && uart_rx_any(self))
        {
            ret |= MP_STREAM_POLL_RD;
        }
        if((flags & MP_STREAM_POLL_WR) && (!UART_IsTXFIFOFull(self->UARTx)))
        {
            ret |= MP_STREAM_POLL_WR;
        }
    }
    else
    {
        *errcode = MP_EINVAL;
        ret = MP_STREAM_ERROR;
    }

    return ret;
}


STATIC const mp_stream_p_t uart_stream_p = {
    .read = uart_read,
    .write = uart_write,
    .ioctl = uart_ioctl,
    .is_text = false,
};


const mp_obj_type_t pyb_uart_type = {
    { &mp_type_type },
    .name = MP_QSTR_UART,
    .print = uart_print,
    .make_new = uart_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &uart_stream_p,
    .locals_dict = (mp_obj_t)&uart_locals_dict,
};
