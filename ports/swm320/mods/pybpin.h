#ifndef __MODS_PYBPIN_H__
#define __MODS_PYBPIN_H__


typedef struct {
    const mp_obj_base_t base;
    const qstr          name;
    GPIO_TypeDef       *port;
    uint8_t             pbit;
    volatile uint32_t  *preg;           // Pin Data Bit-Band
    qstr                alt;
    uint8_t             dir;
    uint8_t             pull;

    uint8_t             IRQn;
    uint8_t             irq_trigger;
    uint8_t             irq_priority;   // 中断优先级
    mp_obj_t            irq_callback;   // 中断处理函数
} pin_obj_t;

#include "pins.h"

extern const mp_obj_type_t pin_type;

extern const mp_obj_dict_t pins_locals_dict;

pin_obj_t *pin_find_by_name(mp_obj_t name);
pin_obj_t *pin_find_by_port_bit(GPIO_TypeDef *port, uint pbit);
void pin_config(pin_obj_t *self, uint alt, uint dir, uint mode);


#endif
