#ifndef __MT7687_H__
#define __MT7687_H__

typedef enum IRQn {
    /****  CM4 internal exceptions  **********/
    Reset_IRQn            = -15,
    NonMaskableInt_IRQn   = -14,
    HardFault_IRQn        = -13,
    MemoryManagement_IRQn = -12,
    BusFault_IRQn         = -11,
    UsageFault_IRQn       = -10,
    SVC_IRQn              = -5,
    DebugMonitor_IRQn     = -4,
    PendSV_IRQn           = -2,
    SysTick_IRQn          = -1,

    /****  MT7687 specific interrupt *********/
    UART1_IRQ             = 0 ,
    DMA_IRQ               = 1 ,
    HIF_IRQ               = 2 ,
    I2C1_IRQ              = 3 ,
    I2C2_IRQ              = 4 ,
    UART2_IRQ             = 5 ,
    CRYPTO_IRQ            = 6,
    SF_IRQ                = 7,
    EINT_IRQ              = 8,
    BTIF_IRQ              = 9,
    WDT_IRQ               = 10,
    SPIS_IRQ              = 12,
    N9_WDT_IRQ            = 13,
    ADC_IRQ               = 14,
    IRDA_TX_IRQ           = 15,
    IRDA_RX_IRQ           = 16,
    GPT3_IRQ              = 20,
    AUDIO_IRQ             = 22,
    GPT_IRQ               = 24,
    SPIM_IRQ              = 27,
} IRQn_Type;

#define __NVIC_PRIO_BITS    3
#define __FPU_PRESENT       1

#define NVIC_PRIORITY_LVL_0      0x00
#define NVIC_PRIORITY_LVL_1      0x20
#define NVIC_PRIORITY_LVL_2      0x40
#define NVIC_PRIORITY_LVL_3      0x60
#define NVIC_PRIORITY_LVL_4      0x80
#define NVIC_PRIORITY_LVL_5      0xA0
#define NVIC_PRIORITY_LVL_6      0xC0
#define NVIC_PRIORITY_LVL_7      0xE0


#include <stdio.h>
#include <stdint.h>
#include "core_cm4.h"
#include "system_MT7687.h"


#if   defined ( __CC_ARM )
  #pragma anon_unions
#endif

typedef struct {
	__IO uint32_t IF;
	__IO uint32_t IE;
	__IO uint32_t RESERVED1[2];
	__IO uint32_t T0CR;
	__IO uint32_t T0IV;			// Initial Value
	__IO uint32_t RESERVED2[2];
	__IO uint32_t T1CR;
	__IO uint32_t T1IV;
	__IO uint32_t RESERVED3[2];
	__IO uint32_t T2CR;
	__I  uint32_t T2V;
	__IO uint32_t RESERVED4[2];
	__I  uint32_t T0V;
	__I  uint32_t T1V;
	__IO uint32_t RESERVED5[6];
	__IO uint32_t T4CR;
	__IO uint32_t T4IV;
	__I  uint32_t T4V;
} TIMR_TypeDef;


#define TIMR_IF_T0_Pos			0
#define TIMR_IF_T0_Msk			(0x01 << TIMR_IF_T0_Pos)
#define TIMR_IF_T1_Pos			1
#define TIMR_IF_T1_Msk			(0x01 << TIMR_IF_T1_Pos)

#define TIMR_IE_T0_Pos			0
#define TIMR_IE_T0_Msk			(0x01 << TIMR_IE_T0_Pos)
#define TIMR_IE_T1_Pos			1
#define TIMR_IE_T1_Msk			(0x01 << TIMR_IE_T1_Pos)

#define TIMR_CR_EN_Pos			0
#define TIMR_CR_EN_Msk			(0x01 << TIMR_CR_EN_Pos)
#define TIMR_CR_AR_Pos			1		// Auto Repeat
#define TIMR_CR_AR_Msk			(0x01 << TIMR_CR_AR_Pos)
#define TIMR_CR_32K_Pos			2		// 时基：0 1KHz   1 32.768KHz
#define TIMR_CR_32K_Msk			(0x01 << TIMR_CR_32K_Pos)
#define TIMR_CR_RESTART_Pos		3
#define TIMR_CR_RESTART_Msk		(0x01 << TIMR_CR_RESTART_Pos)




