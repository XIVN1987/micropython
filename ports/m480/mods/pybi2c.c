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

#include <stdio.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/mphal.h"

#include "misc/bufhelper.h"

#include "chip/M480.h"

#include "mods/pybpin.h"
#include "mods/pybi2c.h"


/// \moduleref pyb
/// \class I2C - a two-wire serial protocol

typedef enum {
    PYB_I2C_0      =  0,
    PYB_I2C_1      =  1,
    PYB_I2C_2      =  2,
    PYB_NUM_I2CS
} pyb_i2c_id_t;


typedef struct _pyb_i2c_obj_t {
    mp_obj_base_t base;
    pyb_i2c_id_t i2c_id;
    I2C_T *I2Cx;
    uint baudrate;
    uint8_t slave;
} pyb_i2c_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_i2c_obj_t pyb_i2c_obj[PYB_NUM_I2CS] = {
    { .i2c_id = PYB_I2C_0, .I2Cx = I2C0 },
    { .i2c_id = PYB_I2C_1, .I2Cx = I2C1 },
    { .i2c_id = PYB_I2C_2, .I2Cx = I2C2 },
};


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
STATIC bool pyb_i2c_is_online(pyb_i2c_obj_t *self, byte addr)
{
    I2C_START(self->I2Cx);
    while(1)
    {
        I2C_WAIT_READY(self->I2Cx) __NOP();
        switch(I2C_GET_STATUS(self->I2Cx))
        {
        case 0x08u:
            I2C_SET_DATA(self->I2Cx, (addr << 1 | 0));          // Write SLA+W to Register I2CDAT
            I2C_SET_CONTROL_REG(self->I2Cx, I2C_CTL_SI);        // Clear SI
            break;

        case 0x18u:                                             // Slave Address ACK
            I2C_SET_CONTROL_REG(self->I2Cx, I2C_CTL_STO_SI);    // Clear SI and send STOP
            return true;

        case 0x20u:                                             // Slave Address NACK
            I2C_SET_CONTROL_REG(self->I2Cx, I2C_CTL_STO_SI);    // Clear SI and send STOP
            return false;

        case 0x38u:                                           /* Arbitration Lost */
        default:                                              /* Unknow status */
            I2C_SET_CONTROL_REG(self->I2Cx, I2C_CTL_STO_SI);    // Clear SI and send STOP
            return false;
        }
    }
}


/******************************************************************************/
/* MicroPython bindings                                                      */
/******************************************************************************/
STATIC void pyb_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_i2c_obj_t *self = self_in;

    mp_printf(print, "I2C(%d, baudrate=%u)", self->i2c_id, self->baudrate);
}


