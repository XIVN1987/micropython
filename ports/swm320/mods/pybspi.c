#include <stdint.h>
#include <string.h>

#include "py/obj.h"
#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/binary.h"

#include "mods/pybpin.h"
#include "mods/pybspi.h"


/// \moduleref pyb
/// \class SPI - a master-driven serial protocol

/******************************************************************************
 DEFINE TYPES
 ******************************************************************************/
typedef enum {
    PYB_SPI_0   =  0,
    PYB_SPI_1   =  1,
    PYB_NUM_SPIS
} pyb_spi_id_t;

typedef struct _pyb_spi_obj_t {
    mp_obj_base_t base;
    pyb_spi_id_t spi_id;
    SPI_TypeDef *SPIx;
    uint baudrate;
    byte polarity;
    byte phase;
    bool slave;
    byte bits;
} pyb_spi_obj_t;


/******************************************************************************
 DECLARE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_spi_obj_t pyb_spi_obj[PYB_NUM_SPIS] = {
    { {&pyb_spi_type}, .spi_id = PYB_SPI_0, .SPIx = SPI0 },
    { {&pyb_spi_type}, .spi_id = PYB_SPI_1, .SPIx = SPI1 },
};


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
STATIC uint spi_transfer(pyb_spi_obj_t *self, uint data)
{
    SPI_WriteWithWait(self->SPIx, data);

    return SPI_Read(self->SPIx);;
}


/******************************************************************************/
/* MicroPython bindings                                                      */
/******************************************************************************/
STATIC void spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_spi_obj_t *self = self_in;

    mp_printf(print, "SPI(%d, baudrate=%u, polarity=%u, phase=%u, bits=%u, mode='%s')",
        self->spi_id, self->baudrate, self->polarity, self->phase, self->bits, self->slave ? "slave" : "master");
}


