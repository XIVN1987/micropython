#include <stdint.h>
#include <string.h>

#include "py/runtime.h"
#include "py/mperrno.h"
#include "py/binary.h"

#include "chip/M480.h"

#include "mods/pybpin.h"
#include "mods/pybspi.h"

#include "misc/bufhelper.h"


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
static pyb_spi_obj_t pyb_spi_obj[PYB_NUM_SPIS] = {
    { {&pyb_spi_type}, .spi_id = PYB_SPI_0, .SPIx = SPI0, .baudrate = 0 },
    { {&pyb_spi_type}, .spi_id = PYB_SPI_1, .SPIx = SPI1, .baudrate = 0 },
    { {&pyb_spi_type}, .spi_id = PYB_SPI_2, .SPIx = SPI2, .baudrate = 0 },
    { {&pyb_spi_type}, .spi_id = PYB_SPI_3, .SPIx = SPI3, .baudrate = 0 },
};


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
static uint spi_transfer(pyb_spi_obj_t *self, uint data)
{
    SPI_WRITE_TX(self->SPIx, data);

    while(SPI_GET_RX_FIFO_EMPTY_FLAG(self->SPIx)) __NOP();

    return SPI_READ_RX(self->SPIx);
}

static byte array_typecode(pyb_spi_obj_t *self)
{
    if(self->bits <= 8)       return 'B';
    else if(self->bits <= 16) return 'H';
    else                      return 'L';
}


/******************************************************************************/
/* MicroPython bindings                                                      */
/******************************************************************************/
static void spi_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_spi_obj_t *self = self_in;

    mp_printf(print, "SPI(%u, baudrate=%u, polarity=%u, phase=%u, mode=%s, msbf=%s, bits=%u)",
        self->spi_id, self->baudrate, self->polarity, self->phase, self->slave ? "Slave" : "Master", self->msbf ? "True" : "False", self->bits);
}


