/* Entry Point */
ENTRY(Reset_Handler)


/* Specify the memory areas */
MEMORY
{
	ROM(arx) : ORIGIN = 0x00000000, LENGTH = 0x00080000 /* 512k */
	RAM(arw) : ORIGIN = 0x20000000, LENGTH = 0x00028000 /* 160k */
}


/* Define output sections */
SECTIONS
{
    . = ORIGIN(ROM);
    .text :
    {
		KEEP(*(.vectors))

        *(.text)
        *(.text*)
        *(.rodata*)
    } > ROM

    . = ALIGN(4);
    __etext = LOADADDR(.data);

    . = ALIGN(4);
    .data :
    {
        __data_start__ = .;
	    
        *(.data)
        *(.data*)

        . = ALIGN(4);
        __data_end__ = .;
    } > RAM AT> ROM

    . = ALIGN(4);
    .bss :
    {
        __bss_start__ = .;

        *(.bss)
        *(.bss*)
        *(COMMON)

        . = ALIGN(4);
        __bss_end__ = .;
    } > RAM
    . = ALIGN(4);

    .heap :
    {
        __HeapBase = .;

        *(.heap)

        . = ALIGN(4);
        __HeapLimit = .;
    } > RAM

    /* .stack_dummy section doesn't contains any symbols.
     * It is only used for linker to calculate size of stack sections */
    .stack_dummy :
    {
        *(.stack)
    } > RAM

    __StackTop   = ORIGIN(RAM) + LENGTH(RAM);
    __StackLimit = __StackTop - SIZEOF(.stack_dummy);
    ASSERT(__StackLimit >= __HeapLimit, "region RAM overflowed with stack")
}
