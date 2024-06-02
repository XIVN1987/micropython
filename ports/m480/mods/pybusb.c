#include <stdarg.h>
#include <string.h>


#include "py/objstr.h"
#include "py/runtime.h"
#include "py/stream.h"
#include "py/mperrno.h"
#include "py/mphal.h"
#include "bufhelper.h"

#include "chip/M480.h"

#include "mods/pybusb.h"
#include "misc/usb_vcp.h"


typedef struct {
    mp_obj_base_t base;
} pyb_usb_vcp_obj_t;

static pyb_usb_vcp_obj_t pyb_usb_vcp_obj = { {&pyb_usb_vcp_type} };


/******************************************************************************/
// MicroPython bindings for USB VCP (Virtual Comm Port)

static void usb_vcp_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    mp_print_str(print, "USB_VCP()");
}


static mp_obj_t usb_vcp_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *args)
{
    mp_arg_check_num(n_args, n_kw, 0, 0, false);

    return &pyb_usb_vcp_obj;
}


static mp_obj_t usb_vcp_isconnected(mp_obj_t self_in)
{
    return mp_obj_new_bool(USBD_IS_ATTACHED());
}
static MP_DEFINE_CONST_FUN_OBJ_1(usb_vcp_isconnected_obj, usb_vcp_isconnected);


static mp_obj_t usb_vcp_any(mp_obj_t self_in)
{
    pyb_usb_vcp_obj_t *self = MP_OBJ_TO_PTR(self_in);

    if(USB_VCP_canRecv())
    {
        return mp_const_true;
    }
    else
    {
        return mp_const_false;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_1(usb_vcp_any_obj, usb_vcp_any);


static const mp_rom_map_elem_t usb_vcp_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_isconnected),  MP_ROM_PTR(&usb_vcp_isconnected_obj) },
    { MP_ROM_QSTR(MP_QSTR_any),          MP_ROM_PTR(&usb_vcp_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),         MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto),     MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline),     MP_ROM_PTR(&mp_stream_unbuffered_readline_obj)},
    { MP_ROM_QSTR(MP_QSTR_write),        MP_ROM_PTR(&mp_stream_write_obj) },
};
static MP_DEFINE_CONST_DICT(usb_vcp_locals_dict, usb_vcp_locals_dict_table);


static uint usb_vcp_read(mp_obj_t self_in, void *buf, uint size, int *errcode)
{
    pyb_usb_vcp_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int ret = USB_VCP_recv((byte*)buf, size, 1);
    if(ret == 0)
    {
        // return EAGAIN error to indicate non-blocking
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }
    return ret;
}


static uint usb_vcp_write(mp_obj_t self_in, const void *buf, uint size, int *errcode)
{
    pyb_usb_vcp_obj_t *self = MP_OBJ_TO_PTR(self_in);

    int ret = USB_VCP_send((uint8_t *)buf, size, 1);
    if(ret == 0)
    {
        // return EAGAIN error to indicate non-blocking
        *errcode = MP_EAGAIN;
        return MP_STREAM_ERROR;
    }
    return ret;
}


static uint usb_vcp_ioctl(mp_obj_t self_in, uint request, uint arg, int *errcode)
{
    pyb_usb_vcp_obj_t *self = MP_OBJ_TO_PTR(self_in);

    uint ret = 0;
    if(request == MP_STREAM_POLL)
    {
        uint flags = arg;
        if((flags & MP_STREAM_POLL_RD) && USB_VCP_canRecv())
        {
            ret |= MP_STREAM_POLL_RD;
        }
        if((flags & MP_STREAM_POLL_WR) && USB_VCP_canSend())
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


static const mp_stream_p_t usb_vcp_stream_p = {
    .read  = usb_vcp_read,
    .write = usb_vcp_write,
    .ioctl = usb_vcp_ioctl,
};


const mp_obj_type_t pyb_usb_vcp_type = {
    { &mp_type_type },
    .name = MP_QSTR_USB_VCP,
    .print = usb_vcp_print,
    .make_new = usb_vcp_make_new,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &usb_vcp_stream_p,
    .locals_dict = (mp_obj_dict_t*)&usb_vcp_locals_dict,
};
