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
#include <stdbool.h>
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
    __IO uint32_t CSR;
    __IO uint32_t OAR;                  // opcode/address register
    __IO uint32_t DR[8];
    __IO uint32_t CR1;
    __IO uint32_t CR2;
         uint32_t RESERVED;
    __IO uint32_t IF;                   // set when transaction complete, clear by reading
} SPI_TypeDef;


#define SPI_CSR_START_Pos       8
#define SPI_CSR_START_Msk       (0x01 << SPI_CSR_START_Pos)
#define SPI_CSR_BUSY_Pos        16
#define SPI_CSR_BUSY_Msk        (0x01 << SPI_CSR_BUSY_Pos)

#define SPI_CR1_MODE_Pos        2       // must be 1
#define SPI_CR1_MODE_Msk        (0x01 << SPI_CR1_MODE_Pos)
#define SPI_CR1_LSBF_Pos        3
#define SPI_CR1_LSBF_Msk        (0x01 << SPI_CR1_LSBF_Pos)
#define SPI_CR1_CPOL_Pos        4
#define SPI_CR1_CPOL_Msk        (0x01 << SPI_CR1_CPOL_Pos)
#define SPI_CR1_CPHA_Pos        5
#define SPI_CR1_CPHA_Msk        (0x01 << SPI_CR1_CPHA_Pos)
#define SPI_CR1_TSTA_Pos        8       // interval between cs assert and sclk, 0 3-spiclk   1 6-spiclk
#define SPI_CR1_TSTA_Msk        (0x01 << SPI_CR1_TSTA_Pos)
#define SPI_CR1_IE_Pos          9
#define SPI_CR1_IE_Msk          (0x01 << SPI_CR1_IE_Pos)
#define SPI_CR1_DUPLEX_Pos      10      // 0 half duplex   1 full duplex
#define SPI_CR1_DUPLEX_Msk      (0x01 << SPI_CR1_DUPLEX_Pos)
#define SPI_CR1_TSTO_Pos        11      // interval between sclk and cs deassert
#define SPI_CR1_TSTO_Msk        (0x1F << SPI_CR1_TSTO_Pos)
#define SPI_CR1_CLKDIV_Pos      16      // 0 HCLK/2   1 HCLK/3 (33% or 66% duty cycle)   ...   4095 HCLK/4097
#define SPI_CR1_CLKDIV_Msk      (0xFFF<< SPI_CR1_CLKDIV_Pos)
#define SPI_CR1_CLKDUTY_Pos     28      // 1 period of SCLK HIGH is longer when CLKDIV is odd
#define SPI_CR1_CLKDUTY_Msk     (0x01 << SPI_CR1_CLKDUTY_Pos)
#define SPI_CR1_SLVSEL_Pos      29      // slave select, 0 slave#0 select   1 slave#1 slect   ...
#define SPI_CR1_SLVSEL_Msk      (0x07u<< SPI_CR1_SLVSEL_Pos)

#define SPI_CR2_MOBIT_Pos       0       // MOSI bit count
#define SPI_CR2_MOBIT_Msk       (0xFF << SPI_CR2_MOBIT_Pos)
#define SPI_CR2_MIBIT_Pos       12      // MOSI bit count
#define SPI_CR2_MIBIT_Msk       (0xFF << SPI_CR2_MIBIT_Pos)
#define SPI_CR2_CMDBIT_Pos      24
#define SPI_CR2_CMDBIT_Msk      (0x3F << SPI_CR2_CMDBIT_Pos)




typedef struct {
    __IO uint32_t PADCR;
    __IO uint32_t TLOW;                 // Time for SCL low  period
    __IO uint32_t THIGH;                // Time for SCL high period
         uint32_t RESERVED1[2];
    __IO uint32_t RDCNT;                // [15:0] read data count
         uint32_t RESERVED2[2];
    __IO uint32_t SLVADDR;              // [ 6:0] slave address
         uint32_t RESERVED3;
    __IO uint32_t PACKCR;
    __IO uint32_t ACK;
    __IO uint32_t CR;
    __IO uint32_t SR;
    __IO uint32_t FIFOCLR;
    __IO uint32_t FIFODR;               // FIFO Depth: 8 bytes
    __IO uint32_t FIFOSR;
    __IO uint32_t FIFOPTR;
} I2C_TypeDef;


