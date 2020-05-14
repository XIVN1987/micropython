#include <stdio.h>
#include <string.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mperrno.h"

#include "misc/bufhelper.h"

#include "mods/pybi2c.h"


/// \moduleref pyb
/// \class I2C - a two-wire serial protocol

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
STATIC pyb_i2c_obj_t pyb_i2c_obj[PYB_NUM_I2CS] = { { .i2c_id = PYB_I2C_0, .I2Cx = I2C1 },
                                                   { .i2c_id = PYB_I2C_1, .I2Cx = I2C2 } };


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
STATIC bool pyb_i2c_scan_device(pyb_i2c_obj_t *self, byte devAddr)
{
    I2C_StartWrite(self->I2Cx, devAddr, (uint8_t *)0, 0);

    while(I2C_Busy(self->I2Cx)) {}

    return I2C_AddrACK(self->I2Cx);
}


/******************************************************************************/
/* MicroPython bindings                                                      */
/******************************************************************************/
STATIC void pyb_i2c_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_i2c_obj_t *self = self_in;

    mp_printf(print, "I2C(%d, baudrate=%u)", self->i2c_id, self->baudrate);
}


STATIC const mp_arg_t pyb_i2c_init_args[] = {
    { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 1} },
    { MP_QSTR_baudrate, MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 40000} },
};
STATIC mp_obj_t pyb_i2c_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_i2c_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(args), pyb_i2c_init_args, args);

    uint i2c_id = args[0].u_int;
    if(i2c_id > PYB_NUM_I2CS-1) {
        mp_raise_OSError(MP_ENODEV);
    }

    pyb_i2c_obj_t *self = &pyb_i2c_obj[i2c_id];
    self->base.type = &pyb_i2c_type;

    self->baudrate = args[1].u_int;

    I2C_InitStructure I2C_initStruct;

    I2C_initStruct.Master = 1;
    I2C_initStruct.Frequency = self->baudrate;
    I2C_Init(self->I2Cx, &I2C_initStruct);

    return self;
}


STATIC mp_obj_t pyb_i2c_scan(mp_obj_t self_in)
{
    pyb_i2c_obj_t *self = self_in;

    mp_obj_t list = mp_obj_new_list(0, NULL);
    for(uint addr = 0x08; addr <= 0x77; addr++) {
        if(pyb_i2c_scan_device(self, addr)) {
            mp_obj_list_append(list, mp_obj_new_int(addr));
        }
    }

    return list;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_i2c_scan_obj, pyb_i2c_scan);


// If there are more than 8 data in buf, this function sends the first 8 data.
// And then you can use pyb_i2c_write() sending the rest data  when TXFIFO is not full
STATIC mp_obj_t pyb_i2c_startWrite(mp_obj_t self_in, mp_obj_t addr_in, mp_obj_t buf)
{
    pyb_i2c_obj_t *self = self_in;

    uint32_t addr = mp_obj_get_int(addr_in);

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(buf, &bufinfo, data);

    I2C_StartWrite(self->I2Cx, addr, (uint8_t *)bufinfo.buf, bufinfo.len);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_i2c_startWrite_obj, pyb_i2c_startWrite);


STATIC mp_obj_t pyb_i2c_startRead(mp_obj_t self_in, mp_obj_t addr_in, mp_obj_t cnt)
{
    pyb_i2c_obj_t *self = self_in;

    uint32_t addr = mp_obj_get_int(addr_in);

    I2C_StartRead(self->I2Cx, addr, mp_obj_get_int(cnt));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(pyb_i2c_startRead_obj, pyb_i2c_startRead);


STATIC mp_obj_t pyb_i2c_txfifo_full(mp_obj_t self_in)
{
    pyb_i2c_obj_t *self = self_in;

    return mp_obj_new_bool(I2C_TXFIFO_Full(self->I2Cx));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_i2c_txfifo_full_obj, pyb_i2c_txfifo_full);


STATIC mp_obj_t pyb_i2c_write(mp_obj_t self_in, mp_obj_t data)
{
    pyb_i2c_obj_t *self = self_in;

    I2C_Write(self->I2Cx, mp_obj_get_int(data));

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_i2c_write_obj, pyb_i2c_write);


STATIC mp_obj_t pyb_i2c_rxfifo_empty(mp_obj_t self_in)
{
    pyb_i2c_obj_t *self = self_in;

    return mp_obj_new_bool(I2C_RXFIFO_Empty(self->I2Cx));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_i2c_rxfifo_empty_obj, pyb_i2c_rxfifo_empty);


STATIC mp_obj_t pyb_i2c_read(mp_obj_t self_in)
{
    pyb_i2c_obj_t *self = self_in;

    return mp_obj_new_int(I2C_Read(self->I2Cx));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_i2c_read_obj, pyb_i2c_read);


STATIC mp_obj_t pyb_i2c_addrACK(mp_obj_t self_in)
{
    pyb_i2c_obj_t *self = self_in;

    return mp_obj_new_bool(I2C_AddrACK(self->I2Cx));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_i2c_addrACK_obj, pyb_i2c_addrACK);


STATIC mp_obj_t pyb_i2c_dataACK(mp_obj_t self_in)
{
    pyb_i2c_obj_t *self = self_in;

    return mp_obj_new_bool(I2C_DataACK(self->I2Cx));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_i2c_dataACK_obj, pyb_i2c_dataACK);


STATIC mp_obj_t pyb_i2c_arbLost(mp_obj_t self_in)
{
    pyb_i2c_obj_t *self = self_in;

    return mp_obj_new_bool(I2C_ArbLost(self->I2Cx));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_i2c_arbLost_obj, pyb_i2c_arbLost);


STATIC const mp_rom_map_elem_t pyb_i2c_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_scan),          MP_ROM_PTR(&pyb_i2c_scan_obj) },
    { MP_ROM_QSTR(MP_QSTR_startWrite),    MP_ROM_PTR(&pyb_i2c_startWrite_obj) },
    { MP_ROM_QSTR(MP_QSTR_startRead),     MP_ROM_PTR(&pyb_i2c_startRead_obj) },
    { MP_ROM_QSTR(MP_QSTR_txfifo_full),   MP_ROM_PTR(&pyb_i2c_txfifo_full_obj) },
    { MP_ROM_QSTR(MP_QSTR_write),         MP_ROM_PTR(&pyb_i2c_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_rxfifo_empty),  MP_ROM_PTR(&pyb_i2c_rxfifo_empty_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),          MP_ROM_PTR(&pyb_i2c_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_addrACK),       MP_ROM_PTR(&pyb_i2c_addrACK_obj) },
    { MP_ROM_QSTR(MP_QSTR_dataACK),       MP_ROM_PTR(&pyb_i2c_dataACK_obj) },
    { MP_ROM_QSTR(MP_QSTR_arbLost),       MP_ROM_PTR(&pyb_i2c_arbLost_obj) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_i2c_locals_dict, pyb_i2c_locals_dict_table);


const mp_obj_type_t pyb_i2c_type = {
    { &mp_type_type },
    .name = MP_QSTR_I2C,
    .print = pyb_i2c_print,
    .make_new = pyb_i2c_make_new,
    .locals_dict = (mp_obj_t)&pyb_i2c_locals_dict,
};
