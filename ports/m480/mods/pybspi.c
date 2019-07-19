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
#include <string.h>

#include "py/runtime.h"
#include "py/mperrno.h"

#include "chip/M480.h"

#include "mods/pybpin.h"
#include "mods/pybspi.h"

#include "misc/bufhelper.h"


#define SPI_CPHA_0  0
#define SPI_CPHA_1  1
#define SPI_CPOL_0  0
#define SPI_CPOL_1  1


/// \moduleref pyb
/// \class SPI - a master-driven serial protocol

/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef enum {
    PYB_SPI_0      =  0,
    PYB_SPI_1      =  1,
    PYB_SPI_2      =  2,
    PYB_SPI_3      =  3,
    PYB_NUM_SPIS
} pyb_spi_id_t;


typedef struct _pyb_spi_obj_t {
    mp_obj_base_t base;
    pyb_spi_id_t spi_id;
    SPI_T *SPIx;
    uint baudrate;
    byte polarity;
    byte phase;
    bool slave;
    bool msbf;
    byte bits;
} pyb_spi_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_spi_obj_t pyb_spi_obj[PYB_NUM_SPIS] = {
    {.spi_id = PYB_SPI_0, .SPIx = SPI0, .baudrate = 0},
    {.spi_id = PYB_SPI_1, .SPIx = SPI1, .baudrate = 0},
    {.spi_id = PYB_SPI_2, .SPIx = SPI2, .baudrate = 0},
    {.spi_id = PYB_SPI_3, .SPIx = SPI3, .baudrate = 0},
};


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
STATIC void pybspi_tx(pyb_spi_obj_t *self, const void *data)
{
    uint32_t txdata;

    switch((self->bits+7)/8) {
    case 1:  txdata = *((const uint8_t *) data); break;
    case 2:  txdata = *((const uint16_t *)data); break;
    case 3:
    case 4:  txdata = *((const uint32_t *)data); break;
    default: break;
    }

    while(SPI_GET_TX_FIFO_FULL_FLAG(self->SPIx)) __NOP();
    SPI_WRITE_TX(self->SPIx, txdata);
}


STATIC void pybspi_rx(pyb_spi_obj_t *self, void *data)
{
    while(SPI_GET_RX_FIFO_EMPTY_FLAG(self->SPIx)) __NOP();
    uint32_t rxdata = SPI_READ_RX(self->SPIx);

    if(data)
    {
        switch((self->bits+7)/8) {
        case 1:  *((uint8_t *) data) = rxdata; break;
        case 2:  *((uint16_t *)data) = rxdata; break;
        case 3:
        case 4:  *((uint32_t *)data) = rxdata; break;
        default: break;
        }
    }
}


STATIC void pybspi_transfer(pyb_spi_obj_t *self, const char *txdata, char *rxdata, uint32_t len, uint32_t *txchar) {
    for(uint i = 0; i < len; i += (self->bits+7)/8) {
        pybspi_tx(self, txdata ? (const void *)&txdata[i] : txchar);
        pybspi_rx(self, rxdata ? (      void *)&rxdata[i] : NULL);
    }
}


/******************************************************************************/
/* MicroPython bindings                                                      */
/******************************************************************************/
STATIC void pyb_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_spi_obj_t *self = self_in;

    mp_printf(print, "SPI(%u, baudrate=%u, polarity=%u, phase=%u, mode=%s, msbf=%s, bits=%u)",
        self->spi_id, self->baudrate, self->polarity, self->phase, self->slave ? "Slave" : "Master", self->msbf ? "True" : "False", self->bits);
}


