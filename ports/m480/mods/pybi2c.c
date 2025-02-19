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

/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef enum {
    PYB_I2C_0   =  0,
    PYB_I2C_1   =  1,
    PYB_I2C_2   =  2,
    PYB_NUM_I2CS
} pyb_i2c_id_t;


typedef struct {
    mp_obj_base_t base;
    pyb_i2c_id_t i2c_id;
    I2C_T *I2Cx;
    uint baudrate;
    uint8_t slave;
} pyb_i2c_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
static pyb_i2c_obj_t pyb_i2c_obj[PYB_NUM_I2CS] = {
    { {&pyb_i2c_type}, .i2c_id = PYB_I2C_0, .I2Cx = I2C0 },
    { {&pyb_i2c_type}, .i2c_id = PYB_I2C_1, .I2Cx = I2C1 },
    { {&pyb_i2c_type}, .i2c_id = PYB_I2C_2, .I2Cx = I2C2 },
};


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
static bool i2c_is_online(pyb_i2c_obj_t *self, byte addr)
{
    I2C_START(self->I2Cx);
    while(1)
    {
        I2C_WAIT_READY(self->I2Cx) __NOP();
        switch(I2C_GET_STATUS(self->I2Cx))
        {
        case 0x08u:
            I2C_SET_DATA(self->I2Cx, (addr << 1) | 0);          // Write SLA+W to Register I2CDAT
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
static void i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_i2c_obj_t *self = self_in;

    mp_printf(print, "I2C(%d, baudrate=%u)", self->i2c_id, self->baudrate);
}


static mp_obj_t i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_id, ARG_baudrate, ARG_slave, ARG_slave_addr, ARG_scl, ARG_sda, ARG_pullup };
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,         MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 1} },
        { MP_QSTR_baudrate,   MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 10000} },
        { MP_QSTR_slave,      MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool= false} },
        { MP_QSTR_slave_addr, MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },    // 0x15、[0x15, 0x25]、[(0x15, 0x3F), (0x25, 0x47)]、[0x15, (0x25, 0x47)]
        { MP_QSTR_scl,        MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
        { MP_QSTR_sda,        MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
        { MP_QSTR_pullup,     MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool= true} }, // 'false' used for 5V device with external pull-up resistor
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint i2c_id = args[ARG_id].u_int;
    if(i2c_id >= PYB_NUM_I2CS)
    {
        mp_raise_OSError(MP_ENODEV);
    }
    pyb_i2c_obj_t *self = &pyb_i2c_obj[i2c_id];

    self->baudrate = args[ARG_baudrate].u_int;
    if(self->baudrate > 1000000)
    {
        mp_raise_ValueError("invalid baudrate value");
    }

    self->slave = args[ARG_slave].u_bool;
    
    if(args[ARG_scl].u_obj != mp_const_none)
    {
        if(args[ARG_pullup].u_bool)
            pin_config_pull(args[ARG_scl].u_obj, GPIO_PUSEL_PULL_UP);
        pin_config_by_func(args[ARG_scl].u_obj, "%s_I2C%d_SCL", self->i2c_id);
    }

    if(args[ARG_sda].u_obj != mp_const_none)
    {
        if(args[ARG_pullup].u_bool)
            pin_config_pull(args[ARG_sda].u_obj, GPIO_PUSEL_PULL_UP);
        pin_config_by_func(args[ARG_sda].u_obj, "%s_I2C%d_SDA", self->i2c_id);
    }

    switch(self->i2c_id)
    {
    case PYB_I2C_0: CLK_EnableModuleClock(I2C0_MODULE); break;
    case PYB_I2C_1: CLK_EnableModuleClock(I2C1_MODULE); break;
    case PYB_I2C_2: CLK_EnableModuleClock(I2C2_MODULE); break;
    default: break;
    }

    I2C_Open(self->I2Cx, self->baudrate);

    if(self->slave)
    {
        uint slave_addr = mp_obj_get_int(args[ARG_slave_addr].u_obj);
        I2C_SetSlaveAddr(self->I2Cx, 0, slave_addr, I2C_GCMODE_DISABLE);
    }

    return self;
}