static mp_obj_t spi_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_id, ARG_baudrate, ARG_polarity, ARG_phase, ARG_slave, ARG_msbf, ARG_bits, ARG_mosi, ARG_miso, ARG_sck, ARG_ss };
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,        MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 0} },
        { MP_QSTR_baudrate,  MP_ARG_REQUIRED | MP_ARG_INT,  {.u_int = 1000000} },    // 1MHz
        { MP_QSTR_polarity,  MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 0} },
        { MP_QSTR_phase,     MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 0} },
        { MP_QSTR_slave,     MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool= false} },
        { MP_QSTR_msbf,      MP_ARG_KW_ONLY  | MP_ARG_BOOL, {.u_bool= true} },
        { MP_QSTR_bits,      MP_ARG_KW_ONLY  | MP_ARG_INT,  {.u_int = 8} },
        { MP_QSTR_mosi,      MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
        { MP_QSTR_miso,      MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
        { MP_QSTR_sck,       MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
        { MP_QSTR_ss,        MP_ARG_KW_ONLY  | MP_ARG_OBJ,  {.u_obj = mp_const_none} },
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint spi_id = args[ARG_id].u_int;
    if(spi_id >= PYB_NUM_SPIS) {
        mp_raise_OSError(MP_ENODEV);
    }
    pyb_spi_obj_t *self = &pyb_spi_obj[spi_id];

    self->baudrate = args[ARG_baudrate].u_int;
    self->polarity = args[ARG_polarity].u_int;
    self->phase    = args[ARG_phase].u_int;
    self->slave    = args[ARG_slave].u_bool;
    self->msbf     = args[ARG_msbf].u_bool;
    self->bits     = args[ARG_bits].u_int;
    if((self->bits < 8) || (self->bits > 32))
    {
        mp_raise_ValueError("invalid bits value");
    }

    if(args[ARG_mosi].u_obj != mp_const_none)
    {
        pin_config_by_func(args[ARG_mosi].u_obj, "%s_SPI%d_MOSI", self->spi_id);
    }

    if(args[ARG_miso].u_obj != mp_const_none)
    {
        pin_config_by_func(args[ARG_miso].u_obj, "%s_SPI%d_MISO", self->spi_id);
    }

    if(args[ARG_sck].u_obj != mp_const_none)
    {
        pin_config_by_func(args[ARG_sck].u_obj, "%s_SPI%d_CLK", self->spi_id);
    }

    if(args[ARG_ss].u_obj != mp_const_none)
    {
        pin_config_by_func(args[ARG_ss].u_obj, "%s_SPI%d_SS", self->spi_id);
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
    switch((self->polarity << 1) | self->phase)
    {
    case 0:  mode = SPI_MODE_0; break;
    case 1:  mode = SPI_MODE_1; break;
    case 2:  mode = SPI_MODE_2; break;
    case 3:
    default: mode = SPI_MODE_3; break;
    }

    SPI_Open(self->SPIx, self->slave ? SPI_SLAVE : SPI_MASTER, mode, self->bits, self->baudrate);

    if(self->msbf) SPI_SET_MSB_FIRST(self->SPIx);
    else           SPI_SET_LSB_FIRST(self->SPIx);

    return self;
}


static mp_obj_t spi_baudrate(size_t n_args, const mp_obj_t *args)
{
    pyb_spi_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->baudrate);
    }
    else                // set
    {
        self->baudrate = mp_obj_get_int(args[1]);

        SPI_SetBusClock(self->SPIx, self->baudrate);

        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(spi_baudrate_obj, 1, 2, spi_baudrate);


static mp_obj_t spi_polarity(size_t n_args, const mp_obj_t *args)
{
    pyb_spi_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->polarity);
    }
    else                // set
    {
        self->polarity = mp_obj_get_int(args[1]);

        uint mode;
        switch((self->polarity << 1) | self->phase)
        {
        case 0:  mode = SPI_MODE_0; break;
        case 1:  mode = SPI_MODE_1; break;
        case 2:  mode = SPI_MODE_2; break;
        case 3:
        default: mode = SPI_MODE_3; break;
        }

        self->SPIx->CTL &= ~(SPI_CTL_CLKPOL_Msk | SPI_CTL_TXNEG_Msk | SPI_CTL_RXNEG_Msk);
        self->SPIx->CTL |= mode;

        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(spi_polarity_obj, 1, 2, spi_polarity);


static mp_obj_t spi_phase(size_t n_args, const mp_obj_t *args)
{
    pyb_spi_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->phase);
    }
    else                // set
    {
        self->phase = mp_obj_get_int(args[1]);

        uint mode;
        switch((self->polarity << 1) | self->phase)
        {
        case 0:  mode = SPI_MODE_0; break;
        case 1:  mode = SPI_MODE_1; break;
        case 2:  mode = SPI_MODE_2; break;
        case 3:
        default: mode = SPI_MODE_3; break;
        }

        self->SPIx->CTL &= ~(SPI_CTL_CLKPOL_Msk | SPI_CTL_TXNEG_Msk | SPI_CTL_RXNEG_Msk);
        self->SPIx->CTL |= mode;

        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(spi_phase_obj, 1, 2, spi_phase);


static mp_obj_t spi_msbf(size_t n_args, const mp_obj_t *args)
{
    pyb_spi_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->msbf);
    }
    else                // set
    {
        self->msbf = mp_obj_get_int(args[1]);

        if(self->msbf) SPI_SET_MSB_FIRST(self->SPIx);
        else           SPI_SET_LSB_FIRST(self->SPIx);

        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(spi_msbf_obj, 1, 2, spi_msbf);


static mp_obj_t spi_bits(size_t n_args, const mp_obj_t *args)
{
    pyb_spi_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->bits);
    }
    else                // set
    {
        self->bits = mp_obj_get_int(args[1]);

        SPI_SET_DATA_WIDTH(self->SPIx, self->bits);

        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(spi_bits_obj, 1, 2, spi_bits);


static mp_obj_t spi_write(mp_obj_t self_in, mp_obj_t data)
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
static MP_DEFINE_CONST_FUN_OBJ_2(spi_write_obj, spi_write);


static mp_obj_t spi_read(mp_obj_t self_in, mp_obj_t len_in)
{
    pyb_spi_obj_t *self = self_in;

    uint len = mp_obj_get_int(len_in);

    uint val;
    if(len == 1)
    {
        val = spi_transfer(self, 0xFFFFFFFF);

        return mp_obj_new_int(val);
    }
    else if(len > 1)
    {
        void * items = m_malloc(self->bits <= 8 ? len : len*2);

        for(uint i = 0; i < len; i++)
        {
            val = spi_transfer(self, 0xFFFFFFFF);

            if(self->bits <= 8)
                ((uint8_t  *)items)[i] = val;
            else if(self->bits <= 16)
                ((uint16_t *)items)[i] = val;
            else
                ((uint32_t *)items)[i] = val;
        }

        return mp_obj_new_memoryview(array_typecode(self), len, items);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_2(spi_read_obj, spi_read);


static mp_obj_t spi_readinto(mp_obj_t self_in, mp_obj_t buf)
{
    pyb_spi_obj_t *self = self_in;

    mp_buffer_info_t bufinfo;
    mp_get_buffer_raise(buf, &bufinfo, MP_BUFFER_WRITE);

    for(uint i = 0; i < bufinfo.len; i++)
    {
        uint val = spi_transfer(self, 0xFFFFFFFF);

        mp_binary_set_val_array_from_int(bufinfo.typecode, bufinfo.buf, i, val);
    }

    return mp_obj_new_int(bufinfo.len);
}
static MP_DEFINE_CONST_FUN_OBJ_2(spi_readinto_obj, spi_readinto);


static mp_obj_t spi_write_read(mp_obj_t self_in, mp_obj_t data)
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
            else if(self->bits <= 16)
                ((uint16_t *)items)[i] = val;
            else
                ((uint32_t *)items)[i] = val;
        }

        return mp_obj_new_memoryview(array_typecode(self), bufinfo.len, items);
    }
}
static MP_DEFINE_CONST_FUN_OBJ_2(spi_write_read_obj, spi_write_read);


static mp_obj_t spi_write_readinto(mp_obj_t self_in, mp_obj_t wbuf, mp_obj_t rbuf)
{
    pyb_spi_obj_t *self = self_in;

    mp_buffer_info_t wbufinfo;
    mp_get_buffer_raise(wbuf, &wbufinfo, MP_BUFFER_READ);

    mp_buffer_info_t rbufinfo;
    mp_get_buffer_raise(rbuf, &rbufinfo, MP_BUFFER_WRITE);

    if(rbufinfo.len < wbufinfo.len)
    {
        mp_raise_msg(&mp_type_Exception, "rbuf.len < wbuf.len");
    }

    for(uint i = 0; i < wbufinfo.len; i++)
    {
        mp_obj_t item = mp_binary_get_val_array(wbufinfo.typecode, wbufinfo.buf, i);

        uint val = spi_transfer(self, mp_obj_get_int(item));

        mp_binary_set_val_array_from_int(rbufinfo.typecode, rbufinfo.buf, i, val);
    }

    return mp_const_none;
}
static MP_DEFINE_CONST_FUN_OBJ_3(spi_write_readinto_obj, spi_write_readinto);


static const mp_rom_map_elem_t spi_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_baudrate),            MP_ROM_PTR(&spi_baudrate_obj) },
    { MP_ROM_QSTR(MP_QSTR_polarity),            MP_ROM_PTR(&spi_polarity_obj) },
    { MP_ROM_QSTR(MP_QSTR_phase),               MP_ROM_PTR(&spi_phase_obj) },
    { MP_ROM_QSTR(MP_QSTR_msbf),                MP_ROM_PTR(&spi_msbf_obj) },
    { MP_ROM_QSTR(MP_QSTR_bits),                MP_ROM_PTR(&spi_bits_obj) },
    { MP_ROM_QSTR(MP_QSTR_write),               MP_ROM_PTR(&spi_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),                MP_ROM_PTR(&spi_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto),            MP_ROM_PTR(&spi_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_read),          MP_ROM_PTR(&spi_write_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_write_readinto),      MP_ROM_PTR(&spi_write_readinto_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_IDLE_LOW),            MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_IDLE_HIGH),           MP_ROM_INT(1) },
    { MP_ROM_QSTR(MP_QSTR_EDGE_FIRST),          MP_ROM_INT(0) },
    { MP_ROM_QSTR(MP_QSTR_EDGE_SECOND),         MP_ROM_INT(1) },
};
static MP_DEFINE_CONST_DICT(spi_locals_dict, spi_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    pyb_spi_type,
    MP_QSTR_SPI,
    MP_TYPE_FLAG_NONE,
    print, spi_print,
    make_new, spi_make_new,
    locals_dict, &spi_locals_dict
);
