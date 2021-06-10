#include <stdint.h>
#include <string.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mperrno.h"

#include "misc/bufhelper.h"

#include "mods/pybspi.h"


/// \moduleref pyb
/// \class SPI - a master-driven serial protocol

/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef enum {
    PYB_SPI_0  =  0,
    PYB_NUM_SPIS
} pyb_spi_id_t;


typedef struct _pyb_spi_obj_t {
    mp_obj_base_t base;
    pyb_spi_id_t spi_id;
    SPI_TypeDef *SPIx;
    uint baudrate;
    byte polarity;
    byte phase;
    byte duplex;
    bool master;
    bool lsbf;
} pyb_spi_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_spi_obj_t pyb_spi_obj[PYB_NUM_SPIS] = { {.spi_id = PYB_SPI_0, .SPIx = SPI} };


/******************************************************************************/
/* MicroPython bindings                                                      */
/******************************************************************************/
STATIC void pyb_spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_spi_obj_t *self = self_in;

    if(self->baudrate > 0) {
        mp_printf(print, "SPI(%d, baudrate=%u, polarity=%u, phase=%u, duplex=%s)",
                 self->spi_id, self->baudrate, self->polarity, self->phase, self->duplex ? "full" : "half");
    } else {
        mp_printf(print, "SPI(%d)", self->spi_id);
    }
}


static const mp_arg_t pyb_spi_init_args[] = {
    { MP_QSTR_id,        MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 0} },
    { MP_QSTR_baudrate,  MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 1000000} },    // 1MHz
    { MP_QSTR_polarity,  MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = SPI_LEVEL_LOW} },
    { MP_QSTR_phase,     MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = SPI_EDGE_FIRST} },
    { MP_QSTR_duplex,    MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = SPI_DUPLEX_HALF} },
    { MP_QSTR_master,    MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool= true} },
    { MP_QSTR_lsbf,      MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool= false} }
};
STATIC mp_obj_t pyb_spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(pyb_spi_init_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(args), pyb_spi_init_args, args);

    // check the peripheral id
    uint32_t spi_id = args[0].u_int;
    if(spi_id > PYB_NUM_SPIS-1) {
        mp_raise_OSError(MP_ENODEV);
    }

    pyb_spi_obj_t *self = &pyb_spi_obj[spi_id];
    self->base.type = &pyb_spi_type;

    self->baudrate = args[1].u_int;
    uint clkdiv = SystemXTALClock / self->baudrate;

    self->polarity = args[2].u_int;
    self->phase    = args[3].u_int;

    self->duplex   = args[4].u_int;

    self->master   = args[5].u_bool;

    self->lsbf     = args[6].u_bool;

    if(!self->master)
    {
        goto invalid_args;
    }

    SPI_InitStructure SPI_initStruct;

    SPI_initStruct.Master = 1;
    SPI_initStruct.ClkDiv = clkdiv;
    SPI_initStruct.Duplex = self->duplex;
    SPI_initStruct.IdleLevel = self->polarity;
    SPI_initStruct.SampleEdge = self->phase;
    SPI_initStruct.LSBFrist = self->lsbf;
    SPI_initStruct.CompleteIE = 0;
    SPI_Init(self->SPIx, &SPI_initStruct);

    return self;

invalid_args:
    mp_raise_ValueError("invalid argument(s) value");
}


STATIC mp_obj_t pyb_spi_write(mp_obj_t self_in, mp_obj_t buf)
{
    pyb_spi_obj_t *self = self_in;

    if(self->duplex == SPI_DUPLEX_FULL)
    {
        mp_raise_msg(&mp_type_Exception, "can only used in half duplex mode");
    }

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(buf, &bufinfo, data);

    SPI_Write(self->SPIx, (uint8_t *)bufinfo.buf, bufinfo.len);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_spi_write_obj, pyb_spi_write);


