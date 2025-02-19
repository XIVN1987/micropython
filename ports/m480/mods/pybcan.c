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


#define TX_MsgObj_Count  8     // 32个message object，前8个用于发送，后24个用于接收（每3个构成1个接收FIFO，共8个接收FIFO）

#define CAN_TX_SUCC      1   // Transmit Success
#define CAN_TX_FAIL      2   // Transmit Failure

#define CAN_STAT_EACT    0   // ERROR ACTIVE,  1 error_counter <   96
#define CAN_STAT_EWARN   1   // ERROR WARNING, 1 error_counter >=  96
#define CAN_STAT_EPASS   2   // ERROR PASSIVE, 1 error_counter >= 127
#define CAN_STAT_BUSOFF  3   // BUS OFF,       1 error_counter >= 255


/******************************************************************************
 DEFINE PRIVATE TYPES
 ******************************************************************************/
typedef enum {
    PYB_CAN_0   = 0,
    PYB_CAN_1   = 1,
    PYB_NUM_CANS
} pyb_can_id_t;


typedef struct {
    mp_obj_base_t base;
    mp_uint_t can_id;
    CAN_T *CANx;
    uint baudrate;
    uint8_t  mode;
    
    struct {
        uint8_t  mode;       // CAN_FRAME_STD、CAN_FRAME_EXT
        uint32_t check;      // check & mask == ID & mask 的Message通过过滤
        uint32_t mask;
    } filter[8];
    uint8_t n_filter;

    uint8_t  IRQn;
    uint8_t  irq_flags;
    uint8_t  irq_trigger;
    uint8_t  irq_priority;   // 中断优先级
    mp_obj_t irq_callback;   // 中断处理函数
} pyb_can_obj_t;


/******************************************************************************
 DEFINE PRIVATE DATA
 ******************************************************************************/
static pyb_can_obj_t pyb_can_obj[PYB_NUM_CANS] = {
    { {&pyb_can_type}, .can_id=PYB_CAN_0, .CANx=CAN0, .IRQn=CAN0_IRQn },
    { {&pyb_can_type}, .can_id=PYB_CAN_1, .CANx=CAN1, .IRQn=CAN1_IRQn },
};


/******************************************************************************
 DEFINE PRIVATE FUNCTIONS
 ******************************************************************************/
static int get_free_MsgObj(CAN_T *CANx, bool tx)
{
    uint start = tx ? 0 : TX_MsgObj_Count;
    uint end   = tx ? TX_MsgObj_Count : 32;
    
    for(uint i = start; i < end; i++)
    {
        if((i < 16 ? CANx->MVLD1 & (1 << i) : CANx->MVLD2 & (1 << (i - 16))) == 0)
        {
            return i;
        }
    }
    
    return -1;
}


void CANx_IRQHandler(pyb_can_obj_t *self)
{
}

void CAN0_IRQHandler(void)
{
    CANx_IRQHandler(&pyb_can_obj[0]);
}

void CAN1_IRQHandler(void)
{
    CANx_IRQHandler(&pyb_can_obj[1]);
}


/******************************************************************************/
// MicroPython bindings

static void can_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    pyb_can_obj_t *self = self_in;

    qstr mode;
    switch(self->mode)
    {
        case CAN_NORMAL_MODE:      mode = MP_QSTR_NORMAL;    break;
        case CAN_TEST_SILENT_Msk:  mode = MP_QSTR_LISTEN;    break;
        case CAN_TEST_LBACK_Msk:   mode = MP_QSTR_LOOPBACK;  break;
    }

    mp_printf(print, "CAN(%u, %u, mode=%q)", self->can_id, self->baudrate, mode);
}


