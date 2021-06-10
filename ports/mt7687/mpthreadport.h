#ifndef __MPTHREADPORT_H__
#define __MPTHREADPORT_H__

#ifndef BOOTLOADER
#include "FreeRTOS.h"
#endif

typedef struct _mp_thread_mutex_t {
    #ifndef BOOTLOADER
    SemaphoreHandle_t handle;
    StaticSemaphore_t buffer;
    #endif
} mp_thread_mutex_t;

void mp_thread_init(void);
void mp_thread_gc_others(void);

#endif
