#include <stdint.h>

// board specific definitions
#include "mpconfigboard.h"


#define MICROPY_CONFIG_ROM_LEVEL (MICROPY_CONFIG_ROM_LEVEL_EXTRA_FEATURES)


/*****************************************************************************/
/* Memory allocation policy                                                  */
#define MICROPY_ALLOC_PATH_MAX                      (256)


/*****************************************************************************/
/* MicroPython emitters                                                      */
#define MICROPY_PERSISTENT_CODE_LOAD                (1)
#define MICROPY_EMIT_THUMB                          (1)
#define MICROPY_EMIT_INLINE_THUMB                   (1)


/*****************************************************************************/
/* Python internal features                                                  */
#define MICROPY_ENABLE_GC                           (1)
#define MICROPY_READER_VFS                          (1)     // Whether to use the VFS reader for importing files
#define MICROPY_FLOAT_IMPL                          (MICROPY_FLOAT_IMPL_FLOAT)
#define MICROPY_LONGINT_IMPL                        (MICROPY_LONGINT_IMPL_MPZ)

#define MICROPY_VFS                                 (1)


/*****************************************************************************/
/* Fine control over Python builtins, classes, modules, etc                  */
#define MICROPY_PY_SYS_PLATFORM                     "M480"

#define MICROPY_PY_TIME_GMTIME_LOCALTIME_MKTIME     (1)     // Whether to provide time.gmtime/localtime/mktime
#define MICROPY_PY_TIME_TIME_TIME_NS                (1)     // Whether to provide time.time/time_ns
#define MICROPY_PY_TIME_INCLUDEFILE                 "ports/m480/mods/modtime.c"


/* Extended modules */
#define MICROPY_PY_OS_SEP                           (1)
#define MICROPY_PY_OS_SYNC                          (1)
#define MICROPY_PY_OS_UNAME                         (1)
#define MICROPY_PY_OS_URANDOM                       (1)
#define MICROPY_PY_OS_DUPTERM                       (3)
#define MICROPY_PY_OS_DUPTERM_BUILTIN_STREAM        (1)
#define MICROPY_PY_OS_INCLUDEFILE                   "ports/m480/mods/modos.c"

// modmath need an implementation of the log2 function which is not a macro
#define MP_NEED_LOG2                                (1)


// fatfs configuration used in ffconf.h
#define MICROPY_FATFS_RPATH                         (2)
#define MICROPY_FATFS_USE_LABEL                     (1)
#define MICROPY_FATFS_ENABLE_LFN                    (1)
#define MICROPY_FATFS_LFN_CODE_PAGE                 437     // 1=SFN/ANSI 437=LFN/U.S.(OEM)


#define MP_STATE_PORT   MP_STATE_VM


// type definitions for the specific machine
#define MP_SSIZE_MAX    (0x7FFFFFFF)
typedef int             mp_int_t;       // must be pointer size
typedef unsigned int    mp_uint_t;      // must be pointer size
typedef long            mp_off_t;


#define MICROPY_MAKE_POINTER_CALLABLE(p)    ((void *)((mp_uint_t)(p) | 1))


/* 包含 M480.h 会导致许多宏定义冲突，将所需 __set_PRIMASK() 等内容提取到这里又会导致
 * 重复定义，因此只能在使用它们的地方将其内容展开，实现对应功能
 */
static inline void enable_irq(mp_uint_t state) {
//  __set_PRIMASK(state);
    __asm volatile ("MSR primask, %0" : : "r" (state) : "memory");
}

static inline mp_uint_t disable_irq(void) {
//  mp_uint_t state = __get_PRIMASK();
    mp_uint_t state;
    __asm volatile ("MRS %0, primask" : "=r" (state) );

//  __disable_irq();
    __asm volatile ("cpsid i" : : : "memory");

    return state;
}

#define MICROPY_BEGIN_ATOMIC_SECTION()     disable_irq()
#define MICROPY_END_ATOMIC_SECTION(state)  enable_irq(state)

#define MICROPY_EVENT_POLL_HOOK \
    do { \
        extern void mp_handle_pending(bool); \
        mp_handle_pending(true); \
        __asm volatile ("wfe"); \
    } while (0);


// We need to provide a declaration/definition of alloca()
#include <alloca.h>		// alloca()在栈中分配空间，离开此函数时自动释放


// There is no classical C heap in bare-metal ports, only Python garbage-collected heap.
// For completeness, emulate C heap via GC heap. Note that MicroPython core never uses malloc()
// and friends, so these defines are mostly to help extension module writers.
#define malloc(n) m_malloc(n)
#define free(p) m_free(p)
#define realloc(p, n) m_realloc(p, n)