static mp_obj_t i2c_baudrate(size_t n_args, const mp_obj_t *args)
{
    pyb_i2c_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->baudrate);
    }
    else                // set
    {
        self->baudrate = mp_obj_get_int(args[1]);

        I2C_SetBusClockFreq(self->I2Cx, self->baudrate);

        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(i2c_baudrate_obj, 1, 2, i2c_baudrate);


static mp_obj_t i2c_scan(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_start, MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0x10} },
        { MP_QSTR_end,   MP_ARG_KW_ONLY | MP_ARG_INT, {.u_int = 0x7F} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    pyb_i2c_obj_t *self = pos_args[0];

    uint start = args[0].u_int;
    uint end   = args[1].u_int;

    mp_obj_t list = mp_obj_new_list(0, NULL);
    for(uint addr = start; addr < end; addr++)
    {
        if(i2c_is_online(self, addr))
        {
            mp_obj_list_append(list, mp_obj_new_int(addr));
        }
    }

    return list;
}
static MP_DEFINE_CONST_FUN_OBJ_KW(i2c_scan_obj, 1, i2c_scan);


static mp_obj_t i2c_writeto(mp_obj_t self_in, mp_obj_t addr_in, mp_obj_t data)
{
    pyb_i2c_obj_t *self = self_in;

    uint addr = mp_obj_get_int(addr_in);

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t tmp[1];
    pyb_buf_get_for_send(data, &bufinfo, tmp);

    if(I2C_WriteMultiBytes(self->I2Cx, addr, bufinfo.buf, bufinfo.len) != bufinfo.len)
    {
        mp_raise_OSError(MP_EIO);
    }

    return mp_obj_new_int(bufinfo.len);     // return the number of bytes written
}
static MP_DEFINE_CONST_FUN_OBJ_3(i2c_writeto_obj, i2c_writeto);


static mp_obj_t i2c_readfrom(mp_obj_t self_in, mp_obj_t addr_in, mp_obj_t len)
{
    pyb_i2c_obj_t *self = self_in;

    uint addr = mp_obj_get_int(addr_in);

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(len, &vstr);

    if(I2C_ReadMultiBytes(self->I2Cx, addr, (uint8_t *)vstr.buf, vstr.len) != vstr.len)
    {
        mp_raise_OSError(MP_EIO);
    }

    return mp_obj_new_bytes_from_vstr(&vstr);     // return the received data
}
static MP_DEFINE_CONST_FUN_OBJ_3(i2c_readfrom_obj, i2c_readfrom);


static mp_obj_t i2c_readfrom_into(mp_obj_t self_in, mp_obj_t addr_in, mp_obj_t buf)
{
    pyb_i2c_obj_t *self = self_in;

    uint addr = mp_obj_get_int(addr_in);

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(buf, &vstr);

    if(I2C_ReadMultiBytes(self->I2Cx, addr, (uint8_t *)vstr.buf, vstr.len) != vstr.len)
    {
        mp_raise_OSError(MP_EIO);
    }

    return mp_obj_new_int(vstr.len);    // return the number of bytes received
}
static MP_DEFINE_CONST_FUN_OBJ_3(i2c_readfrom_into_obj, i2c_readfrom_into);


static mp_obj_t i2c_mem_writeto(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr,        MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_memaddr,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_data,        MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = 0} },
        { MP_QSTR_memaddr_len, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 1} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    pyb_i2c_obj_t *self = pos_args[0];

    uint addr        = args[0].u_int;
    uint memaddr     = args[1].u_int;
    uint memaddr_len = args[3].u_int;
    if((memaddr_len < 1) || (memaddr_len > 3))
    {
        mp_raise_ValueError("memaddr_len invalid");
    }

    // get the buffer to write from
    mp_buffer_info_t bufinfo;
    uint8_t tmp[1];
    pyb_buf_get_for_send(args[2].u_obj, &bufinfo, tmp);

    if(memaddr_len == 1)
    {
        if(I2C_WriteMultiBytesOneReg(self->I2Cx, addr, memaddr, bufinfo.buf, bufinfo.len) != bufinfo.len)
        {
            mp_raise_OSError(MP_EIO);
        }
    }
    else
    {
        if(I2C_WriteMultiBytesTwoRegs(self->I2Cx, addr, memaddr, bufinfo.buf, bufinfo.len) != bufinfo.len)
        {
            mp_raise_OSError(MP_EIO);
        }
    }

    return mp_obj_new_int(bufinfo.len);     // return the number of bytes written
}
static MP_DEFINE_CONST_FUN_OBJ_KW(i2c_mem_writeto_obj, 4, i2c_mem_writeto);