typedef struct {
	union {
		__I  uint32_t RBR;                
		__O  uint32_t THR; 
		__IO uint32_t DLL;    
	};
	union {
		__IO uint32_t IER; 
		__IO uint32_t DLH;
	};
	union {
		__I  uint32_t IIR;                
		__O  uint32_t FCR;  
	};
    __IO uint32_t LCR;                
    __IO uint32_t MCR;                
    __IO uint32_t LSR;                
    __IO uint32_t MSR;                
    __IO uint32_t SCR;
    __I  uint32_t RSV;
    __IO uint32_t HSP;  // High Speed
} UART_TypeDef;


#define UART_IER_RDA_Pos		0		// Receive Data Avaliable
#define UART_IER_RDA_Mks		(0x01 << UART_IER_RDA_Pos)
#define UART_IER_THE_Pos		1		// TX Holding Empty or TX FIFO reach Trigger Level
#define UART_IER_THE_Msk		(0x01 << UART_IER_THE_Pos)
#define UART_IER_ELS_Pos		2		// Line Status Error(BI, FE, PE, OE)
#define UART_IER_ELS_Msk		(0x01 << UART_IER_ELS_Pos)

#define UART_IIR_IID_Pos		0		// Interrupt ID
#define UART_IIR_IID_Msk		(0x3F << UART_IIR_IID_Pos)
#define UART_IIR_ERROR          0x06
#define UART_IIR_RDA            0x04
#define UART_IIR_TIMEOUT        0x0C
#define UART_IIR_THRE           0x02

#define UART_FCR_FFEN_Pos		0		// FIFO Enable
#define UART_FCR_FFEN_Msk		(0x01 << UART_FCR_FFEN_Pos)
#define UART_FCR_CLRR_Pos		1		// Clear RX FIFO
#define UART_FCR_CLRR_Msk		(0x01 << UART_FCR_CLRR_Pos)
#define UART_FCR_CLRT_Pos		2		// Clear TX FIFO
#define UART_FCR_CLRT_Msk		(0x01 << UART_FCR_CLRT_Pos)
#define UART_FCR_TFTL_Pos		4		// TX FIFO trigger level
#define UART_FCR_TFTL_Msk		(0x03 << UART_FCR_TFTL_Pos)
#define UART_FCR_RFTL_Pos		6		// RX FIFO trigger level
#define UART_FCR_RFTL_Msk		(0x03 << UART_FCR_RFTL_Pos)

#define UART_LCR_WLS_Pos		0		// Word Length Select
#define UART_LCR_WLS_Msk		(0x03 << UART_LCR_WLS_Pos)
#define UART_LCR_STB_Pos		2		// STOP bits
#define UART_LCR_STB_Msk		(0x01 << UART_LCR_STB_Pos)
#define UART_LCR_PEN_Pos		3		// Parity Enable
#define UART_LCR_PEN_Msk		(0x01 << UART_LCR_PEN_Pos)
#define UART_LCR_EPS_Pos		4		// Even Parity Select
#define UART_LCR_EPS_Msk		(0x01 << UART_LCR_EPS_Pos)
#define UART_LCR_SP_Pos 		5		// Stick Parity
#define UART_LCR_SP_Msk 		(0x01 << UART_LCR_SP_Pos)
#define UART_LCR_SB_Pos 		6		// Set Break, TX forced into 0
#define UART_LCR_SB_Msk 		(0x01 << UART_LCR_SB_Pos)
#define UART_LCR_DLAB_Pos		7		// Divisor Latch Access Bit
#define UART_LCR_DLAB_Msk		(0x01 << UART_LCR_DLAB_Pos)

#define UART_LSR_RDA_Pos		0
#define UART_LSR_RDA_Msk		(0x01 << UART_LSR_RDA_Pos)
#define UART_LSR_ROV_Pos		1
#define UART_LSR_ROV_Msk		(0x01 << UART_LSR_ROV_Pos)
#define UART_LSR_PE_Pos			2		// Parity Error
#define UART_LSR_PE_Msk			(0x01 << UART_LSR_PE_Pos)
#define UART_LSR_FE_Pos			3		// Frame Error
#define UART_LSR_FE_Msk			(0x01 << UART_LSR_FE_Pos)
#define UART_LSR_BI_Pos			4		// Break Interrupt
#define UART_LSR_BI_Msk			(0x01 << UART_LSR_BI_Pos)
#define UART_LSR_THRE_Pos		5		// TX Holding Register Empty
#define UART_LSR_THRE_Msk		(0x01 << UART_LSR_THRE_Pos)
#define UART_LSR_TEMT_Pos		6		// THR Empty and TSR Empty
#define UART_LSR_TEMT_Msk		(0x01 << UART_LSR_TEMT_Pos)
#define UART_LSR_RFER_Pos		7		// RX FIFO Error
#define UART_LSR_RFER_Msk		(0x01 << UART_LSR_RFER_Pos)




