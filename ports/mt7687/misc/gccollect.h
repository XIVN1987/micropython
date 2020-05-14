#ifndef __MISC_GCCOLLECT_H__
#define __MISC_GCCOLLECT_H__

// variables defining memory layout
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __heap_start__;
extern uint32_t __heap_end__;
extern uint32_t __stack_start__;
extern uint32_t __stack_end__;

void gc_collect(void);

#endif
