#ifndef __MISC_GCCOLLECT_H__
#define __MISC_GCCOLLECT_H__


// variables defining memory layout
extern uint32_t __data_start__;
extern uint32_t __data_end__;
extern uint32_t __bss_start__;
extern uint32_t __bss_end__;
extern uint32_t __HeapBase;
extern uint32_t __HeapLimit;
extern uint32_t __StackLimit;
extern uint32_t __StackTop;


void gc_collect(void);


#endif
