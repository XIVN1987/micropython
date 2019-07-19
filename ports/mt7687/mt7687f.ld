/* Entry Point */
ENTRY(Reset_Handler)

/* Specify the memory areas */
MEMORY
{
    ROM_CODE   (arx) : ORIGIN = 0x0000000,     LENGTH = 0x00005000 /*   20k */
    BOOTLOADER (arx) : ORIGIN = 0x10000000,    LENGTH = 0x00008000 /*   32k */
    RESERVED   (ar)  : ORIGIN = 0x10008000,    LENGTH = 0x00008000 /*   32k */
    N9_CODE    (ar)  : ORIGIN = 0x10010000,    LENGTH = 0x00069000 /*  420k */
    XIP_CODE   (arx) : ORIGIN = 0x10079000,    LENGTH = 0x0017E000 /* 1528k */
    ROM_NVDM   (ar)  : ORIGIN = 0x101F7000,    LENGTH = 0x00009000 /*   36k */
    TCMRAM     (arw) : ORIGIN = 0x00100100,    LENGTH = 0x0000FF00 /*   64k, 256 bytes for isr_vector */
    SYSRAM     (arw) : ORIGIN = 0x20000000,    LENGTH = 0x0003FC00 /*  255k */
}

/* Define output sections */
SECTIONS
{
    . = ORIGIN(XIP_CODE);
    .text :
    {
        KEEP(*(.rst_handler))
        KEEP(*(.pre_cache_init))

        *(.text)
        *(.text*)
        *(.rodata*)
    } > XIP_CODE

    . = ALIGN(4);
    __data_load__ = LOADADDR(.data);

    . = ALIGN(4);
    .data :
    {
        __data_start__ = .;
        
        KEEP(*(.isr_vector))

        *(.data)
        *(.data*)

        . = ALIGN(4);
        __data_end__ = .;
    } > SYSRAM AT> XIP_CODE

    . = ALIGN(4);
    __tcmtext_load__ = LOADADDR(.tcmTEXT);

    . = ALIGN(4);
    .tcmTEXT :
    {
        __tcmtext_start__ = .;

        *(.tcmTEXT)

        . = ALIGN(4);
        __tcmtext_end__ = .;
    } > TCMRAM AT> XIP_CODE

    . = ALIGN(4);
    .bss :
    {
        __bss_start__ = .;

        *(.bss)
        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        __bss_end__ = .;
    } > SYSRAM
    . = ALIGN(4);

    .heap :
    {
        __heap_start__ = .;

        *(.heap)

        __heap_end__ = .;
        __HeapLimit = .;
    } > SYSRAM

    /* .stack_dummy section doesn't contains any symbols.
     * It is only used for linker to calculate size of stack sections */
    .stack_dummy :
    {
        __stack_start__ = .;

        *(.stack)

        __stack_end__ = .;
    } > SYSRAM

    __StackTop   = ORIGIN(SYSRAM) + LENGTH(SYSRAM);
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);
    ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
}
