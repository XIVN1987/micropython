#include <stdio.h>
#include <string.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mperrno.h"

#include "mods/pybpin.h"
#include "mods/pybi2c.h"

#include "misc/bufhelper.h"


/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef enum {
    PYB_I2C_0  =  0,
    PYB_I2C_1  =  1,
    PYB_NUM_I2CS
} pyb_i2c_id_t;


typedef struct _pyb_i2c_obj_t {
    mp_obj_base_t base;
    pyb_i2c_id_t i2c_id;
    I2C_TypeDef *I2Cx;
    uint baudrate;
} pyb_i2c_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_i2c_obj_t pyb_i2c_obj[PYB_NUM_I2CS] = {
    { {&pyb_i2c_type}, .i2c_id = PYB_I2C_0, .I2Cx = I2C0 },
    { {&pyb_i2c_type}, .i2c_id = PYB_I2C_1, .I2Cx = I2C1 },
};


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
STATIC bool i2c_is_online(pyb_i2c_obj_t *self, byte addr)
{
    bool ack = I2C_Start(self->I2Cx, (addr << 1) | 0);

    I2C_Stop(self->I2Cx);

    for(uint i = 0; i < 1000; i++) __NOP();   // 没有这个延时的话只有第一个波形能发出

    return ack;
}


STATIC void _i2c_readfrom_into(pyb_i2c_obj_t *self, uint addr, vstr_t *vstr)
{
    bool ack = I2C_Start(self->I2Cx, (addr << 1) | 1);
    if(!ack)
    {
        I2C_Stop(self->I2Cx);
        mp_raise_msg(&mp_type_Exception, "NACK received");
    }

    uint i;
    for(i = 0; i < vstr->len - 1; i++)
    {
        vstr->buf[i] = I2C_Read(self->I2Cx, 1);
    }
    vstr->buf[i] = I2C_Read(self->I2Cx, 0);

    I2C_Stop(self->I2Cx);
}


STATIC void _i2c_mem_readfrom_into(pyb_i2c_obj_t *self, uint addr, uint memaddr, uint memaddr_len, vstr_t *vstr)
{
    bool ack = I2C_Start(self->I2Cx, (addr << 1) | 0);
    if(!ack)
    {
        I2C_Stop(self->I2Cx);
        mp_raise_msg(&mp_type_Exception, "NACK received");
    }

    while(memaddr_len--)
    {
        ack = I2C_Write(self->I2Cx, memaddr >> (memaddr_len * 8));
        if(!ack)
        {
            I2C_Stop(self->I2Cx);
            mp_raise_msg(&mp_type_Exception, "NACK received");
        }
    }

    ack = I2C_Start(self->I2Cx, (addr << 1) | 1);
    if(!ack)
    {
        I2C_Stop(self->I2Cx);
        mp_raise_msg(&mp_type_Exception, "NACK received");
    }

    uint i;
    for(i = 0; i < vstr->len - 1; i++)
    {
        vstr->buf[i] = I2C_Read(self->I2Cx, 1);
    }
    vstr->buf[i] = I2C_Read(self->I2Cx, 0);

    I2C_Stop(self->I2Cx);
}


/******************************************************************************/
/* MicroPython bindings                                                      */
/******************************************************************************/
STATIC void i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_i2c_obj_t *self = self_in;

    mp_printf(print, "I2C(%d, baudrate=%u)", self->i2c_id, self->baudrate);
}