#define I2C_PADCR_DECNT_Pos     0       // Deglitch counting number
#define I2C_PADCR_DECNT_Msk     (0x1F << I2C_PADCR_DECNT_Pos)
#define I2C_PADCR_SCLDRVH_Pos   5       // 1 SCL drive high   0 SCL pull high
#define I2C_PADCR_SCLDRVH_Msk   (0x01 << I2C_PADCR_SCLDRVH_Pos)
#define I2C_PADCR_SDADRVH_Pos   6
#define I2C_PADCR_SDADRVH_Msk   (0x01 << I2C_PADCR_SDADRVH_Pos)
#define I2C_PADCR_SYNCEN_Pos    7       // 1 enable internal sync function on SCL/SDA, including 2-cycles of latency
#define I2C_PADCR_SYNCEN_Msk    (0x01 << I2C_PADCR_SYNCEN_Pos)

#define I2C_TLOW_SETUP_Pos      0       // SDA to SCL rising edge timing. The data setup time
#define I2C_TLOW_SETUP_Msk      (0xFF << I2C_TLOW_SETUP_Pos)
#define I2C_TLOW_STOP_Pos       8       // SCL rising edge to SDA timing. The STOP condition period
#define I2C_TLOW_STOP_Msk       (0xFF << I2C_TLOW_STOP_Pos)

#define I2C_THIGH_START_Pos     0       // SDA to SCL falling edge timing. The START condition period
#define I2C_THIGH_START_Msk     (0xFF << I2C_THIGH_START_Pos)
#define I2C_THIGH_HOLD_Pos      8       // SCL falling edge to SDA timing. The data hold time
#define I2C_THIGH_HOLD_Msk      (0xFF << I2C_THIGH_HOLD_Pos)

#define I2C_PACKCR_DIR_Pos      0       // direction for packet#0 in general mode and normal mode, 0 write   1 read
#define I2C_PACKCR_DIR_Msk      (0x01 << I2C_PACKCR_DIR_Pos)
#define I2C_PACKCR_DIR1_Pos     1       // direction for packet#1 in general mode
#define I2C_PACKCR_DIR1_Msk     (0x01 << I2C_PACKCR_DIR1_Pos)
#define I2C_PACKCR_DIR2_Pos     2       // direction for packet#2 in general mode
#define I2C_PACKCR_DIR2_Msk     (0x01 << I2C_PACKCR_DIR2_Pos)
#define I2C_PACKCR_DIR3_Pos     3       // direction for packet#3 in general mode
#define I2C_PACKCR_DIR3_Msk     (0x01 << I2C_PACKCR_DIR3_Pos)
#define I2C_PACKCR_CNT_Pos      4       // number of packet in general mode
#define I2C_PACKCR_CNT_Msk      (0x03 << I2C_PACKCR_CNT_Pos)

#define I2C_ACK_DATA_Pos        0       // The last 8 received ACK
#define I2C_ACK_DATA_Msk        (0xFF << I2C_ACK_DATA_Pos)
#define I2C_ACK_ADDR_Pos        8       // The received ACK after ADDR byte for packet#0 in general mode and normal mode
#define I2C_ACK_ADDR_Msk        (0x01 << I2C_ACK_ADDR_Pos)
#define I2C_ACK_ADDR1_Pos       9       // The received ACK after ADDR byte for packet#1 in general mode
#define I2C_ACK_ADDR1_Msk       (0x01 << I2C_ACK_ADDR1_Pos)
#define I2C_ACK_ADDR2_Pos       10      // The received ACK after ADDR byte for packet#2 in general mode
#define I2C_ACK_ADDR2_Msk       (0x01 << I2C_ACK_ADDR2_Pos)
#define I2C_ACK_ADDR3_Pos       11      // The received ACK after ADDR byte for packet#3 in general mode
#define I2C_ACK_ADDR3_Msk       (0x01 << I2C_ACK_ADDR3_Pos)

