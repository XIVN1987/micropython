/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
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
#include <stdio.h>
#include <string.h>

#include "py/objtuple.h"
#include "py/objarray.h"
#include "py/runtime.h"
#include "py/gc.h"
#include "py/binary.h"
#include "py/stream.h"
#include "py/mphal.h"

#include "misc/bufhelper.h"

#include "mods/pybpin.h"
#include "mods/pybcan.h"


enum {
    CAN_STATE_ERROR_ACTIVE  = 0,
    CAN_STATE_ERROR_WARNING = 1,
    CAN_STATE_ERROR_PASSIVE = 2,
    CAN_STATE_BUS_OFF       = 4,
    CAN_STATE_STOPPED       = 8,
};


/******************************************************************************
 DEFINE PRIVATE TYPES
 ******************************************************************************/
typedef enum {
    PYB_CAN_0   = 0,
    PYB_CAN_1   = 1,
    PYB_NUM_CANS
} pyb_can_id_t;


typedef struct _pyb_can_obj_t {
    mp_obj_base_t base;
    mp_uint_t can_id;
    CAN_T *CANx;
    uint32_t baudrate;
    bool     extframe;
    uint8_t  mode;

    uint8_t  IRQn;
    uint8_t  irq_flags;
    uint8_t  irq_trigger;
    uint8_t  irq_priority;   // 中断优先级
    mp_obj_t irq_callback;   // 中断处理函数
} pyb_can_obj_t;


/******************************************************************************
 DEFINE PRIVATE DATA
 ******************************************************************************/
STATIC pyb_can_obj_t pyb_can_obj[PYB_NUM_CANS] = {
    { .can_id = PYB_CAN_0, .CANx = CAN0 },
    { .can_id = PYB_CAN_1, .CANx = CAN1 },
};


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/

STATIC bool get_free_message_obj(CAN_T *tCAN, uint32_t timeout, uint32_t *pu32MessageObj) {
    uint32_t start = mp_hal_ticks_ms();
    for(;;) {
        for(uint i = 0; i < 32; i ++) {
            if((i < 16ul ? tCAN->MVLD1 & (1ul << i) : tCAN->MVLD2 & (1ul << (i - 16ul))) == 0) {
				*pu32MessageObj = i;
				return true;
			}
		}

        if(mp_hal_ticks_ms() - start >= timeout) {
            return false; // timeout
        }

        MICROPY_EVENT_POLL_HOOK
    }
}


STATIC bool can_receive_with_timeout(CAN_T *tCAN, uint32_t u32MsgNum, STR_CANMSG_T* pCanMsg, uint32_t timeout) {
    uint32_t start = mp_hal_ticks_ms();

    for(;;) {
		if(CAN_Receive(tCAN, u32MsgNum, pCanMsg))
			return true;

        if(mp_hal_ticks_ms() - start >= timeout) {
            return false; // timeout
        }

        MICROPY_EVENT_POLL_HOOK
    }
}


STATIC void can_handle_irq_callback(pyb_can_obj_t *self, uint32_t cb_reason, uint32_t fifo) {
        if(self->irq_callback != mp_const_none) {
#if  MICROPY_PY_THREAD
            mp_sched_lock();
#else
            // When executing code within a handler we must lock the GC to prevent
            // any memory allocations.  We must also catch any exceptions.
            gc_lock();
#endif
            nlr_buf_t nlr;
            if (nlr_push(&nlr) == 0) {
                mp_obj_t args[3];
                args[0] = self;
                args[1] = MP_OBJ_NEW_SMALL_INT(cb_reason);
                args[2] = MP_OBJ_NEW_SMALL_INT(fifo);
                mp_call_function_n_kw(self->irq_callback, 3, 0, args);

                nlr_pop();
            } else {
                // Uncaught exception; disable the callback so it doesn't run again.
                self->irq_callback = mp_const_none;
                CAN_DisableInt(self->CANx, CAN_CON_IE_Msk);
                mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
            }
#if  MICROPY_PY_THREAD
            mp_sched_unlock();
#else
            gc_unlock();
#endif
        }
}