typedef struct {
	__IO uint32_t CR0;
	__IO uint32_t CR1;
	__IO uint32_t CR2;
	__IO uint32_t CR3;
	__IO uint32_t CR4;
	     uint32_t RESERVED0[(0x830D0000 - 0x83008190)/4 -1];
	__IO uint32_t DR;
	__IO uint32_t IE;
	union {
		__IO uint32_t IF;
		__IO uint32_t FFCLR;
	};
	__IO uint32_t SRRDY;
	     uint32_t RESERVED1;
	__IO uint32_t LSR;
	     uint32_t RESERVED2[18];
	__IO uint32_t FFTRILVL;				// FIFO Trigger Level
} ADC_TypeDef;




typedef struct {
	__IO uint32_t GCR;					// global control
	     uint32_t RESERVED1[63];
	struct {
		__IO uint32_t CR;
		__IO uint32_t S0;				// State 0
		__IO uint32_t S1;
		     uint32_t RESERVED2;
	} CH[40];
} PWM_TypeDef;


#define PWM_GCR_START_Pos		0		// 置位时，所有置位global start en的通道全部一起启动
#define PWM_GCR_START_Msk		(0x01 << PWM_GCR_START_Pos)
#define PWM_GCR_CLKSRC_Pos		1		// 0 32KHz   1 2MHz   2 XTAL
#define PWM_GCR_CLKSRC_Msk		(0x03 << PWM_GCR_CLKSRC_Pos)
#define PWM_GCR_RESET_Pos		3
#define PWM_GCR_RESET_Msk		(0x01 << PWM_GCR_RESET_Pos)

#define PWM_CR_START_Pos		0
#define PWM_CR_START_Msk		(0x01 << PWM_CR_START_Pos)
#define PWM_CR_REPLAY_EN_Pos	1
#define PWM_CR_REPLAY_EN_Msk	(0x01 << PWM_CR_REPLAY_EN_Pos)
#define PWM_CR_POLARITY_Pos		2		// 0 active high   1 active low
#define PWM_CR_POLARITY_Msk		(0x01 << PWM_CR_POLARITY_Pos)
#define PWM_CR_OPEN_DRAIN_Pos	3
#define PWM_CR_OPEN_DRAIN_Msk	(0x01 << PWM_CR_OPEN_DRAIN_Pos)
#define PWM_CR_CLKEN_Pos		4
#define PWM_CR_CLKEN_Msk		(0x01 << PWM_CR_CLKEN_Pos)
#define PWM_CR_GLOBAL_START_EN_Pos 	5
#define PWM_CR_GLOBAL_START_EN_Msk 	(0x01 << PWM_CR_GLOBAL_START_EN_Pos)
#define PWM_CR_S0_STAY_CYCLE_Pos	8
#define PWM_CR_S0_STAY_CYCLE_Msk	(0xFFF<< PWM_CR_S0_STAY_CYCLE_Pos)
#define PWM_CR_S1_STAY_CYCLE_Pos	20
#define PWM_CR_S1_STAY_CYCLE_Msk	(0xFFF<< PWM_CR_S1_STAY_CYCLE_Pos)

#define PWM_S0_ON_TIME_Pos		0
#define PWM_S0_ON_TIME_Msk		(0xFFFF << PWM_S0_ON_TIME_Pos)
#define PWM_S0_OFF_TIME_Pos		16
#define PWM_S0_OFF_TIME_Msk		(0xFFFF << PWM_S0_OFF_TIME_Pos)

#define PWM_S1_ON_TIME_Pos		0
#define PWM_S1_ON_TIME_Msk		(0xFFFF << PWM_S1_ON_TIME_Pos)
#define PWM_S1_OFF_TIME_Pos		16
#define PWM_S1_OFF_TIME_Msk		(0xFFFF << PWM_S1_OFF_TIME_Pos)




typedef struct {
    __IO uint32_t CR;
         uint32_t RESERVED0[3];
    __IO uint32_t OLEN;                 // output data length
    __IO uint32_t ILEN;                 // input  data length
         uint32_t RESERVED1[(0x800-0x18)/4];
    __IO uint32_t DATA[40];
} SFC_TypeDef;

#define SFC_CR_WIP_Pos          0
#define SFC_CR_WIP_Msk          (0x01 << SFC_CR_WIP_Pos)
#define SFC_CR_WIPRdy_Pos       1
#define SFC_CR_WIPRdy_Msk       (0x01 << SFC_CR_WIPRdy_Pos)