#define I2C_CR_START_Pos        0       // clear when data starts to transfer
#define I2C_CR_START_Msk        (0x01 << I2C_CR_START_Pos)
#define I2C_CR_GMODE_Pos        14      // 0 normal mode   1 general mode
#define I2C_CR_GMODE_Msk        (0x01 << I2C_CR_GMODE_Pos)
#define I2C_CR_MASTER_Pos       15
#define I2C_CR_MASTER_Msk       (0x01 << I2C_CR_MASTER_Pos)

#define I2C_SR_BUSBUSY_Pos      0
#define I2C_SR_BUSBUSY_Msk      (0x01 << I2C_SR_BUSBUSY_Pos)
#define I2C_SR_ARBLOST_Pos      1       // arbitration lost, clear by writing 1
#define I2C_SR_ARBLOST_Msk      (0x01 << I2C_SR_ARBLOST_Pos)
#define I2C_SR_IDLE_Pos         2       // 1 ready for new transaction
#define I2C_SR_IDLE_Msk         (0x01 << I2C_SR_IDLE_Pos)

#define I2C_FIFOCLR_RX_Pos      0
#define I2C_FIFOCLR_RX_Msk      (0x01 << I2C_FIFOCLR_RX_Pos)
#define I2C_FIFOCLR_TX_Pos      1
#define I2C_FIFOCLR_TX_Msk      (0x01 << I2C_FIFOCLR_TX_Pos)

#define I2C_FIFOSR_RXEMP_Pos    0       // RX FIFO empty
#define I2C_FIFOSR_RXEMP_Msk    (0x01 << I2C_FIFOSR_RXEMP_Pos)
#define I2C_FIFOSR_RXFULL_Pos   1       // RX FIFO full
#define I2C_FIFOSR_RXFULL_Msk   (0x01 << I2C_FIFOSR_RXFULL_Pos)
#define I2C_FIFOSR_RXUNDR_Pos   2       // RX FIFO underrun
#define I2C_FIFOSR_RXUNDR_Msk   (0x01 << I2C_FIFOSR_RXUNDR_Pos)
#define I2C_FIFOSR_RXOVF_Pos    3       // RX FIFO overflow
#define I2C_FIFOSR_RXOVF_Msk    (0x01 << I2C_FIFOSR_RXOVF_Pos)
#define I2C_FIFOSR_TXEMP_Pos    4       // TX FIFO empty
#define I2C_FIFOSR_TXEMP_Msk    (0x01 << I2C_FIFOSR_TXEMP_Pos)
#define I2C_FIFOSR_TXFULL_Pos   5       // TX FIFO full
#define I2C_FIFOSR_TXFULL_Msk   (0x01 << I2C_FIFOSR_TXFULL_Pos)
#define I2C_FIFOSR_TXUNDR_Pos   6       // TX FIFO underrun
#define I2C_FIFOSR_TXUNDR_Msk   (0x01 << I2C_FIFOSR_TXUNDR_Pos)
#define I2C_FIFOSR_TXOVF_Pos    7       // TX FIFO overflow
#define I2C_FIFOSR_TXOVF_Msk    (0x01 << I2C_FIFOSR_TXOVF_Pos)

#define I2C_FIFOPTR_RXRD_Pos    0       // RX FIFO read pointer
#define I2C_FIFOPTR_RXRD_Msk    (0x0F << I2C_FIFOPTR_RXRD_Pos)
#define I2C_FIFOPTR_RXWR_Pos    4       // RX FIFO write pointer
#define I2C_FIFOPTR_RXWR_Msk    (0x0F << I2C_FIFOPTR_RXWR_Pos)
#define I2C_FIFOPTR_TXRD_Pos    8       // TX FIFO read pointer
#define I2C_FIFOPTR_TXRD_Msk    (0x0F << I2C_FIFOPTR_TXRD_Pos)
#define I2C_FIFOPTR_TXWR_Pos    12      // TX FIFO write pointer
#define I2C_FIFOPTR_TXWR_Msk    (0x0F << I2C_FIFOPTR_TXWR_Pos)