STATIC mp_obj_t spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_id, ARG_baudrate, ARG_polarity, ARG_phase, ARG_slave, ARG_bits, ARG_mosi, ARG_miso, ARG_sck, ARG_ss };
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,        MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 0} },
        { MP_QSTR_baudrate,  MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 1000000} },    // 1MHz
        { MP_QSTR_polarity,  MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 0} },
        { MP_QSTR_phase,     MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 0} },
        { MP_QSTR_slave,     MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool= false} },
        { MP_QSTR_bits,      MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 8} },
        { MP_QSTR_mosi,      MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
        { MP_QSTR_miso,      MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
        { MP_QSTR_sck,       MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
        { MP_QSTR_ss,        MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} }, // used for slave mode
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // check the peripheral id
    uint spi_id = args[ARG_id].u_int;
    if(spi_id >= PYB_NUM_SPIS) {
        mp_raise_OSError(MP_ENODEV);
    }
    pyb_spi_obj_t *self = &pyb_spi_obj[spi_id];

    self->baudrate = args[ARG_baudrate].u_int;
    uint clkdiv = 7;
    for(int i = 0; i < 8; i++)
    {
        if(SystemCoreClock / (1 << (i+2)) < self->baudrate)
        {
            clkdiv = i;
            break;
        }
    }

    self->polarity = args[ARG_polarity].u_int;
    self->phase    = args[ARG_phase].u_int;

    self->slave = args[ARG_slave].u_bool;

    self->bits = args[ARG_bits].u_int;
    if((self->bits < 4) || (self->bits > 16))
    {
        mp_raise_ValueError("bits invalid");
    }

    SPI_InitStructure SPI_initStruct;
    SPI_initStruct.clkDiv = clkdiv;
    SPI_initStruct.FrameFormat = SPI_FORMAT_SPI;
    SPI_initStruct.SampleEdge = self->phase;
    SPI_initStruct.IdleLevel = self->polarity;
    SPI_initStruct.WordSize = self->bits;
    SPI_initStruct.Master = (self->slave ? 0 : 1);
    SPI_initStruct.RXHFullIEn = 0;
    SPI_initStruct.TXEmptyIEn = 0;
    SPI_initStruct.TXCompleteIEn = 0;
    SPI_Init(self->SPIx, &SPI_initStruct);
    SPI_Open(self->SPIx);

    pin_obj_t *pin_mosi = pin_find_by_name(args[ARG_mosi].u_obj);
    if((pin_mosi->pbit % 2) == 1)
        mp_raise_ValueError("SPI MOSI need be Even number pin, like PA0, PA2, PA4");

    pin_obj_t *pin_miso = pin_find_by_name(args[ARG_miso].u_obj);
    if((pin_miso->pbit % 2) == 0)
        mp_raise_ValueError("SPI MISO need be Odd  number pin, like PA1, PA3, PA5");

    pin_obj_t *pin_sclk = pin_find_by_name(args[ARG_sck].u_obj);
    if((pin_sclk->pbit % 2) == 0)
        mp_raise_ValueError("SPI SCLK need be Odd  number pin, like PA1, PA3, PA5");

    if(!self->slave) goto not_salve;

     pin_obj_t *pin_ssel = pin_find_by_name(args[ARG_ss].u_obj);
    if((pin_ssel->pbit % 2) == 1)
        mp_raise_ValueError("SPI SSEL need be Even number pin, like PA0, PA2, PA4");
not_salve:

    switch(self->spi_id) {
    case PYB_SPI_0:
        pin_config(pin_mosi, FUNMUX0_SPI0_MOSI, 0, 0);
        pin_config(pin_miso, FUNMUX1_SPI0_MISO, 0, 0);
        pin_config(pin_sclk, FUNMUX1_SPI0_SCLK, 0, 0);
        if(self->slave)
            pin_config(pin_ssel, FUNMUX0_SPI0_SSEL, 0, 0);
        break;

    case PYB_SPI_1:
        pin_config(pin_mosi, FUNMUX0_SPI1_MOSI, 0, 0);
        pin_config(pin_miso, FUNMUX1_SPI1_MISO, 0, 0);
        pin_config(pin_sclk, FUNMUX1_SPI1_SCLK, 0, 0);
        if(self->slave)
            pin_config(pin_ssel, FUNMUX0_SPI1_SSEL, 0, 0);
        break;

    default:
        break;
    }

    return self;
}


STATIC mp_obj_t spi_baudrate(size_t n_args, const mp_obj_t *args)
{
    pyb_spi_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->baudrate);
    }
    else                // set
    {
        self->baudrate = mp_obj_get_int(args[1]);

        uint clkdiv = 7;
        for(int i = 0; i < 8; i++)
        {
            if(SystemCoreClock / (1 << (i+2)) < self->baudrate)
            {
                clkdiv = i;
                break;
            }
        }

        self->SPIx->CTRL &= ~SPI_CTRL_CLKDIV_Msk;
        self->SPIx->CTRL |= (clkdiv << SPI_CTRL_CLKDIV_Pos);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(spi_baudrate_obj, 1, 2, spi_baudrate);


STATIC mp_obj_t spi_polarity(size_t n_args, const mp_obj_t *args)
{
    pyb_spi_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->polarity);
    }
    else                // set
    {
        self->polarity = mp_obj_get_int(args[1]);

        if(self->polarity) self->SPIx->CTRL |=  SPI_CTRL_CPOL_Msk;
        else               self->SPIx->CTRL &= ~SPI_CTRL_CPOL_Msk;

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(spi_polarity_obj, 1, 2, spi_polarity);


STATIC mp_obj_t spi_phase(size_t n_args, const mp_obj_t *args)
{
    pyb_spi_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->phase);
    }
    else                // set
    {
        self->phase = mp_obj_get_int(args[1]);

        if(self->phase) self->SPIx->CTRL |=  SPI_CTRL_CPHA_Msk;
        else            self->SPIx->CTRL &= ~SPI_CTRL_CPHA_Msk;

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(spi_phase_obj, 1, 2, spi_phase);


STATIC mp_obj_t spi_bits(size_t n_args, const mp_obj_t *args)
{
    pyb_spi_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->bits);
    }
    else                // set
    {
        self->bits = mp_obj_get_int(args[1]);

        self->SPIx->CTRL &= ~SPI_CTRL_SIZE_Msk;
        self->SPIx->CTRL |= ((self->bits - 1) << SPI_CTRL_SIZE_Pos);

        return mp_const_none;
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(spi_bits_obj, 1, 2, spi_bits);


STATIC mp_obj_t spi_write(mp_obj_t self_in, mp_obj_t data)
{
    pyb_spi_obj_t *self = self_in;

    if(mp_obj_is_int(data))
    {
        spi_transfer(self, mp_obj_get_int(data));

        return mp_obj_new_int(1);
    }
    else
    {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(data, &bufinfo, MP_BUFFER_READ);

        for(uint i = 0; i < bufinfo.len; i++)
        {
            mp_obj_t item = mp_binary_get_val_array(bufinfo.typecode, bufinfo.buf, i);

            spi_transfer(self, mp_obj_get_int(item));
        }

        return mp_obj_new_int(bufinfo.len);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(spi_write_obj, spi_write);


STATIC mp_obj_t spi_read(mp_obj_t self_in, mp_obj_t len_in)
{
    pyb_spi_obj_t *self = self_in;

    uint len = mp_obj_get_int(len_in);

    uint val;
    if(len == 1)
    {
        val = spi_transfer(self, 0xFFFF);

        return mp_obj_new_int(val);
    }
    else if(len > 1)
    {
        void * items = m_malloc(self->bits <= 8 ? len : len*2);

        for(uint i = 0; i < len; i++)
        {
            val = spi_transfer(self, 0xFFFF);

            if(self->bits <= 8)
                ((uint8_t  *)items)[i] = val;
            else
                ((uint16_t *)items)[i] = val;
        }

        return mp_obj_new_memoryview(self->bits <= 8 ? 'B' : 'H', len, items);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(spi_read_obj, spi_read);


STATIC mp_obj_t spi_readinto(mp_obj_t self_in, mp_obj_t buf)
{
    pyb_spi_obj_t *self = self_in;

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_WRITE);

    for(uint i = 0; i < bufinfo.len; i++)
    {
        uint val = spi_transfer(self, 0xFFFF);

        mp_binary_set_val_array_from_int(bufinfo.typecode, bufinfo.buf, i, val);
    }

    return mp_obj_new_int(bufinfo.len);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(spi_readinto_obj, spi_readinto);


STATIC mp_obj_t spi_write_read(mp_obj_t self_in, mp_obj_t data)
{
    pyb_spi_obj_t *self = self_in;

    uint val;
    if(mp_obj_is_int(data))
    {
        val = spi_transfer(self, mp_obj_get_int(data));

        return mp_obj_new_int(val);
    }
    else
    {
        mp_buffer_info_t bufinfo;
        mp_get_buffer_raise(data, &bufinfo, MP_BUFFER_READ);

        void * items = m_malloc(self->bits <= 8 ? bufinfo.len : bufinfo.len*2);

        for(uint i = 0; i < bufinfo.len; i++)
        {
            mp_obj_t item = mp_binary_get_val_array(bufinfo.typecode, bufinfo.buf, i);
            val = spi_transfer(self, mp_obj_get_int(item));

            if(self->bits <= 8)
                ((uint8_t  *)items)[i] = val;
            else
                ((uint16_t *)items)[i] = val;
        }

        return mp_obj_new_memoryview(self->bits <= 8 ? 'B' : 'H', bufinfo.len, items);
    }
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(spi_write_read_obj, spi_write_read);


STATIC mp_obj_t spi_write_readinto(mp_obj_t self_in, mp_obj_t wbuf, mp_obj_t rbuf)
{
    pyb_spi_obj_t *self = self_in;

    mp_buffer_info_t wbufinfo;
    mp_get_buffer_raise(wbuf, &wbufinfo, MP_BUFFER_READ);

    mp_buffer_info_t rbufinfo;
    mp_get_buffer_raise(rbuf, &rbufinfo, MP_BUFFER_WRITE);

    if(rbufinfo.len < wbufinfo.len)
    {
        mp_raise_msg(&mp_type_Exception, "rxbuf.len < txbuf.len");
    }

    for(uint i = 0; i < wbufinfo.len; i++)
    {
        mp_obj_t item = mp_binary_get_val_array(wbufinfo.typecode, wbufinfo.buf, i);
        uint val = spi_transfer(self, mp_obj_get_int(item));

        mp_binary_set_val_array_from_int(rbufinfo.typecode, rbufinfo.buf, i, val);
    }

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_3(spi_write_readinto_obj, spi_write_readinto);


STATIC const mp_rom_map_elem_t spi_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_baudrate),            MP_ROM_PTR(&spi_baudrate_obj) },
    { MP_ROM_QSTR(MP_QSTR_polarity),            MP_ROM_PTR(&spi_polarity_obj) },
    { MP_ROM_QSTR(MP_QSTR_phase),               MP_ROM_PTR(&spi_phase_obj) },
    { MP_ROM_QSTR(MP_QSTR_bits),                MP_ROM_PTR(&spi_bits_obj) },
    { MP_ROM_QSTR(MP_QSTR_write),               MP_ROM_PTR(&spi_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),                MP_ROM_PTR(&spi_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto),            MP_ROM_PTR(&spi_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_read),          MP_ROM_PTR(&spi_write_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_readinto),      MP_ROM_PTR(&spi_write_readinto_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_IDLE_LOW),            MP_ROM_INT(SPI_LOW_LEVEL) },
    { MP_ROM_QSTR(MP_QSTR_IDLE_HIGH),           MP_ROM_INT(SPI_HIGH_LEVEL) },
    { MP_ROM_QSTR(MP_QSTR_EDGE_FIRST),          MP_ROM_INT(SPI_FIRST_EDGE) },
    { MP_ROM_QSTR(MP_QSTR_EDGE_SECOND),         MP_ROM_INT(SPI_SECOND_EDGE) },
};
STATIC MP_DEFINE_CONST_DICT(spi_locals_dict, spi_locals_dict_table);


const mp_obj_type_t pyb_spi_type = {
    { &mp_type_type },
    .name = MP_QSTR_SPI,
    .print = spi_print,
    .make_new = spi_make_new,
    .locals_dict = (mp_obj_t)&spi_locals_dict,
};