void CANx_IRQHandler(pyb_can_obj_t *self, uint32_t u32IIDRStatus)
{
    uint32_t u32CBReason = 0;

    if(u32IIDRStatus == 0x00008000){
        uint32_t u32Status;
        u32Status = CAN_GET_INT_STATUS(self->CANx);

        if(u32Status & CAN_STATUS_RXOK_Msk){
            (self->CANx)->STATUS &= ~CAN_STATUS_RXOK_Msk;   /* Clear Rx Ok status*/
        }

        if(u32Status & CAN_STATUS_TXOK_Msk){
            (self->CANx)->STATUS &= ~CAN_STATUS_TXOK_Msk;    /* Clear Tx Ok status*/
        }

        if(u32Status & CAN_STATUS_EPASS_Msk){
            u32CBReason |= CAN_STATE_ERROR_PASSIVE;
        }

        if(u32Status & CAN_STATUS_EWARN_Msk){
            u32CBReason |= CAN_STATE_ERROR_WARNING;
        }

        if(u32Status & CAN_STATUS_BOFF_Msk){
            u32CBReason |= CAN_STATE_BUS_OFF;
        }

        if(u32CBReason){
            can_handle_irq_callback(self, u32CBReason, 0);
        }

    }
    else if(u32IIDRStatus != 0){

        can_handle_irq_callback(self, u32CBReason, (u32IIDRStatus - 1));
        CAN_CLR_INT_PENDING_BIT(self->CANx, (u32IIDRStatus -1));      /* Clear Interrupt Pending */
    }
}

void CAN0_IRQHandler(void)
{
    uint u32IIDRStatus = CAN0->IIDR;

    CANx_IRQHandler(&pyb_can_obj[0], u32IIDRStatus);

    if(u32IIDRStatus <= 0x20) CAN_CLR_INT_PENDING_BIT(CAN0, ((CAN0->IIDR) -1));
}

void CAN1_IRQHandler(void)
{
    uint u32IIDRStatus = CAN1->IIDR;

    CANx_IRQHandler(&pyb_can_obj[1], u32IIDRStatus);

    if(u32IIDRStatus <= 0x20) CAN_CLR_INT_PENDING_BIT(CAN1, ((CAN1->IIDR) -1));
}



/******************************************************************************/
// MicroPython bindings

STATIC void pyb_can_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind) {
    pyb_can_obj_t *self = self_in;

    qstr mode;
    switch(self->mode) {
        case CAN_NORMAL_MODE:       mode = MP_QSTR_NORMAL;          break;
        case CAN_TEST_LBACK_Msk:    mode = MP_QSTR_LOOPBACK;        break;
        case CAN_TEST_SILENT_Msk:   mode = MP_QSTR_SILENT;          break;
        case (CAN_TEST_LBACK_Msk | CAN_TEST_SILENT_Msk):
        default:                    mode = MP_QSTR_SILENT_LOOPBACK; break;
    }

    mp_printf(print, "CAN(%u, %u, extframe=%q, mode=%q)", self->can_id, self->baudrate, self->extframe ? MP_QSTR_True : MP_QSTR_False, mode);
}


STATIC mp_obj_t pyb_can_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args) {
    enum { ARG_id, ARG_baudrate, ARG_extframe, ARG_mode, ARG_irq, ARG_callback, ARG_priority, ARG_rxd, ARG_txd };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_baudrate,	MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 100000} },
        { MP_QSTR_extframe, MP_ARG_KW_ONLY  | MP_ARG_BOOL,{.u_bool= false} },
        { MP_QSTR_mode,     MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = CAN_NORMAL_MODE} },
        { MP_QSTR_irq,      MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_callback, MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_priority, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 8} },
        { MP_QSTR_rxd,      MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_txd,      MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint can_id = args[0].u_int;
    if(can_id >= PYB_NUM_CANS) {
        mp_raise_OSError(MP_ENODEV);
    }

    pyb_can_obj_t *self = &pyb_can_obj[can_id];
    self->base.type = &pyb_can_type;

    self->baudrate = args[ARG_baudrate].u_int;
    self->extframe = args[ARG_extframe].u_bool;
    self->mode     = args[ARG_mode].u_int;

    if(args[ARG_rxd].u_obj != mp_const_none)
    {
        pin_config_by_func(args[ARG_rxd].u_obj, "%s_CAN%d_RXD", self->can_id);
    }

    if(args[ARG_txd].u_obj != mp_const_none)
    {
        pin_config_by_func(args[ARG_txd].u_obj, "%s_CAN%d_TXD", self->can_id);
    }

    switch(self->can_id) {
    case PYB_CAN_0: CLK_EnableModuleClock(CAN0_MODULE); break;
    case PYB_CAN_1: CLK_EnableModuleClock(CAN1_MODULE); break;
    default: break;
    }

    if(self->mode == CAN_NORMAL_MODE)
    {
        CAN_Open(self->CANx, self->baudrate, CAN_NORMAL_MODE);
    }
    else
    {
        CAN_Open(self->CANx, self->baudrate, CAN_BASIC_MODE);
        CAN_EnterTestMode(self->CANx, self->mode);
    }

	return self;
}