static mp_obj_t can_make_new(const mp_obj_type_t *type, size_t n_args, size_t n_kw, const mp_obj_t *all_args)
{
    enum { ARG_id, ARG_baudrate, ARG_mode, ARG_irq, ARG_callback, ARG_priority, ARG_rxd, ARG_txd };
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,       MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_baudrate,	MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 100000} },
        { MP_QSTR_mode,     MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = CAN_NORMAL_MODE} },
        { MP_QSTR_irq,      MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_callback, MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_priority, MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = 4} },
        { MP_QSTR_rxd,      MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_txd,      MP_ARG_KW_ONLY  | MP_ARG_OBJ, {.u_obj = mp_const_none} },
    };

    // parse args
    mp_map_t kw_args;
    mp_map_init_fixed_table(&kw_args, n_kw, all_args + n_args);
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args, all_args, &kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    uint can_id = args[ARG_id].u_int;
    if(can_id >= PYB_NUM_CANS)
    {
        mp_raise_OSError(MP_ENODEV);
    }
    pyb_can_obj_t *self = &pyb_can_obj[can_id];

    self->baudrate = args[ARG_baudrate].u_int;
    self->mode     = args[ARG_mode].u_int;
    if((self->mode != CAN_NORMAL_MODE) && (self->mode != CAN_TEST_SILENT_Msk) && (self->mode != CAN_TEST_LBACK_Msk))
    {
        mp_raise_ValueError("invalid mode value");
    }
    
    self->irq_trigger  = args[ARG_irq].u_int;
    self->irq_callback = args[ARG_callback].u_obj;
    self->irq_priority = args[ARG_priority].u_int;
    if(self->irq_priority > 7)
    {
        mp_raise_ValueError("invalid priority value");
    }
    else
    {
        NVIC_SetPriority(self->IRQn, self->irq_priority << 1);
    }

    if(args[ARG_rxd].u_obj != mp_const_none)
    {
        pin_config_by_func(args[ARG_rxd].u_obj, "%s_CAN%d_RXD", self->can_id);
    }

    if(args[ARG_txd].u_obj != mp_const_none)
    {
        pin_config_by_func(args[ARG_txd].u_obj, "%s_CAN%d_TXD", self->can_id);
    }

    switch(self->can_id)
    {
    case PYB_CAN_0: CLK_EnableModuleClock(CAN0_MODULE); break;
    case PYB_CAN_1: CLK_EnableModuleClock(CAN1_MODULE); break;
    }

    uint baudrate = CAN_Open(self->CANx, self->baudrate, CAN_NORMAL_MODE);
    if(baudrate != self->baudrate)
    {
        mp_raise_ValueError("unsupported baudrate");
    }
    
    if(self->mode != CAN_NORMAL_MODE)
    {
        CAN_EnterTestMode(self->CANx, self->mode);
    }

	return self;
}