STATIC mp_obj_t pyb_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_id, ARG_baudrate, ARG_slave, ARG_slave_addr, ARG_scl, ARG_sda };
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,         MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 1} },
        { MP_QSTR_baudrate,   MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 10000} },
        { MP_QSTR_slave,      MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool= false} },
        { MP_QSTR_slave_addr, MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },    // 0x15、[0x15, 0x25]、[(0x15, 0x3F), (0x25, 0x47)]、[0x15, (0x25, 0x47)]
        { MP_QSTR_scl,        MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
        { MP_QSTR_sda,        MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint i2c_id = args[0].u_int;
    if(i2c_id >= PYB_NUM_I2CS) {
        mp_raise_OSError(MP_ENODEV);
    }

    pyb_i2c_obj_t *self = &pyb_i2c_obj[i2c_id];
    self->base.type = &pyb_i2c_type;

    self->baudrate = args[ARG_baudrate].u_int;
    if(self->baudrate > 1000000)
    {
        goto invalid_args;
    }

    self->slave = args[ARG_slave].u_bool;
    
    if(args[ARG_scl].u_obj != mp_const_none)
    {
        pin_config_pull(args[ARG_scl].u_obj, GPIO_PUSEL_PULL_UP);
        pin_config_by_func(args[ARG_scl].u_obj, "%s_I2C%d_SCL", self->i2c_id);
    }

    if(args[ARG_sda].u_obj != mp_const_none)
    {
        pin_config_pull(args[ARG_sda].u_obj, GPIO_PUSEL_PULL_UP);
        pin_config_by_func(args[ARG_sda].u_obj, "%s_I2C%d_SDA", self->i2c_id);
    }

    switch(self->i2c_id) {
    case PYB_I2C_0: CLK_EnableModuleClock(I2C0_MODULE); break;
    case PYB_I2C_1: CLK_EnableModuleClock(I2C1_MODULE); break;
    case PYB_I2C_2: CLK_EnableModuleClock(I2C2_MODULE); break;
    default: break;
    }

    I2C_Open(self->I2Cx, self->baudrate);

    if(self->slave)
    {
        if(mp_obj_is_integer(args[ARG_slave_addr].u_obj))
        {
            uint slave_addr = mp_obj_get_int(args[ARG_slave_addr].u_obj);
            I2C_SetSlaveAddr(self->I2Cx, 0, slave_addr, I2C_GCMODE_DISABLE);
        }
        else if(mp_obj_get_type(args[ARG_slave_addr].u_obj) == &mp_type_tuple)
        {
            //mp_obj_t item = mp_obj_t
        }
        else
        {
            goto invalid_args;
        }
    }

    return self;

invalid_args:
    mp_raise_ValueError("invalid argument(s) value");
}


STATIC mp_obj_t pyb_i2c_scan(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_start, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0x10} },
        { MP_QSTR_end,   MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0x7F} },
    };

    // parse args
    pyb_i2c_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint start = args[0].u_int;
    uint end   = args[1].u_int;

    mp_obj_t list = mp_obj_new_list(0, NULL);
    for(uint addr = start; addr < end; addr++) {
        if(pyb_i2c_is_online(self, addr)) {
            mp_obj_list_append(list, mp_obj_new_int(addr));
        }
    }

    return list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_scan_obj, 1, pyb_i2c_scan);


STATIC mp_obj_t pyb_i2c_writeto(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0x00} },
        { MP_QSTR_data, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    pyb_i2c_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint addr = args[0].u_int;

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(args[1].u_obj, &bufinfo, data);

    if(I2C_WriteMultiBytes(self->I2Cx, addr, bufinfo.buf, bufinfo.len) != bufinfo.len)
    {
        mp_raise_OSError(MP_EIO);
    }

    return mp_obj_new_int(bufinfo.len);     // return the number of bytes written
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_writeto_obj, 3, pyb_i2c_writeto);


STATIC mp_obj_t pyb_i2c_readfrom(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr,   MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0x00} },
        { MP_QSTR_nbytes, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    pyb_i2c_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint addr = args[0].u_int;

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(args[1].u_obj, &vstr);

    if(I2C_ReadMultiBytes(self->I2Cx, addr, (uint8_t *)vstr.buf, vstr.len) != vstr.len)
    {
        mp_raise_OSError(MP_EIO);
    }

    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);     // return the received data
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_readfrom_obj, 3, pyb_i2c_readfrom);


STATIC mp_obj_t pyb_i2c_readfrom_into(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0x00} },
        { MP_QSTR_buff, MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    pyb_i2c_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint addr = args[0].u_int;

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(args[1].u_obj, &vstr);

    if(I2C_ReadMultiBytes(self->I2Cx, addr, (uint8_t *)vstr.buf, vstr.len) != vstr.len)
    {
        mp_raise_OSError(MP_EIO);
    }

    return mp_obj_new_int(vstr.len);    // return the number of bytes received
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_readfrom_into_obj, 3, pyb_i2c_readfrom_into);


STATIC mp_obj_t pyb_i2c_writeto_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0x00} },
        { MP_QSTR_memaddr,  MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0x00} },
        { MP_QSTR_data,     MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_addrsize, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 1} },
    };

    // parse args
    pyb_i2c_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint addr          = args[0].u_int;
    uint mem_addr      = args[1].u_int;
    uint mem_addr_size = args[3].u_int;     // width of mem_addr (1 or 2 bytes)

    // get the buffer to write from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(args[2].u_obj, &bufinfo, data);

    if(mem_addr_size == 1)
    {
        if(I2C_WriteMultiBytesOneReg(self->I2Cx, addr, mem_addr, bufinfo.buf, bufinfo.len) != bufinfo.len)
        {
            mp_raise_OSError(MP_EIO);
        }
    }
    else
    {
        if(I2C_WriteMultiBytesTwoRegs(self->I2Cx, addr, mem_addr, bufinfo.buf, bufinfo.len) != bufinfo.len)
        {
            mp_raise_OSError(MP_EIO);
        }
    }

    return mp_obj_new_int(bufinfo.len);     // return the number of bytes written
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_writeto_mem_obj, 4, pyb_i2c_writeto_mem);