// Force a software restart of the controller, to allow transmission after a bus error
STATIC mp_obj_t pyb_can_restart(mp_obj_t self_in) {
    pyb_can_obj_t *self = self_in;

    CAN_EnterInitMode(self->CANx, 0);
	mp_hal_delay_ms(1000);
    CAN_LeaveInitMode(self->CANx);

    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_can_restart_obj, pyb_can_restart);


// Get the state of the controller
STATIC mp_obj_t pyb_can_state(mp_obj_t self_in) {
    pyb_can_obj_t *self = self_in;

    uint u32Status = CAN_GET_INT_STATUS(self->CANx);

    uint state = CAN_STATE_STOPPED;
    if(u32Status & CAN_STATUS_BOFF_Msk) {
        state = CAN_STATE_BUS_OFF;
    } else if(u32Status & CAN_STATUS_EPASS_Msk) {
        state = CAN_STATE_ERROR_PASSIVE;
    } else if(u32Status & CAN_STATUS_EWARN_Msk) {
        state = CAN_STATE_ERROR_WARNING;
    } else {
        state = CAN_STATE_ERROR_ACTIVE;
    }

    return MP_OBJ_NEW_SMALL_INT(state);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(pyb_can_state_obj, pyb_can_state);


STATIC mp_obj_t pyb_can_any(mp_obj_t self_in, mp_obj_t fifo_in) {
    pyb_can_obj_t *self = self_in;

    mp_int_t fifo = mp_obj_get_int(fifo_in);
 
    if((fifo >= 0) && (fifo < 32)) {
        if(CAN_IsNewDataReceived(self->CANx, fifo))
			return mp_const_true;
	}

    return mp_const_false;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_2(pyb_can_any_obj, pyb_can_any);


/// \method send(id, data, *, timeout=1000)
/// Send a message on the bus:
///
///   - `id`   is the address to send to
///   - `data` is the data to send (an integer to send, or a buffer object).
///   - `timeout` is the timeout in milliseconds to wait for the send.
///
/// Return value: `None`.
STATIC mp_obj_t pyb_can_send(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_id, ARG_data, ARG_timeout, ARG_remoteReq };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,        MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_data,      MP_ARG_REQUIRED | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_timeout,   MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 1000} },
        { MP_QSTR_remoteReq, MP_ARG_KW_ONLY  | MP_ARG_BOOL,{.u_bool= false} },
    };

    // parse args
    pyb_can_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t data[1];
    pyb_buf_get_for_send(args[ARG_data].u_obj, &bufinfo, data);

    if(bufinfo.len > 8) {
        mp_raise_ValueError("CAN data more than 8 bytes");
    }

    uint u32MessageObj;
    if(get_free_message_obj(self->CANx, args[ARG_timeout].u_int, &u32MessageObj) == false){
       mp_raise_OSError(MP_ETIMEDOUT);
	}

    STR_CANMSG_T tMsg;

    if(self->extframe) tMsg.IdType = CAN_EXT_ID;
    else               tMsg.IdType = CAN_STD_ID;

    if(args[ARG_remoteReq].u_bool) tMsg.FrameType= CAN_REMOTE_FRAME;
    else                           tMsg.FrameType= CAN_DATA_FRAME;

    tMsg.Id  = args[ARG_id].u_int;
    tMsg.DLC = bufinfo.len;

    for(uint i = 0; i < bufinfo.len; i ++)
		tMsg.Data[i] = ((byte*)bufinfo.buf)[i];

    if(CAN_Transmit(self->CANx, MSG(u32MessageObj), &tMsg) == FALSE) {
        mp_raise_OSError(MP_EIO);
    }

    return mp_const_none;

}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_can_send_obj, 1, pyb_can_send);


/// \method recv(fifo, *, timeout=1000)
///
/// Receive data on the bus:
///
///   - `fifo` is an integer, which is the FIFO to receive on
///   - `timeout` is the timeout in milliseconds to wait for the receive.
///
/// Return value: buffer of data bytes.
STATIC mp_obj_t pyb_can_recv(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_fifo, ARG_timeout };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_fifo,    MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_timeout, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 1000} },
    };

    // parse args
    pyb_can_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	uint32_t fifo = args[ARG_fifo].u_int;

	STR_CANMSG_T sCanMsg;

    if(can_receive_with_timeout(self->CANx, fifo, &sCanMsg, args[ARG_timeout].u_int) == false) {
       mp_raise_OSError(MP_ETIMEDOUT);
	}

    mp_obj_t tuple = mp_obj_new_tuple(3, NULL);
    mp_obj_t *items = ((mp_obj_tuple_t*)MP_OBJ_TO_PTR(tuple))->items;

    items[0] = MP_OBJ_NEW_SMALL_INT(sCanMsg.Id);
    items[1] = mp_obj_new_bytes(&sCanMsg.Data[0], sCanMsg.DLC);
    items[2] = sCanMsg.FrameType == CAN_REMOTE_FRAME ? mp_const_true : mp_const_false;

    return tuple;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_can_recv_obj, 1, pyb_can_recv);