typedef struct {
	__IO uint32_t CR0;
	__IO uint32_t CR1;					// clock cycle count in periodic mode
	__IO uint32_t CR2;
	__IO uint32_t CR3;
	__IO uint32_t CR4;
         uint32_t RESERVED[(0x830D0000 - 0x83008190)/4 -1];
	__IO uint32_t DR;
	__IO uint32_t IE;					// 1 interrupt will be generated if the RXFIFO contains data or time-out
	union {
		__IO uint32_t IF;				// 0x4 RX data received   0xC RX data time-out
		__IO uint32_t FFCLR;			// 0x2 Clear all bytes in RX FIFO
	};
	__IO uint32_t SYNC;					// SYNC[7] = 1: DR and IE not readable/writable
										// SYNC = 0xBF: DR、IE、IF、LSB not readable/writable
         uint32_t RESERVED2;
	__IO uint32_t LSR;					// 1 set by RXFIFO becoming full   0 cleared by reading RXFIFO
         uint32_t RESERVED3[15];
    __IO uint32_t RTOCNT;				// RX Timeout Count
    	 uint32_t RESERVED4[2];
	__IO uint32_t FFTRG;				// RXFIFO Trigger
         uint32_t RESERVED5[32];
    __IO uint32_t RXPTR;                // RXFIFO Pointer
} ADC_TypeDef;


#define ADC_CR0_EN_Pos			0
#define ADC_CR0_EN_Msk			(0x01 << ADC_CR0_EN_Pos)
#define ADC_CR0_AVG_Pos			1
#define ADC_CR0_AVG_Msk			(0x07 << ADC_CR0_AVG_Pos)
#define ADC_CR0_TCH_Pos			4		// channel stable time
#define ADC_CR0_TCH_Msk			(0x0F << ADC_CR0_TCH_Pos)
#define ADC_CR0_MODE_Pos		8		// 0 one-time mode   1 periodic mode
#define ADC_CR0_MODE_Msk		(0x01 << ADC_CR0_MODE_Pos)
#define ADC_CR0_TINIT_Pos		9		// initial stable time
#define ADC_CR0_TINIT_Msk		(0x01 << ADC_CR0_TINIT_Pos)
#define ADC_CR0_CHEN_Pos		16		// channel enable
#define ADC_CR0_CHEN_Msk		(0x0F << ADC_CR0_CHEN_Pos)

#define ADC_CR2_CMPIE_Pos		0		// compare interrupt enable
#define ADC_CR2_CMPIE_Msk		(0x0F << ADC_CR2_CMPIE_Pos)
#define ADC_CR2_CMPVAL_Pos		4		// when adc result > CMPVAL, interrupt assert
#define ADC_CR2_CMPVAL_Msk		(0xFFF<< ADC_CR2_CMPVAL_Pos)
#define ADC_CR2_CMPIF_Pos		16		// compare interrupt flag
#define ADC_CR2_CMPIF_Msk		(0x0F << ADC_CR2_CMPIF_Pos)

#define ADC_DR_VALUE_Pos        0
#define ADC_DR_VALUE_Msk        (0xFFF<< ADC_DR_VALUE_Pos)
#define ADC_DR_CHNUM_Pos        12
#define ADC_DR_CHNUM_Msk        (0x0F << ADC_DR_CHNUM_Pos)

#define ADC_FFTRG_LVL_Pos		3		// FIFO Trigger Level, interrupt will be set if the data in RXFIFO is more than FFTRG.LVL
#define ADC_FFTRG_LVL_Msk		(0x0F << ADC_FFTRG_LVL_Pos)

#define ADC_RXPTR_RD_Pos        0       // RXFIFO Read Pointer
#define ADC_RXPTR_RD_Msk        (0x0F << ADC_RXPTR_RD_Pos)
#define ADC_RXPTR_WR_Pos        4       // RXFIFO Write Pointer
#define ADC_RXPTR_WR_Msk        (0x0F << ADC_RXPTR_WR_Pos)




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