STATIC mp_obj_t i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_id, ARG_baudrate, ARG_scl, ARG_sda };
    STATIC const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 0} },
        { MP_QSTR_baudrate, MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 10000} },
        { MP_QSTR_scl,      MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
        { MP_QSTR_sda,      MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
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

    pin_obj_t *pin_scl = pin_find_by_name(args[ARG_scl].u_obj);
    if((pin_scl->pbit % 2) == 1)
        mp_raise_ValueError("I2C SCL need be Even number pin, like PA0, PA2, PA4");

    pin_obj_t *pin_sda = pin_find_by_name(args[ARG_sda].u_obj);
    if((pin_sda->pbit % 2) == 0)
        mp_raise_ValueError("I2C SDA need be Odd  number pin, like PA1, PA3, PA5");

    switch((uint)pin_scl->port)
    {
    case (uint)GPIOA: PORT->PORTA_PULLU |= (1 << pin_scl->pbit);  break;
    case (uint)GPIOC: PORT->PORTC_PULLU |= (1 << pin_scl->pbit);  break;
    default: break;
    }

    switch((uint)pin_sda->port)
    {
    case (uint)GPIOA: PORT->PORTA_PULLU |= (1 << pin_sda->pbit);  break;
    case (uint)GPIOC: PORT->PORTC_PULLU |= (1 << pin_sda->pbit);  break;
    default: break;
    }

    switch(self->i2c_id)
    {
    case PYB_I2C_0:
        pin_config(pin_scl, FUNMUX0_I2C0_SCL, 0, 0);
        pin_config(pin_sda, FUNMUX1_I2C0_SDA, 0, 0);
        break;

    case PYB_I2C_1:
        pin_config(pin_scl, FUNMUX0_I2C1_SCL, 0, 0);
        pin_config(pin_sda, FUNMUX1_I2C1_SDA, 0, 0);
        break;

    default:
        break;
    }

    I2C_InitStructure I2C_initStruct;
    I2C_initStruct.Master = 1;
    I2C_initStruct.Addr7b = 1;
    I2C_initStruct.MstClk = self->baudrate;
    I2C_initStruct.MstIEn = 0;
    I2C_Init(self->I2Cx, &I2C_initStruct);
    I2C_Open(self->I2Cx);

    return self;
}


STATIC mp_obj_t i2c_baudrate(size_t n_args, const mp_obj_t *args)
{
    pyb_i2c_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->baudrate);
    }
    else                // set
    {
        self->baudrate = mp_obj_get_int(args[1]);

        self->I2Cx->CLKDIV = SystemCoreClock/5/self->baudrate;

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(i2c_baudrate_obj, 1, 2, i2c_baudrate);


STATIC mp_obj_t i2c_scan(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
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
    for(uint addr = start; addr <= end; addr++)
    {
        if(i2c_is_online(self, addr))
        {
            mp_obj_list_append(list, mp_obj_new_int(addr));
        }
    }
    return list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(i2c_scan_obj, 1, i2c_scan);


STATIC mp_obj_t i2c_writeto(mp_obj_t self_in, mp_obj_t addr_in, mp_obj_t data)
{
    pyb_i2c_obj_t *self = self_in;

    uint addr = mp_obj_get_int(addr_in);

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t tmp[1];
    pyb_buf_get_for_send(data, &bufinfo, tmp);

    bool ack = I2C_Start(self->I2Cx, (addr << 1) | 0);
    if(!ack)
    {
        I2C_Stop(self->I2Cx);
        mp_raise_msg(&mp_type_Exception, "NACK received");
    }

    for(uint i = 0; i < bufinfo.len; i++)
    {
        ack = I2C_Write(self->I2Cx, ((uint8_t *)bufinfo.buf)[i]);
        if(!ack)
        {
            I2C_Stop(self->I2Cx);
            mp_raise_msg(&mp_type_Exception, "NACK received");
        }
    }

    I2C_Stop(self->I2Cx);

    return mp_obj_new_int(bufinfo.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(i2c_writeto_obj, i2c_writeto);


STATIC mp_obj_t i2c_readfrom(mp_obj_t self_in, mp_obj_t addr_in, mp_obj_t len)
{
    pyb_i2c_obj_t *self = self_in;

    uint addr = mp_obj_get_int(addr_in);

    vstr_t vstr;
    pyb_buf_get_for_recv(len, &vstr);

    _i2c_readfrom_into(self, addr, &vstr);

    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(i2c_readfrom_obj, i2c_readfrom);


STATIC mp_obj_t i2c_readfrom_into(mp_obj_t self_in, mp_obj_t addr_in, mp_obj_t buf)
{
    pyb_i2c_obj_t *self = self_in;

    uint addr = mp_obj_get_int(addr_in);

    vstr_t vstr;
    pyb_buf_get_for_recv(buf, &vstr);

    _i2c_readfrom_into(self, addr, &vstr);

    return mp_obj_new_int(vstr.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(i2c_readfrom_into_obj, i2c_readfrom_into);


STATIC mp_obj_t i2c_mem_writeto(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
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

    bool ack = I2C_Start(self->I2Cx, (addr << 1) | 0);
    if(!ack)
    {
        I2C_Stop(self->I2Cx);
        mp_raise_msg(&mp_type_Exception, "NACK received");
    }

    while(memaddr_len--)
    {
        ack = I2C_Write(self->I2Cx, memaddr >> (memaddr_len * 8));
        if(!ack)
        {
            I2C_Stop(self->I2Cx);
            mp_raise_msg(&mp_type_Exception, "NACK received");
        }
    }

    for(uint i = 0; i < bufinfo.len; i++)
    {
        ack = I2C_Write(self->I2Cx, ((uint8_t *)bufinfo.buf)[i]);
        if(!ack)
        {
            I2C_Stop(self->I2Cx);
            mp_raise_msg(&mp_type_Exception, "NACK received");
        }
    }

    I2C_Stop(self->I2Cx);

    return mp_obj_new_int(bufinfo.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(i2c_mem_writeto_obj, 4, i2c_mem_writeto);


STATIC mp_obj_t i2c_mem_readfrom(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr,        MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_memaddr,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_len,         MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = 0} },
        { MP_QSTR_memaddr_len, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 1} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(args), allowed_args, args);

    pyb_i2c_obj_t *self = pos_args[0];

    uint addr        = args[0].u_int;
    uint memaddr     = args[1].u_int;
    uint memaddr_len = args[3].u_int;
    if((memaddr_len < 1) || (memaddr_len > 3)) {
        mp_raise_ValueError("addrsize invalid");
    }

    vstr_t vstr;
    pyb_buf_get_for_recv(args[2].u_obj, &vstr);

    _i2c_mem_readfrom_into(self, addr, memaddr, memaddr_len, &vstr);

    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(i2c_mem_readfrom_obj, 4, i2c_mem_readfrom);


STATIC mp_obj_t i2c_mem_readfrom_into(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_addr,        MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_memaddr,     MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_buf,         MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = 0} },
        { MP_QSTR_memaddr_len, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 1} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(args), allowed_args, args);

    pyb_i2c_obj_t *self = pos_args[0];

    uint addr        = args[0].u_int;
    uint memaddr     = args[1].u_int;
    uint memaddr_len = args[3].u_int;
    if((memaddr_len < 1) || (memaddr_len > 3)) {
        mp_raise_ValueError("addrsize invalid");
    }

    vstr_t vstr;
    pyb_buf_get_for_recv(args[2].u_obj, &vstr);

    _i2c_mem_readfrom_into(self, addr, memaddr, memaddr_len, &vstr);

    return mp_obj_new_int(vstr.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(i2c_mem_readfrom_into_obj, 4, i2c_mem_readfrom_into);


STATIC const mp_rom_map_elem_t i2c_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_baudrate),           MP_ROM_PTR(&i2c_baudrate_obj) },
    { MP_ROM_QSTR(MP_QSTR_scan),               MP_ROM_PTR(&i2c_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_writeto),            MP_ROM_PTR(&i2c_writeto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom),           MP_ROM_PTR(&i2c_readfrom_obj) },
    { MP_ROM_QSTR(MP_QSTR_readfrom_into),      MP_ROM_PTR(&i2c_readfrom_into_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem_writeto),        MP_ROM_PTR(&i2c_mem_writeto_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem_readfrom),       MP_ROM_PTR(&i2c_mem_readfrom_obj) },
    { MP_ROM_QSTR(MP_QSTR_mem_readfrom_into),  MP_ROM_PTR(&i2c_mem_readfrom_into_obj) },
};
STATIC MP_DEFINE_CONST_DICT(i2c_locals_dict, i2c_locals_dict_table);


const mp_obj_type_t pyb_i2c_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2C,
    .print = i2c_print,
    .make_new = i2c_make_new,
    .locals_dict = (mp_obj_t)&i2c_locals_dict,
};