STATIC mp_obj_t pyb_i2c_readfrom_mem(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr,     MP_ARG_REQUIRED  | MP_ARG_INT, {.u_int = 0x00} },
        { MP_QSTR_memaddr,  MP_ARG_REQUIRED  | MP_ARG_INT, {.u_int = 0x00} },
        { MP_QSTR_nbytes,   MP_ARG_REQUIRED  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_addrsize, MP_ARG_KW_ONLY   | MP_ARG_INT, {.u_int = 1} },
    };

    // parse args
    pyb_i2c_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint addr          = args[0].u_int;
    uint mem_addr      = args[1].u_int;
    uint mem_addr_size = args[3].u_int;     // width of mem_addr (1 or 2 bytes)

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(args[2].u_obj, &vstr);

    if(mem_addr_size == 1)
    {
        if(I2C_ReadMultiBytesOneReg(self->I2Cx, addr, mem_addr, (uint8_t *)vstr.buf, vstr.len) != vstr.len)
        {
            mp_raise_OSError(MP_EIO);
        }
    }
    else
    {
        if(I2C_ReadMultiBytesTwoRegs(self->I2Cx, addr, mem_addr, (uint8_t *)vstr.buf, vstr.len) != vstr.len)
        {
            mp_raise_OSError(MP_EIO);
        }
    }

    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_readfrom_mem_obj, 4, pyb_i2c_readfrom_mem);


STATIC mp_obj_t pyb_i2c_readfrom_mem_into(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr,     MP_ARG_REQUIRED  | MP_ARG_INT, {.u_int = 0x00} },
        { MP_QSTR_memaddr,  MP_ARG_REQUIRED  | MP_ARG_INT, {.u_int = 0x00} },
        { MP_QSTR_buff,     MP_ARG_REQUIRED  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_addrsize, MP_ARG_KW_ONLY   | MP_ARG_INT, {.u_int = 1} },
    };

    // parse args
    pyb_i2c_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint addr          = args[0].u_int;
    uint mem_addr      = args[1].u_int;
    uint mem_addr_size = args[3].u_int;     //  width of mem_addr (1 or 2 bytes)

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(args[2].u_obj, &vstr);

    if(mem_addr_size == 1)
    {
        if(I2C_ReadMultiBytesOneReg(self->I2Cx, addr, mem_addr, (uint8_t *)vstr.buf, vstr.len) != vstr.len)
        {
            mp_raise_OSError(MP_EIO);
        }
    }
    else
    {
        if(I2C_ReadMultiBytesTwoRegs(self->I2Cx, addr, mem_addr, (uint8_t *)vstr.buf, vstr.len) != vstr.len)
        {
            mp_raise_OSError(MP_EIO);
        }
    }

    return MP_OBJ_NEW_SMALL_INT(vstr.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_i2c_readfrom_mem_into_obj, 4, pyb_i2c_readfrom_mem_into);


STATIC const mp_rom_map_elem_t pyb_i2c_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_scan),                MP_ROM_PTR(&pyb_i2c_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto),             MP_ROM_PTR(&pyb_i2c_writeto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom),            MP_ROM_PTR(&pyb_i2c_readfrom_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_into),       MP_ROM_PTR(&pyb_i2c_readfrom_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto_mem),         MP_ROM_PTR(&pyb_i2c_writeto_mem_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem),        MP_ROM_PTR(&pyb_i2c_readfrom_mem_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_mem_into),   MP_ROM_PTR(&pyb_i2c_readfrom_mem_into_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_i2c_locals_dict, pyb_i2c_locals_dict_table);


const mp_obj_type_t pyb_i2c_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2C,
    .print = pyb_i2c_print,
    .make_new = pyb_i2c_make_new,
    .locals_dict = (mp_obj_t)&pyb_i2c_locals_dict,
};