static mp_obj_t can_baudrate(size_t n_args, const mp_obj_t *args)
{
    pyb_can_obj_t *self = args[0];

    if(n_args == 1)     // get
    {
        return mp_obj_new_int(self->baudrate);
    }
    else                // set
    {
        self->baudrate = mp_obj_get_int(args[1]);
        
        uint baudrate = CAN_SetBaudRate(self->CANx, self->baudrate);
        if(baudrate != self->baudrate)
        {
            mp_raise_ValueError("unsupported baudrate");
        }

        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(can_baudrate_obj, 1, 2, can_baudrate);


static mp_obj_t can_any(mp_obj_t self_in)
{
    pyb_can_obj_t *self = self_in;

    for(uint i = TX_MsgObj_Count; i < 32; i++)
    {
        if(CAN_IsNewDataReceived(self->CANx, i))
            return mp_const_true;
    }

    return mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_1(can_any_obj, can_any);


static mp_obj_t can_full(mp_obj_t self_in)
{
    pyb_can_obj_t *self = self_in;

    int msgobj = get_free_MsgObj(self->CANx, true);
    if(msgobj < 0)
        return mp_const_true;

    return mp_const_false;
}
static MP_DEFINE_CONST_FUN_OBJ_1(can_full_obj, can_full);


// used when can_full() return False
static mp_obj_t can_send(size_t n_args, const mp_obj_t *pos_args, mp_map_t *kw_args)
{
    enum { ARG_id, ARG_data, ARG_format, ARG_remoteReq, ARG_retry };
    const mp_arg_t allowed_args[] = {
        { MP_QSTR_id,        MP_ARG_REQUIRED | MP_ARG_INT, {.u_int = 0} },
        { MP_QSTR_data,                        MP_ARG_OBJ, {.u_obj = mp_const_none} },
        { MP_QSTR_format,    MP_ARG_KW_ONLY  | MP_ARG_INT, {.u_int = CAN_STD_ID} },
        { MP_QSTR_remoteReq, MP_ARG_KW_ONLY  | MP_ARG_BOOL,{.u_bool= false} },
        { MP_QSTR_retry,     MP_ARG_KW_ONLY  | MP_ARG_BOOL,{.u_bool= true} },
    };

    // parse args
    mp_arg_val_t args[MP_ARRAY_SIZE(allowed_args)];
    mp_arg_parse_all(n_args - 1, pos_args + 1, kw_args, MP_ARRAY_SIZE(allowed_args), allowed_args, args);

    pyb_can_obj_t *self = pos_args[0];
    
    int msgobj = get_free_MsgObj(self->CANx, true);
    if(msgobj < 0)
    {
        mp_raise_OSError(MP_EIO);
	}

    // get the buffer to send from
    mp_buffer_info_t bufinfo;
    uint8_t tmp[1];
    pyb_buf_get_for_send(args[ARG_data].u_obj, &bufinfo, tmp);

    if(bufinfo.len > 8)
    {
        mp_raise_ValueError("CAN data more than 8 bytes");
    }

    STR_CANMSG_T txMsg;
    txMsg.IdType = args[ARG_format].u_int;
    txMsg.FrameType = args[ARG_remoteReq].u_bool ? CAN_REMOTE_FRAME : CAN_DATA_FRAME;
    txMsg.Id  = args[ARG_id].u_int;
    txMsg.DLC = bufinfo.len;

    for(uint i = 0; i < bufinfo.len; i ++)
		txMsg.Data[i] = ((byte*)bufinfo.buf)[i];

    CAN_Transmit(self->CANx, msgobj, &txMsg);

    return mp_const_none;

}
static MP_DEFINE_CONST_FUN_OBJ_KW(can_send_obj, 3, can_send);


// used when can_any() return True
static mp_obj_t can_read(mp_obj_t self_in)
{
    pyb_can_obj_t *self = self_in;

    STR_CANMSG_T rxMsg;
    
    for(uint i = TX_MsgObj_Count; i < 32; i++)
    {
        if(CAN_IsNewDataReceived(self->CANx, i))
        {
            CAN_Receive(self->CANx, i, &rxMsg);
            break;
        }
    }
    
    mp_obj_t tuple = mp_obj_new_tuple(3, NULL);
    mp_obj_t *items = ((mp_obj_tuple_t*)MP_OBJ_TO_PTR(tuple))->items;

    items[0] = (rxMsg.IdType == CAN_STD_ID) ? MP_OBJ_NEW_QSTR(MP_QSTR_FRAME_STD) : MP_OBJ_NEW_QSTR(MP_QSTR_FRAME_EXT);
    items[1] = MP_OBJ_NEW_SMALL_INT(rxMsg.Id);
    if(rxMsg.FrameType == CAN_REMOTE_FRAME)
        items[2] = mp_const_true;
    else
        items[2] = mp_obj_new_bytes(rxMsg.Data, rxMsg.DLC);

    return tuple;
}
static MP_DEFINE_CONST_FUN_OBJ_1(can_read_obj, can_read);


static mp_obj_t can_state(mp_obj_t self_in)
{
    pyb_can_obj_t *self = self_in;

    uint state = 0;
    uint can_sr = CAN_GET_INT_STATUS(self->CANx);

    if(can_sr & CAN_STATUS_BOFF_Msk)
        state = CAN_STAT_BUSOFF;
    else if(can_sr & CAN_STATUS_EPASS_Msk)
        state = CAN_STAT_EPASS;
    else if(can_sr & CAN_STATUS_EWARN_Msk)
        state = CAN_STAT_EWARN;
    else
        state = CAN_STAT_EACT;

    return MP_OBJ_NEW_SMALL_INT(state);
}
static MP_DEFINE_CONST_FUN_OBJ_1(can_state_obj, can_state);


static mp_obj_t can_filter(size_t n_args, const mp_obj_t *args)
{
    pyb_can_obj_t *self = args[0];
    
    if(n_args == 1)     // get
    {
        mp_obj_t filters = mp_obj_new_list(self->n_filter, NULL);
        mp_obj_t *filter = ((mp_obj_tuple_t*)MP_OBJ_TO_PTR(filters))->items;
        for(uint i = 0; i < self->n_filter; i++)
        {
            filter[i] = mp_obj_new_tuple(3, NULL);
            mp_obj_t *items = ((mp_obj_tuple_t*)MP_OBJ_TO_PTR(filter[i]))->items;
            
            items[0] = (self->filter[i].mode == CAN_STD_ID) ? MP_OBJ_NEW_QSTR(MP_QSTR_FRAME_STD) : MP_OBJ_NEW_QSTR(MP_QSTR_FRAME_EXT);
            items[1] = mp_obj_new_int(self->filter[i].check);
            items[2] = mp_obj_new_int(self->filter[i].mask);
        }
        
        return filters;
    }
    else                // set
    {
        uint len;
        mp_obj_t *items;
    
        mp_obj_get_array(args[1], &len, &items);
        
        self->n_filter = (len > 8) ? 8 : len;
    
        for(uint i = 0; i < self->n_filter; i++)
        {
            uint len2;
            mp_obj_t *items2;
            
            mp_obj_get_array(items[i], &len2, &items2);
            
            self->filter[i].mode  = mp_obj_get_int(items2[0]);
            self->filter[i].check = mp_obj_get_int(items2[1]);
            self->filter[i].mask  = mp_obj_get_int(items2[2]);
        }
        
        for(uint i = 0; i < self->n_filter; i++)
        {
            uint msgobj = TX_MsgObj_Count + i*3;
            
            CAN_SetRxMsgObjAndMsk(self->CANx, msgobj,   self->filter[i].mode, self->filter[i].check, self->filter[i].mode, false);
            CAN_SetRxMsgObjAndMsk(self->CANx, msgobj+1, self->filter[i].mode, self->filter[i].check, self->filter[i].mode, false);
            CAN_SetRxMsgObjAndMsk(self->CANx, msgobj+2, self->filter[i].mode, self->filter[i].check, self->filter[i].mode, true);
        }
        
        return mp_const_none;
    }
}
static MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(can_filter_obj, 1, 2, can_filter);


static const mp_rom_map_elem_t can_locals_dict_table[] = {
    // instance methods
    { MP_ROM_QSTR(MP_QSTR_baudrate),     MP_ROM_PTR(&can_baudrate_obj) },
    { MP_ROM_QSTR(MP_QSTR_any),          MP_ROM_PTR(&can_any_obj) },
    { MP_ROM_QSTR(MP_QSTR_full),         MP_ROM_PTR(&can_full_obj) },
    { MP_ROM_QSTR(MP_QSTR_send),         MP_ROM_PTR(&can_send_obj) },
    { MP_ROM_QSTR(MP_QSTR_read),         MP_ROM_PTR(&can_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_state),        MP_ROM_PTR(&can_state_obj) },
    { MP_ROM_QSTR(MP_QSTR_filter),       MP_ROM_PTR(&can_filter_obj) },

    // class constants
    { MP_ROM_QSTR(MP_QSTR_NORMAL),       MP_ROM_INT(CAN_NORMAL_MODE) },
    { MP_ROM_QSTR(MP_QSTR_LISTEN),       MP_ROM_INT(CAN_TEST_SILENT_Msk) },
    { MP_ROM_QSTR(MP_QSTR_LOOPBACK),     MP_ROM_INT(CAN_TEST_LBACK_Msk) },
    
    { MP_ROM_QSTR(MP_QSTR_FRAME_STD),    MP_ROM_INT(CAN_STD_ID) },
    { MP_ROM_QSTR(MP_QSTR_FRAME_EXT),    MP_ROM_INT(CAN_EXT_ID) },
    
    { MP_ROM_QSTR(MP_QSTR_TX_SUCC),      MP_ROM_INT(CAN_TX_SUCC) },
    { MP_ROM_QSTR(MP_QSTR_TX_FAIL),      MP_ROM_INT(CAN_TX_FAIL) },
        
    { MP_ROM_QSTR(MP_QSTR_STAT_EACT),    MP_ROM_INT(CAN_STAT_EACT)   },
    { MP_ROM_QSTR(MP_QSTR_STAT_EWARN),   MP_ROM_INT(CAN_STAT_EWARN)  },
    { MP_ROM_QSTR(MP_QSTR_STAT_EPASS),   MP_ROM_INT(CAN_STAT_EPASS)  },
    { MP_ROM_QSTR(MP_QSTR_STAT_BUSOFF),  MP_ROM_INT(CAN_STAT_BUSOFF) },
};
static MP_DEFINE_CONST_DICT(can_locals_dict, can_locals_dict_table);


MP_DEFINE_CONST_OBJ_TYPE(
    pyb_can_type,
    MP_QSTR_CAN,
    MP_TYPE_FLAG_NONE,
    print, can_print,
    make_new, can_make_new,
    locals_dict, &can_locals_dict
);