STATIC mp_obj_t pyb_spi_read(mp_obj_t self_in, mp_obj_t cnt)
{
    pyb_spi_obj_t *self = self_in;

    if(self->duplex == SPI_DUPLEX_FULL)
    {
        mp_raise_msg(&mp_type_Exception, "can only used in half duplex mode");
    }

    uint32_t byte_count = mp_obj_get_int(cnt);

    SPI_Read(self->SPIx, byte_count);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_spi_read_obj, pyb_spi_read);


STATIC mp_obj_t pyb_spi_readWrite(mp_obj_t self_in, mp_obj_t buf)
{
    pyb_spi_obj_t *self = self_in;

    if(self->duplex == SPI_DUPLEX_HALF)
    {
        mp_raise_msg(&mp_type_Exception, "can only used in full duplex mode");
    }

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(buf, &bufinfo, data);

    SPI_ReadWrite(self->SPIx, (uint8_t *)bufinfo.buf, bufinfo.len);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_spi_readWrite_obj, pyb_spi_readWrite);


STATIC mp_obj_t pyb_spi_busy(mp_obj_t self_in)
{
    pyb_spi_obj_t *self = self_in;

    return mp_obj_new_bool(SPI_Busy(self->SPIx));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_spi_busy_obj, pyb_spi_busy);


STATIC mp_obj_t pyb_spi_fetchData(mp_obj_t self_in, mp_obj_t cnt)
{
    pyb_spi_obj_t *self = self_in;

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(cnt, &vstr);

    SPI_FetchData(self->SPIx, vstr.buf, vstr.len);

    return mp_obj_new_str_from_vstr(&mp_type_bytes, &vstr);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_spi_fetchData_obj, pyb_spi_fetchData);


STATIC mp_obj_t pyb_spi_fetchInto(mp_obj_t self_in, mp_obj_t buf)
{
    pyb_spi_obj_t *self = self_in;

    // get the buffer to receive into
    vstr_t vstr;
    pyb_buf_get_for_recv(buf, &vstr);

    SPI_FetchData(self->SPIx, vstr.buf, vstr.len);

    return mp_obj_new_int(vstr.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_spi_fetchInto_obj, pyb_spi_fetchInto);


STATIC const mp_rom_map_elem_t pyb_spi_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_write),               MP_ROM_PTR(&pyb_spi_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),                MP_ROM_PTR(&pyb_spi_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readWrite),           MP_ROM_PTR(&pyb_spi_readWrite_obj) },
    { MP_ROM_QSTR(MP_QSTR_busy),                MP_ROM_PTR(&pyb_spi_busy_obj) },
    { MP_ROM_QSTR(MP_QSTR_fetchData),           MP_ROM_PTR(&pyb_spi_fetchData_obj) },
    { MP_ROM_QSTR(MP_QSTR_fetchInto),           MP_ROM_PTR(&pyb_spi_fetchInto_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_LEVEL_LOW),           MP_ROM_INT(SPI_LEVEL_LOW)   },
    { MP_ROM_QSTR(MP_QSTR_LEVEL_HIGH),          MP_ROM_INT(SPI_LEVEL_HIGH)  },
    { MP_ROM_QSTR(MP_QSTR_EDGE_FIRST),          MP_ROM_INT(SPI_EDGE_FIRST)  },
    { MP_ROM_QSTR(MP_QSTR_EDGE_SECOND),         MP_ROM_INT(SPI_EDGE_SECOND) },
    { MP_ROM_QSTR(MP_QSTR_DUPLEX_HALF),         MP_ROM_INT(SPI_DUPLEX_HALF) },
    { MP_ROM_QSTR(MP_QSTR_DUPLEX_FULL),         MP_ROM_INT(SPI_DUPLEX_FULL) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_spi_locals_dict, pyb_spi_locals_dict_table);


const mp_obj_type_t pyb_spi_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPI,
    .print = pyb_spi_print,
    .make_new = pyb_spi_make_new,
    .locals_dict = (mp_obj_t)&pyb_spi_locals_dict,
};
