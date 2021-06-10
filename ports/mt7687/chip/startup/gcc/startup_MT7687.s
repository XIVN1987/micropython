    .arch armv7-m
    .syntax unified

    .section .stack
    .align 3
    .globl    __StackTop
    .globl    __StackLimit
__StackLimit:
    .space    0x10000
__StackTop:

    .section .heap
    .align 3
    .globl    __HeapBase
    .globl    __HeapLimit
__HeapBase:
    .space    0x20000
__HeapLimit:

    .section .isr_vector
    .align 2
    .globl __isr_vector
__isr_vector:
    .long    __StackTop            
    .long    Reset_Handler         
    .long    NMI_Handler          
    .long    HardFault_Handler     
    .long    MemManage_Handler     
    .long    BusFault_Handler      
    .long    UsageFault_Handler   
    .long    0                    
    .long    0                    
    .long    0                    
    .long    0                     
    .long    SVC_Handler          
    .long    DebugMon_Handler     
    .long    0                     
    .long    PendSV_Handler           
    .long    SysTick_Handler         

    /* External interrupts */
    .long   UART1_Handler
    .long   DMA_Handler
    .long   HIF_Handler   
    .long   I2C1_Handler
    .long   I2C2_Handler
    .long   UART2_Handler
    .long   CRYPTO_Handler
    .long   SF_Handler
    .long   EINT_Handler
    .long   BTIF_Handler
    .long   WDT_Handler
    .long   Default_Handler
    .long   SPIS_Handler
    .long   N9_WDT_Handler
    .long   ADC_Handler
    .long   IRDA_TX_Handler
    .long   IRDA_RX_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   GPT3_Handler
    .long   Default_Handler
    .long   AUDIO_Handler
    .long   N9_IRQ_Handler
    .long   GPT_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   SPIM_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    .long   Default_Handler
    
    .size    __isr_vector, . - __isr_vector


    .text
    .thumb
    .thumb_func
    .section .rst_handler
    .align 2
    .globl    Reset_Handler
    .type     Reset_Handler, %function
Reset_Handler:
/* Loop to copy data from read only memory to RAM */
    cpsid i

    ldr    sp, =__StackTop

    .equ    WDT_Base,     0x83080030
    .equ    WDT_Disable,  0x2200

    ldr  r0, =WDT_Base
    ldr  r1, =WDT_Disable
    str  r1, [r0, #0]

    bl     pre_cache_init

    ldr    r1, =__data_load__
    ldr    r2, =__data_start__
    ldr    r3, =__data_end__

.Lflash_to_ram_loop:
    cmp     r2, r3
    ittt    lo
    ldrlo   r0, [r1], #4
    strlo   r0, [r2], #4
    blo    .Lflash_to_ram_loop

    ldr    r2, =__bss_start__
    ldr    r3, =__bss_end__

.Lbss_to_ram_loop:
    cmp     r2, r3
    ittt    lo
    movlo   r0, #0
    strlo   r0, [r2], #4
    blo    .Lbss_to_ram_loop

    ldr    r1, =__tcmtext_load__
    ldr    r2, =__tcmtext_start__
    ldr    r3, =__tcmtext_end__

.Lflash_to_tcm_loop:
    cmp     r2, r3
    ittt    lo
    ldrlo   r0, [r1], #4
    strlo   r0, [r2], #4
    blo    .Lflash_to_tcm_loop

    ldr    r0, =SystemInit
    blx    r0

    ldr    r0, =main
    bx     r0
    .pool
    .size Reset_Handler, . - Reset_Handler


    .text
    .thumb
    .thumb_func
    .section .pre_cache_init
    .align 2
    .globl    pre_cache_init
    .type     pre_cache_init, %function
pre_cache_init:
    .equ   Cache_Ctrl_Base,      0x01530000
    .equ   Cache_Ctrl_Op,        0x01530004
    .equ   Cache_Channel_En,     0x0153002C
    .equ   Cache_Entry_Start,    0x01540000
    .equ   Cache_Entry_End,      0x01540040

    .equ   Cache_Disable,        0x0
    .equ   Cache_Enable,         0x30D
    .equ   Cache_Flush_All,      0x13
    .equ   Cache_Invalidate_All, 0x03
    .equ   Cache_Start_Addr,     0x10000100
    .equ   Cache_End_Addr,       0x11000000

    ldr     r0, =Cache_Ctrl_Base
    movs    r1, #Cache_Disable
    str     r1, [r0, #0]

    ldr     r0, =Cache_Ctrl_Op
    movs    r1, #Cache_Flush_All
    str     r1, [r0, #0]

    ldr     r0, =Cache_Ctrl_Op
    movs    r1, #Cache_Invalidate_All
    str     r1, [r0, #0]

    ldr     r0, =Cache_Entry_Start
    ldr     r1, =Cache_Start_Addr
    str     r1, [r0, #0]

    ldr     r0, =Cache_Entry_End
    ldr     r1, =Cache_End_Addr
    str     r1, [r0, #0]

    ldr     r0, =Cache_Channel_En
    movs    r1, #0x1
    str     r1, [r0, #0]

    ldr     r0, =Cache_Ctrl_Base
    ldr     r1, =Cache_Enable
    str     r1, [r0, #0]

    bx      lr
    .size pre_cache_init, . - pre_cache_init


    .text
/* Macro to define default handlers. 
   Default handler will be weak symbol and just dead loops. */
    .macro    def_default_handler    handler_name
    .align 1
    .thumb_func
    .weak    \handler_name
    .type    \handler_name, %function
\handler_name :
    b    .
    .size    \handler_name, . - \handler_name
    .endm

    def_default_handler    NMI_Handler
    def_default_handler    HardFault_Handler
    def_default_handler    MemManage_Handler
    def_default_handler    BusFault_Handler
    def_default_handler    UsageFault_Handler
    def_default_handler    SVC_Handler
    def_default_handler    DebugMon_Handler
    def_default_handler    PendSV_Handler
    def_default_handler    SysTick_Handler

    def_default_handler    UART1_Handler
	def_default_handler    DMA_Handler
	def_default_handler    HIF_Handler   
	def_default_handler    I2C1_Handler
	def_default_handler    I2C2_Handler
	def_default_handler    UART2_Handler
	def_default_handler    CRYPTO_Handler
	def_default_handler    SF_Handler
	def_default_handler    EINT_Handler
	def_default_handler    BTIF_Handler
	def_default_handler    WDT_Handler
	def_default_handler    SPIS_Handler
	def_default_handler    N9_WDT_Handler
	def_default_handler    ADC_Handler
	def_default_handler    IRDA_TX_Handler
	def_default_handler    IRDA_RX_Handler
	def_default_handler    GPT3_Handler
	def_default_handler    AUDIO_Handler
	def_default_handler    N9_IRQ_Handler
    def_default_handler    GPT_Handler
	def_default_handler    SPIM_Handler

    def_default_handler    Default_Handler

    .end