typedef struct {
    __IO uint32_t CR;
    __IO uint32_t OP;
    __IO uint32_t HCNT0L;               // Cache hit count 0 lower part
    __IO uint32_t HCNT0U;               // Cache hit count 0 upper part
    __IO uint32_t CCNT0L;               // Cache access count 0 lower part
    __IO uint32_t CCNT0U;               // Cache access count 0 upper part
    __IO uint32_t HCNT1L;
    __IO uint32_t HCNT1U;
    __IO uint32_t CCNT1L;
    __IO uint32_t CCNT1U;
         uint32_t RESERVED0[1];
    __IO uint32_t REGION_EN;
         uint32_t RESERVED1[(0x10000 - 12*4) / 4];
    __IO uint32_t ENTRY_N[16];
    __IO uint32_t END_ENTRY_N[16];
} CACHE_TypeDef;


#define CACHE_CR_MCEN_Pos 		0       // MPU Cache Enable
#define CACHE_CR_MCEN_Msk 		(0x01<<CACHE_CR_MCEN_Pos)
#define CACHE_CR_CNTEN0_Pos 	2       // Cache hit counter 0 enable
#define CACHE_CR_CNTEN0_Msk 	(0x01<<CACHE_CR_CNTEN0_Pos)
#define CACHE_CR_CNTEN1_Pos 	3
#define CACHE_CR_CNTEN1_Msk 	(0x01<<CACHE_CR_CNTEN1_Pos)
#define CACHE_CR_MDRF_Pos 		7
#define CACHE_CR_MDRF_Msk 		(0x01<<CACHE_CR_MDRF_Pos)
#define CACHE_CR_CACHESIZE_Pos 	8       // 00 No Cache   01 8KB, 1-way cache   10 16KB, 2-way cache
#define CACHE_CR_CACHESIZE_Msk 	(0x03<<CACHE_CR_CACHESIZE_Pos)

#define CACHE_OP_EN_Pos 		0
#define CACHE_OP_EN_Msk 		(0x01<<CACHE_OP_EN_Pos)
#define CACHE_OP_OP_Pos 		1       // 0001 invalidate all cache lines   0010 invalidate one cache line using address   0100 invalidate one cache line using set/way
                                        // 1001 flush all cache lines        1010 flush one cache line using address        1100 flush one cache line using set/way
#define CACHE_OP_OP_Msk 		(0x0F<<CACHE_OP_OP_Pos)
#define CACHE_OP_TADDR_Pos 		5
#define CACHE_OP_TADDR_Msk 		(0x7FFFFFFUL<<CACHE_OP_TADDR_Pos)



/* AON */
#define TOP_CFG_AON_BASE            0x81021000
#define IOT_PINMUX_AON_BASE         0x81023000

/* CM4 AHB */
#define CM4_TCMROM_BASE             0x00000000
#define CM4_TCMRAM_BASE             0x00100000
#define CM4_XIPROM_BASE      		0x10000000
#define CM4_SYSRAM_BASE             0x20000000
#define CM4_CACHE_BASE         		0x01530000

/* CM4 APB2 */
#define CM4_CONFIG_BASE             0x83000000
#define CM4_TOPCFGAON_BASE          0x83008000
#define IOT_GPIO_AON_BASE           0x8300B000
#define CM4_CONFIG_AON_BASE         0x8300C000
#define CM4_UART1_BASE              0x83030000
#define CM4_UART2_BASE              0x83040000
#define CM4_TIMR_BASE               0x83050000
#define CM4_SFC_BASE                0x83070000  // Serial Flash Controller
#define CM4_ADC_BASE                0x830D0000
#define CM4_PWM_BASE				0x8300A600

#define TIMR	((TIMR_TypeDef *)   CM4_TIMR_BASE)

#define UART1   ((UART_TypeDef *)   CM4_UART1_BASE)
#define UART2   ((UART_TypeDef *)   CM4_UART2_BASE)

#define PWM		((PWM_TypeDef  *)   CM4_PWM_BASE)

#define SFC     ((SFC_TypeDef *)    CM4_SFC_BASE)

#define CACHE   ((CACHE_TypeDef *)  CM4_CACHE_BASE)


#include "MT7687_GPIO.h"
#include "MT7687_TIMR.h"
#include "MT7687_UART.h"
#include "MT7687_SPI.h"
#include "MT7687_I2C.h"
#include "MT7687_ADC.h"
#include "MT7687_PWM.h"
#include "MT7687_Flash.h"


#endif // __MT7687_H__