STATIC mp_obj_t pyb_can_setfilter(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args) {
    enum { ARG_fifo, ARG_id, ARG_mask };
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_fifo, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_id,   MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_mask, MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
    };

    // parse args
    pyb_can_obj_t *self = pos_args[0];
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

	uint32_t fifo = args[ARG_fifo].u_int;

    if(!CAN_SetRxMsgAndMsk(self->CANx, fifo, self->extframe ? CAN_EXT_ID : CAN_STD_ID, args[ARG_id].u_int, args[ARG_mask].u_int)) {
		mp_raise_ValueError("CAN filter parameter error");
	}

	return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_KW(pyb_can_setfilter_obj, 1, pyb_can_setfilter);


STATIC const mp_rom_map_elem_t pyb_can_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_any),             MP_ROM_PTR(&pyb_can_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_send),            MP_ROM_PTR(&pyb_can_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_recv),            MP_ROM_PTR(&pyb_can_recv_obj) },
    { MP_ROM_QSTR(MP_QSTR_state),           MP_ROM_PTR(&pyb_can_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_restart),         MP_ROM_PTR(&pyb_can_restart_obj) },
    { MP_ROM_QSTR(MP_QSTR_setfilter),       MP_ROM_PTR(&pyb_can_setfilter_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_NORMAL),          MP_ROM_INT(CAN_NORMAL_MODE) },
    { MP_ROM_QSTR(MP_QSTR_LOOPBACK),        MP_ROM_INT(CAN_TEST_LBACK_Msk) },
    { MP_ROM_QSTR(MP_QSTR_SILENT),          MP_ROM_INT(CAN_TEST_SILENT_Msk) },
    { MP_ROM_QSTR(MP_QSTR_SILENT_LOOPBACK), MP_ROM_INT(CAN_TEST_SILENT_Msk | CAN_TEST_LBACK_Msk) },

    // values for CAN.state()
    { MP_ROM_QSTR(MP_QSTR_STOPPED),         MP_ROM_INT(CAN_STATE_STOPPED) },
    { MP_ROM_QSTR(MP_QSTR_ERROR_ACTIVE),    MP_ROM_INT(CAN_STATE_ERROR_ACTIVE) },
    { MP_ROM_QSTR(MP_QSTR_ERROR_WARNING),   MP_ROM_INT(CAN_STATE_ERROR_WARNING) },
    { MP_ROM_QSTR(MP_QSTR_ERROR_PASSIVE),   MP_ROM_INT(CAN_STATE_ERROR_PASSIVE) },
    { MP_ROM_QSTR(MP_QSTR_BUS_OFF),         MP_ROM_INT(CAN_STATE_BUS_OFF) },
};
STATIC MP_DEFINE_CONST_DICT(pyb_can_locals_dict, pyb_can_locals_dict_table);


mp_uint_t can_ioctl(mp_obj_t self_in, mp_uint_t request, mp_uint_t flags, int *errcode) {
    pyb_can_obj_t *self = self_in;

    mp_uint_t ret = 0;
    if(request == MP_STREAM_POLL) {
        if(flags & MP_STREAM_POLL_RD) {
            for(uint i = 0; i < 32; i++) {
                if(CAN_IsNewDataReceived(self->CANx, i)) {
					ret |= MP_STREAM_POLL_RD;
					break;
				}
			}			
        }
        if(flags & MP_STREAM_POLL_WR) {
			uint32_t u32MessageObj;
            if(get_free_message_obj(self->CANx, 0, &u32MessageObj)) {
				ret |= MP_STREAM_POLL_WR;
			}
        }
    } else {
        *errcode = MP_EINVAL;
        ret = -1;
    }

    return ret;
}


STATIC const mp_stream_p_t can_stream_p = {
    .ioctl = can_ioctl,
    .is_text = false,
};


const mp_obj_type_t pyb_can_type = {
    { &mp_type_type },
    .name = MP_QSTR_CAN,
    .print = pyb_can_print,
    .make_new = pyb_can_make_new,
    .protocol = &can_stream_p,
    .locals_dict = (mp_obj_t)&pyb_can_locals_dict,
};