typedef struct {
         uint32_t RESERVED;
    __IO uint32_t PWRCHK1;
    __IO uint32_t PWRCHK2;
    __IO uint32_t KEY;
    __IO uint32_t PROT1;
    __IO uint32_t PROT2;
    __IO uint32_t PROT3;
    __IO uint32_t PROT4;
    __IO uint32_t CR;
    __IO uint32_t RESERVED1;
    __IO uint32_t XOSCCR;
    __IO uint32_t DEBNCE;               // This duration prevent abnormal write during losing core power
    __IO uint32_t PMUEN;
    __IO uint32_t PADCR;
         uint32_t RESERVED2;
    __IO uint32_t WAVEOUT;
    __IO uint32_t TC_YEA;               // 0 - 99
    __IO uint32_t TC_MON;               // 1 - 12
    __IO uint32_t TC_DAY;               // 1 - 31
    __IO uint32_t TC_DOW;               // 0 - 6
    __IO uint32_t TC_HOU;               // 0 - 23
    __IO uint32_t TC_MIN;               // 0 - 59
    __IO uint32_t TC_SEC;               // 0 - 59
         uint32_t RESERVED3;
    __IO uint32_t AL_YEA;
    __IO uint32_t AL_MON;
    __IO uint32_t AL_DAY;
    __IO uint32_t AL_DOW;
    __IO uint32_t AL_HOU;
    __IO uint32_t AL_MIN;
    __IO uint32_t AL_SEC;
    __IO uint32_t ALMCR;
    __IO uint32_t RIPCR;
    __IO uint32_t RIP_CNTH;
    __IO uint32_t RIP_CNTL;
         uint32_t RESERVED4;
    __IO uint32_t TMRCR;
    __IO uint32_t TMR_CNTH;
    __IO uint32_t TMR_CNTL;
         uint32_t RESERVED5[9];
    __IO uint32_t SPARE[16];
    __IO uint32_t COREPDN;              // Core Powerdown
         uint32_t RESERVED6[15];
    __IO uint32_t BACKUP[36];
} RTC_TypeDef;


#define RTC_CR_RCSTOP_Pos		0		// ripple counter stop
#define RTC_CR_RCSTOP_Msk		(0x01 << RTC_CR_RCSTOP_Pos)
#define RTC_CR_PWRPASS_Pos		2		// pass RTC power stable check
#define RTC_CR_PWRPASS_Msk		(0x01 << RTC_CR_PWRPASS_Pos)
#define RTC_CR_KEYPASS_Pos		3		// pass RTC write protection
#define RTC_CR_KEYPASS_Msk		(0x01 << RTC_CR_KEYPASS_Pos)
#define RTC_CR_PROTPASS_Pos		4		// pass RTC security check protection
#define RTC_CR_PROTPASS_Msk		(0x01 << RTC_CR_PROTPASS_Pos)
#define RTC_CR_SYNCBUSY_Pos		6		// 1 Sync busy, cannot read / write TC_xx and TMR_xx register
#define RTC_CR_SYNCBUSY_Msk		(0x01 << RTC_CR_SYNCBUSY_Pos)
#define RTC_CR_DEBNCEOK_Pos		7		// 0 RTC is still de-bouncing
#define RTC_CR_DEBNCEOK_Msk		(0x01 << RTC_CR_DEBNCEOK_Pos)

#define RTC_XOSCCR_CALI_Pos		0		// 32k PAD drive strength
#define RTC_XOSCCR_CALI_Msk		(0x0F << RTC_XOSCCR_CALI_Pos)
#define RTC_XOSCCR_AMPGAIN_Pos	4		// Amplitude gain
#define RTC_XOSCCR_AMPGAIN_Msk	(0x01 << RTC_XOSCCR_AMPGAIN_Pos)
#define RTC_XOSCCR_AMPEN_Pos	5		// Amplitude enable
#define RTC_XOSCCR_AMPEN_Msk	(0x01 << RTC_XOSCCR_AMPEN_Pos)
#define RTC_XOSCCR_PDN_Pos		7		// 32k power down
#define RTC_XOSCCR_PDN_Msk		(0x01 << RTC_XOSCCR_PDN_Pos)

#define RTC_PMUEN_EN_Pos        0       // write 1 to enable both PMU_EN and PMU_EN_EXT at the same time
                                        // write 0 to disable PMU_EN, and PMU_EN_EXT will be disabled afterwards