static mp_obj_t i2c_mem_readfrom(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr,        MP_ARG_REQUIRED  | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_memaddr,     MP_ARG_REQUIRED  | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_len,         MP_ARG_REQUIRED  | MP_ARG_OBJ, {.u_obj = 0} },
        { MP_QSTR_memaddr_len, MP_ARG_KW_ONLY   | MP_ARG_INT, {.u_int = 1} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    pyb_i2c_obj_t *self = pos_args[0];

    uint addr        = args[0].u_int;
    uint memaddr     = args[1].u_int;
    uint memaddr_len = args[3].u_int;
    if((memaddr_len < 1) || (memaddr_len > 3))
    {
        mp_raise_ValueError("memaddr_len invalid");
    }

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(args[2].u_obj, &vstr);

    if(memaddr_len == 1)
    {
        if(I2C_ReadMultiBytesOneReg(self->I2Cx, addr, memaddr, (uint8_t *)vstr.buf, vstr.len) != vstr.len)
        {
            mp_raise_OSError(MP_EIO);
        }
    }
    else
    {
        if(I2C_ReadMultiBytesTwoRegs(self->I2Cx, addr, memaddr, (uint8_t *)vstr.buf, vstr.len) != vstr.len)
        {
            mp_raise_OSError(MP_EIO);
        }
    }

    return mp_obj_new_bytes_from_vstr(&vstr);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(i2c_mem_readfrom_obj, 4, i2c_mem_readfrom);


static mp_obj_t i2c_mem_readfrom_into(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr,        MP_ARG_REQUIRED  | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_memaddr,     MP_ARG_REQUIRED  | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_buf,         MP_ARG_REQUIRED  | MP_ARG_OBJ, {.u_obj = 0} },
        { MP_QSTR_memaddr_len, MP_ARG_KW_ONLY   | MP_ARG_INT, {.u_int = 1} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    pyb_i2c_obj_t *self = pos_args[0];

    uint addr        = args[0].u_int;
    uint memaddr     = args[1].u_int;
    uint memaddr_len = args[3].u_int;
    if((memaddr_len < 1) || (memaddr_len > 3))
    {
        mp_raise_ValueError("memaddr_len invalid");
    }

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(args[2].u_obj, &vstr);

    if(memaddr_len == 1)
    {
        if(I2C_ReadMultiBytesOneReg(self->I2Cx, addr, memaddr, (uint8_t *)vstr.buf, vstr.len) != vstr.len)
        {
            mp_raise_OSError(MP_EIO);
        }
    }
    else
    {
        if(I2C_ReadMultiBytesTwoRegs(self->I2Cx, addr, memaddr, (uint8_t *)vstr.buf, vstr.len) != vstr.len)
        {
            mp_raise_OSError(MP_EIO);
        }
    }

    return MP_OBJ_NEW_SMALL_INT(vstr.len);
}
static MP_DEFINE_CONST_FUN_OBJ_KW(i2c_mem_readfrom_into_obj, 4, i2c_mem_readfrom_into);


static const mp_rom_map_elem_t i2c_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_baudrate),          MP_ROM_PTR(&i2c_baudrate_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan),              MP_ROM_PTR(&i2c_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto),           MP_ROM_PTR(&i2c_writeto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom),          MP_ROM_PTR(&i2c_readfrom_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_into),     MP_ROM_PTR(&i2c_readfrom_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem_writeto),       MP_ROM_PTR(&i2c_mem_writeto_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem_readfrom),      MP_ROM_PTR(&i2c_mem_readfrom_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem_readfrom_into), MP_ROM_PTR(&i2c_mem_readfrom_into_obj) },
};
static MP_DEFINE_CONST_DICT(i2c_locals_dict, i2c_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    pyb_i2c_type,
    MP_QSTR_I2C,
    MP_TYPE_FLAG_NONE,
    print, i2c_print,
    make_new, i2c_make_new,
    locals_dict, &i2c_locals_dict
);