static const mp_arg_t pyb_spi_init_args[] = {
    { MP_QSTR_id,        MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 0} },
    { MP_QSTR_baudrate,  MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 1000000} },    // 1MHz
    { MP_QSTR_polarity,  MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 0} },
    { MP_QSTR_phase,     MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 0} },
    { MP_QSTR_slave,     MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool= false} },
    { MP_QSTR_msbf,      MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool= true} },
    { MP_QSTR_bits,      MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 8} },
    { MP_QSTR_mosi,      MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
    { MP_QSTR_miso,      MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
    { MP_QSTR_clk,       MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
    { MP_QSTR_cs,        MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
};
STATIC mp_obj_t pyb_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_spi_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(pyb_spi_init_args), pyb_spi_init_args, args);

    // check the peripheral id
    uint spi_id = args[0].u_int;
    if(spi_id >= PYB_NUM_SPIS) {
        mp_raise_OSError(MP_ENODEV);
    }

    pyb_spi_obj_t *self = &pyb_spi_obj[spi_id];
    self->base.type = &pyb_spi_type;

    self->baudrate = args[1].u_int;

    self->polarity = args[2].u_int;
    if((self->polarity != SPI_CPOL_0) && (self->polarity != SPI_CPOL_1))
    {
        goto invalid_args;
    }

    self->phase    = args[3].u_int;
    if((self->phase != SPI_CPHA_0) && (self->phase != SPI_CPHA_1))
    {
        goto invalid_args;
    }

    self->slave = args[4].u_bool;

    self->msbf = args[5].u_bool;

    self->bits = args[6].u_int;
    if((self->bits < 4) || (self->bits > 32))
    {
        goto invalid_args;
    }

    if(args[7].u_obj != mp_const_none)
    {
        pin_config_by_func(args[7].u_obj, "%s_SPI%d_MOSI", self->spi_id);
    }

    if(args[8].u_obj != mp_const_none)
    {
        pin_config_by_func(args[8].u_obj, "%s_SPI%d_MISO", self->spi_id);
    }

    if(args[9].u_obj != mp_const_none)
    {
        pin_config_by_func(args[9].u_obj, "%s_SPI%d_CLK", self->spi_id);
    }

    if(args[10].u_obj != mp_const_none)
    {
        pin_config_by_func(args[10].u_obj, "%s_SPI%d_SS", self->spi_id);
    }

    switch(self->spi_id) {
    case PYB_SPI_0:
        CLK_EnableModuleClock(SPI0_MODULE);
        CLK_SetModuleClock(SPI0_MODULE, CLK_CLKSEL2_SPI0SEL_PCLK1, 0);
        break;

    case PYB_SPI_1:
        CLK_EnableModuleClock(SPI1_MODULE);
        CLK_SetModuleClock(SPI1_MODULE, CLK_CLKSEL2_SPI1SEL_PCLK0, 0);
        break;

    case PYB_SPI_2:
        CLK_EnableModuleClock(SPI2_MODULE);
        CLK_SetModuleClock(SPI2_MODULE, CLK_CLKSEL2_SPI2SEL_PCLK1, 0);
        break;

    case PYB_SPI_3:
        CLK_EnableModuleClock(SPI3_MODULE);
        CLK_SetModuleClock(SPI3_MODULE, CLK_CLKSEL2_SPI3SEL_PCLK0, 0);
        break;

    default:
        break;
    }

    uint mode;
    if((self->polarity == SPI_CPOL_0) && (self->phase = SPI_CPHA_0))      mode = SPI_MODE_0;
    else if((self->polarity == SPI_CPOL_0) && (self->phase = SPI_CPHA_1)) mode = SPI_MODE_1;
    else if((self->polarity == SPI_CPOL_1) && (self->phase = SPI_CPHA_0)) mode = SPI_MODE_2;
    else if((self->polarity == SPI_CPOL_1) && (self->phase = SPI_CPHA_1)) mode = SPI_MODE_3;

    if(self->msbf) SPI_SET_MSB_FIRST(self->SPIx);
    else           SPI_SET_LSB_FIRST(self->SPIx);
    SPI_Open(self->SPIx, self->slave ? SPI_SLAVE : SPI_MASTER, mode, self->bits, self->baudrate);

    return self;

invalid_args:
    mp_raise_ValueError("invalid argument(s) value");
}


STATIC mp_obj_t pyb_spi_write(mp_obj_t self_in, mp_obj_t buf)
{
    pyb_spi_obj_t *self = self_in;

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(buf, &bufinfo, data);

    // just send
    pybspi_transfer(self, (const char *)bufinfo.buf, NULL, bufinfo.len, NULL);

    return mp_obj_new_int(bufinfo.len);     // return the number of bytes written
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_spi_write_obj, pyb_spi_write);


static const mp_arg_t pyb_spi_read_args[] = {
    { MP_QSTR_nbytes,  MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    { MP_QSTR_write,   MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 0xFF} },
};
STATIC mp_obj_t pyb_spi_read(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args
    pyb_spi_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_spi_read_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(pyb_spi_read_args), pyb_spi_read_args, args);

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(args[0].u_obj, &vstr); // first arg need mp_obj_t type

    // just receive
    pybspi_transfer(self, NULL, vstr.buf, vstr.len, (uint32_t *)&args[1].u_int);

    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);     // return the received data
}

STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_spi_read_obj, 1, pyb_spi_read);


static const mp_arg_t pyb_spi_readinto_args[] = {
    { MP_QSTR_buf,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    { MP_QSTR_write,   MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 0xFF} },
};
STATIC mp_obj_t pyb_spi_readinto(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    // parse args
    pyb_spi_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_spi_readinto_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(pyb_spi_readinto_args), pyb_spi_readinto_args, args);

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(args[0].u_obj, &vstr);

    // just receive
    pybspi_transfer(self, NULL, vstr.buf, vstr.len, (uint32_t *)&args[1].u_int);

    return mp_obj_new_int(vstr.len);    // return the number of bytes received
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_spi_readinto_obj, 1, pyb_spi_readinto);


STATIC mp_obj_t pyb_spi_write_read(mp_obj_t self_in, mp_obj_t buf)
{
    pyb_spi_obj_t *self = self_in;

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(buf, &bufinfo, data);

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(MP_OBJ_NEW_SMALL_INT(bufinfo.len), &vstr); // first arg need mp_obj_t type

    // send and receive
    pybspi_transfer(self, (const char *)bufinfo.buf, vstr.buf, bufinfo.len, NULL);

    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);     // return the received data
}

STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_spi_write_read_obj, pyb_spi_write_read);


STATIC mp_obj_t pyb_spi_write_readinto (mp_obj_t self, mp_obj_t writebuf, mp_obj_t readbuf) {
    // get buffers to write from/read to
    mp_buffer_info_t bufinfo_write;
    uint8_t data_send[1];
    mp_buffer_info_t bufinfo_read;

    if(writebuf == readbuf) {
        // same object for writing and reading, it must be a r/w buffer
        mp_get_buffer_raise(writebuf, &bufinfo_write, MP_BUFFER_RW);
        bufinfo_read = bufinfo_write;
    } else {
        // get the buffer to write from
        pyb_buf_get_for_send(writebuf, &bufinfo_write, data_send);

        // get the read buffer
        mp_get_buffer_raise(readbuf, &bufinfo_read, MP_BUFFER_WRITE);
        if (bufinfo_read.len != bufinfo_write.len) {
            mp_raise_ValueError("invalid argument(s) value");
        }
    }

    // send and receive
    pybspi_transfer(self, (const char *)bufinfo_write.buf, bufinfo_read.buf, bufinfo_write.len, NULL);

    return mp_obj_new_int(bufinfo_write.len);   // return the number of transferred bytes
}

STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_spi_write_readinto_obj, pyb_spi_write_readinto);


STATIC const mp_rom_map_elem_t pyb_spi_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_write),               MP_ROM_PTR(&pyb_spi_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),                MP_ROM_PTR(&pyb_spi_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto),            MP_ROM_PTR(&pyb_spi_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_read),          MP_ROM_PTR(&pyb_spi_write_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_readinto),      MP_ROM_PTR(&pyb_spi_write_readinto_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_CPHA_0),              MP_ROM_INT(SPI_CPHA_0) },
    { MP_ROM_QSTR(MP_QSTR_CPHA_1),              MP_ROM_INT(SPI_CPHA_1) },
    { MP_ROM_QSTR(MP_QSTR_CPOL_0),              MP_ROM_INT(SPI_CPOL_0) },
    { MP_ROM_QSTR(MP_QSTR_CPOL_1),              MP_ROM_INT(SPI_CPOL_1) },
};

STATIC MP_DEFINE_CONST_DICT(pyb_spi_locals_dict, pyb_spi_locals_dict_table);


const mp_obj_type_t pyb_spi_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPI,
    .print = pyb_spi_print,
    .make_new = pyb_spi_make_new,
    .locals_dict = (mp_obj_t)&pyb_spi_locals_dict,
};