#define RTC_PMUEN_EN_Msk        (0x01 << RTC_PMUEN_EN_Pos)
#define RTC_PMUEN_ENEXT_Pos     1       //
#define RTC_PMUEN_ENEXT_Msk     (0x01 << RTC_PMUEN_ENEXT_Pos)
#define RTC_PMUEN_ENSTAT_Pos    2       // 11 PMU is enabled   10 PMU is about to be disabled   00 PMU is disabled
#define RTC_PMUEN_ENSTAT_Msk    (0x03 << RTC_PMUEN_ENSTAT_Pos)
#define RTC_PMUEN_ALMIF_Pos     4       // Interrupt flag of alarm, write 1 to clear
#define RTC_PMUEN_ALMIF_Msk     (0x01 << RTC_PMUEN_ALMIF_Pos)
#define RTC_PMUEN_TMRIF_Pos     5       // Interrupt flag of timer, write 1 to clear
#define RTC_PMUEN_TMRIF_Msk     (0x01 << RTC_PMUEN_TMRIF_Pos)

#define RTC_WAVEOUT_SEL_Pos     0       // 0 ripple counter[14]   1 ripper counter[13]   ...   14 ripple counter[0]   15 32k clock
#define RTC_WAVEOUT_SEL_Msk     (0x0F << RTC_WAVEOUT_SEL_Pos)
#define RTC_WAVEOUT_EN_Pos      4
#define RTC_WAVEOUT_EN_Msk      (0x01 << RTC_WAVEOUT_EN_Pos)

#define RTC_ALMCR_EN_Pos        0       // alarm enable
#define RTC_ALMCR_EN_Msk        (0x01 << RTC_ALMCR_EN_Pos)
#define RTC_ALMCR_SEC_Pos       1       // 1 compare second
#define RTC_ALMCR_SEC_Msk       (0x01 << RTC_ALMCR_SEC_Pos)
#define RTC_ALMCR_MIN_Pos       2
#define RTC_ALMCR_MIN_Msk       (0x01 << RTC_ALMCR_MIN_Pos)
#define RTC_ALMCR_HOU_Pos       3
#define RTC_ALMCR_HOU_Msk       (0x01 << RTC_ALMCR_HOU_Pos)
#define RTC_ALMCR_DOW_Pos       4
#define RTC_ALMCR_DOW_Msk       (0x01 << RTC_ALMCR_DOW_Pos)
#define RTC_ALMCR_DAY_Pos       5
#define RTC_ALMCR_DAY_Msk       (0x01 << RTC_ALMCR_DAY_Pos)
#define RTC_ALMCR_MON_Pos       6
#define RTC_ALMCR_MON_Msk       (0x01 << RTC_ALMCR_MON_Pos)
#define RTC_ALMCR_YEA_Pos       7
#define RTC_ALMCR_YEA_Msk       (0x01 << RTC_ALMCR_YEA_Pos)

#define RTC_RIPCR_RDTRG_Pos     0       // read trigger, 1 trigger read ripple counter   0 clear RDOK bit
#define RTC_RIPCR_RDTRG_Msk     (0x01 << RTC_RIPCR_RDTRG_Pos)
#define RTC_RIPCR_RDOK_Pos      1       // read ok, 1 RIP_CNT is ready for read
#define RTC_RIPCR_RDOK_Msk      (0x01 << RTC_RIPCR_RDOK_Pos)

#define RTC_TMRCR_IE_Pos        1       // 1 start to count down TMR_CNT
#define RTC_TMRCR_IE_Msk        (0x01 << RTC_TMRCR_IE_Pos)

#define RTC_COREPDN_SHUT_Pos    0       // Before core power shut down, write this bit to disable RTC domain interface
#define RTC_COREPDN_SHUT_Msk    (0x01 << RTC_COREPDN_SHUT_Pos)
#define RTC_COREPDN_RTCWEN_Pos  1       // 1 CPU can write RTC domain register
#define RTC_COREPDN_RTCWEN_Msk  (0x01 << RTC_COREPDN_RTCWEN_Pos)




typedef struct {
    __IO uint32_t MODE;
    __IO uint32_t INTT;                 // Interrupt Time
    __IO uint32_t FEED;                 // restart the WDT time-out counter if KEY=0x1971
    __IO uint32_t STAT;
    __IO uint32_t RSTT;                 // Reset Time
    __IO uint32_t SWTRG;                // Software Trigger, KEY=0x1209
} WDT_TypeDef;


#define WDT_MODE_EN_Pos         0
#define WDT_MODE_EN_Msk         (0x01 << WDT_MODE_EN_Pos)
#define WDT_MODE_INT_Pos        3       // 0 reset mode   1 interrupt mode
#define WDT_MODE_INT_Msk        (0x01 << WDT_MODE_INT_Pos)
#define WDT_MODE_KEY_Pos        8       // write access is allowed if KEY=0x22
#define WDT_MODE_KEY_Msk        (0xFF << WDT_MODE_KEY_Pos)

#define WDT_INTT_KEY_Pos        0       // write access is allowed if KEY=0x08
#define WDT_INTT_KEY_Msk        (0x1F << WDT_INTT_KEY_Pos)
#define WDT_INTT_TIME_Pos       5       // unit is T32k * 1024 = 32ms
#define WDT_INTT_TIME_Msk       (0x7FF<< WDT_INTT_TIME_Pos)

#define WDT_STAT_SWTRG_Pos      14      // 1 WDT reset/interrupt due to software-triggered
#define WDT_STAT_SWTRG_Msk      (0x01 << WDT_STAT_SWTRG_Pos)
#define WDT_STAT_TRG_Pos        15      // 1 WDT reset/interrupt due to hardware timer time-out
#define WDT_STAT_TRG_Msk        (0x01 << WDT_STAT_TRG_Pos)





/* AON */
#define TOP_CFG_AON_BASE            0x81021000
#define IOT_PINMUX_AON_BASE         0x81023000

/* CM4 AHB */
#define CM4_TCMROM_BASE             0x00000000
#define CM4_TCMRAM_BASE             0x00100000
#define CM4_XIPROM_BASE      		0x10000000
#define CM4_SYSRAM_BASE             0x20000000
#define CM4_CACHE_BASE         		0x01530000
#define CM4_SPI_BASE                0x24000000

/* CM4 APB2 */
#define CM4_CONFIG_BASE             0x83000000
#define CM4_TOPCFGAON_BASE          0x83008000
#define IOT_GPIO_AON_BASE           0x8300B000
#define CM4_CONFIG_AON_BASE         0x8300C000
#define CM4_UART1_BASE              0x83030000
#define CM4_UART2_BASE              0x83040000
#define CM4_TIMR_BASE               0x83050000
#define CM4_SFC_BASE                0x83070000  // Serial Flash Controller
#define CM4_I2C1_BASE               0x83090240
#define CM4_I2C2_BASE               0x830A0240
#define CM4_ADC_BASE                0x830D0000
#define CM4_PWM_BASE				0x8300A600
#define CM4_RTC_BASE                0x830C0000
#define CM4_WDT_BASE                0x83080030



#define TIMR	((TIMR_TypeDef *)   CM4_TIMR_BASE)

#define UART1   ((UART_TypeDef *)   CM4_UART1_BASE)
#define UART2   ((UART_TypeDef *)   CM4_UART2_BASE)

#define SPI     ((SPI_TypeDef  *)   CM4_SPI_BASE)

#define I2C1    ((I2C_TypeDef  *)   CM4_I2C1_BASE)
#define I2C2    ((I2C_TypeDef  *)   CM4_I2C2_BASE)

#define ADC     ((ADC_TypeDef  *)   CM4_ADC_BASE)

#define PWM		((PWM_TypeDef  *)   CM4_PWM_BASE)

#define SFC     ((SFC_TypeDef  *)   CM4_SFC_BASE)

#define CACHE   ((CACHE_TypeDef*)   CM4_CACHE_BASE)

#define RTC     ((RTC_TypeDef  *)   CM4_RTC_BASE)

#define WDT     ((WDT_TypeDef  *)   CM4_WDT_BASE)


#include "MT7687_GPIO.h"
#include "MT7687_TIMR.h"
#include "MT7687_UART.h"
#include "MT7687_SPI.h"
#include "MT7687_I2C.h"
#include "MT7687_ADC.h"
#include "MT7687_PWM.h"
#include "MT7687_Flash.h"
#include "MT7687_RTC.h"
#include "MT7687_WDT.h"


#endif // __MT7687_H__
