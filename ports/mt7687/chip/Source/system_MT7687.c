#include "MT7687.h"
#include "system_MT7687.h"

uint32_t SystemCoreClock = 192000000;
uint32_t SystemXTALClock = 40000000;


#define TOP_CFG_AON_CM4                     (CM4_CONFIG_BASE + 0x8000)


#define TOP_CFG_CM4_CKG_EN0                 *((volatile uint32_t *) (TOP_CFG_AON_CM4 + 0x01B0))

#define CM4_WBTAC_MCU_CK_SEL_SHIFT          14
#define CM4_WBTAC_MCU_CK_SEL_MASK           (0x03 << CM4_WBTAC_MCU_CK_SEL_SHIFT)
#define CM4_WBTAC_MCU_CK_SEL_XTAL           0
#define CM4_WBTAC_MCU_CK_SEL_WIFI_PLL_960   1
#define CM4_WBTAC_MCU_CK_SEL_WIFI_PLL_320   2
#define CM4_WBTAC_MCU_CK_SEL_BBP_PLL        3

#define CM4_MCU_DIV_SEL_SHIFT               4
#define CM4_MCU_DIV_SEL_MASK                (0x3F << CM4_MCU_DIV_SEL_SHIFT)

#define CM4_HCLK_SEL_SHIFT                  0
#define CM4_HCLK_SEL_MASK                   (0x07 << CM4_HCLK_SEL_SHIFT)
#define CM4_HCLK_SEL_OSC                    0
#define CM4_HCLK_SEL_32K                    1
#define CM4_HCLK_SEL_SYS_64M                2
#define CM4_HCLK_SEL_PLL                    4


#define TOP_CFG_CM4_PWR_CTL_CR              *((volatile uint32_t *) (TOP_CFG_AON_CM4 + 0x01B8))

#define CM4_MCU_960_EN_SHIFT                18
#define CM4_MCU_960_EN_MASK                 (0x01 << CM4_MCU_960_EN_SHIFT)
#define CM4_MPLL_EN_SHIFT                   28
#define CM4_MPLL_EN_MASK                    (0x03 << CM4_MPLL_EN_SHIFT)
#define CM4_MPLL_EN_PLL1_OFF_PLL2_OFF       0
#define CM4_MPLL_EN_PLL1_ON_PLL2_OFF        2
#define CM4_MPLL_EN_PLL1_ON_PLL2_ON         3
#define CM4_BT_PLL_RDY_SHIFT                27
#define CM4_BT_PLL_RDY_MASK                 (0x01 << CM4_BT_PLL_RDY_SHIFT)
#define CM4_WF_PLL_RDY_SHIFT                26
#define CM4_WF_PLL_RDY_MASK                 (0x01 << CM4_WF_PLL_RDY_SHIFT)
#define CM4_NEED_RESTORE_SHIFT              24
#define CM4_NEED_RESTORE_MASK               (0x01 << CM4_NEED_RESTORE_SHIFT)




#define TOP_AON_CM4_STRAP_STA		            *((volatile uint32_t *) (CM4_TOPCFGAON_BASE + 0x1C0))

#define TOP_AON_CM4_PWRCTLCR        			*((volatile uint32_t *) (CM4_TOPCFGAON_BASE + 0x1B8))
#define CM4_PWRCTLCR_CM4_XTAL_FREQ_OFFSET       (0)
#define CM4_PWRCTLCR_CM4_XTAL_FREQ_MASK         0x7F
#define CM4_PWRCTLCR_CM4_XTAL_FREQ_20M_OFFSET   (4)
#define CM4_PWRCTLCR_CM4_XTAL_FREQ_40M_OFFSET   (9)
#define CM4_PWRCTLCR_CM4_XTAL_FREQ_26M_OFFSET   (6)
#define CM4_PWRCTLCR_CM4_XTAL_FREQ_52M_OFFSET   (10)
#define CM4_PWRCTLCR_CM4_MCU_PWR_STAT_OFFSET    (11)
#define CM4_PWRCTLCR_CM4_MCU_PWR_STAT_MASK      BITS(11, 12)
#define CM4_PWRCTLCR_CM4_XO_NO_OFF_OFFSET       (15)
#define CM4_PWRCTLCR_CM4_FORCE_MCU_STOP_OFFSET  (16)
#define CM4_PWRCTLCR_CM4_MCU_960_EN_OFFSET      (18)
#define CM4_PWRCTLCR_CM4_HCLK_IRQ_B_OFFSET      (19)
#define CM4_PWRCTLCR_CM4_HCLK_IRQ_EN_OFFSET     (20)
#define CM4_PWRCTLCR_CM4_DSLP_IRQ_B_OFFSET      (21)
#define CM4_PWRCTLCR_CM4_DSLP_IRQ_EN_OFFSET     (22)
#define CM4_PWRCTLCR_CM4_DSLP_IRQ_EN_MASK       BITS(22, 23)
#define CM4_PWRCTLCR_CM4_NEED_RESTORE_OFFSET    (24)
#define CM4_PWRCTLCR_CM4_DSLP_MODE_OFFSET       (25)
#define CM4_PWRCTLCR_WF_PLL_RDY_OFFSET          (26)
#define CM4_PWRCTLCR_BT_PLL_RDY_OFFSET          (27)
#define CM4_PWRCTLCR_CM4_MPLL_EN_OFFSET         (28)
#define CM4_PWRCTLCR_CM4_MPLL_EN_MASK           BITS(28, 29)
#define CM4_PWRCTLCR_CM4_MPLL_BT_EN_OFFSET      (29)
#define CM4_PWRCTLCR_CM4_HW_CONTROL_OFFSET      (30)
#define CM4_PWRCTLCR_CM4_HWCTL_PWR_STAT_OFFSET  (31)


#define TOP_CFG_HCLK_2M_CKGEN           *((volatile uint32_t *) (CM4_CONFIG_BASE + 0x81B4))
#define SF_TOP_CLK_SEL_SHIFT            13
#define SF_TOP_CLK_SEL_MASK             (0x3 << SF_TOP_CLK_SEL_SHIFT)
#define SF_TOP_CLK_SEL_XTAL             0
#define SF_TOP_CLK_SEL_SYS_64M          1
#define SF_TOP_CLK_SEL_CM4_HCLK         2
#define SF_TOP_CLK_SEL_CM4_AON_32K      3


void SystemInit(void)
{
	uint32_t reg;

	/***************************************************************************
	*                             中断向量表重映射
	***************************************************************************/	
    SCB->VTOR  = 0x20000000;
	
	
	/***************************************************************************
	*                             系统时钟设置
	***************************************************************************/
    reg = TOP_AON_CM4_PWRCTLCR & (~CM4_PWRCTLCR_CM4_XTAL_FREQ_MASK);
    switch((TOP_AON_CM4_STRAP_STA >> 13) & 0x07)
	{
	case 0:
		SystemXTALClock = 20000000;
		reg |= 1 << CM4_PWRCTLCR_CM4_XTAL_FREQ_20M_OFFSET;
		break;
	case 1:
		SystemXTALClock = 40000000;
		reg |= 1 << CM4_PWRCTLCR_CM4_XTAL_FREQ_40M_OFFSET;
		break;
	case 2:
		SystemXTALClock = 26000000;
		reg |= 1 << CM4_PWRCTLCR_CM4_XTAL_FREQ_26M_OFFSET;
		break;
	case 3:
		SystemXTALClock = 52000000;
		reg |= 1 << CM4_PWRCTLCR_CM4_XTAL_FREQ_52M_OFFSET;
		break;
	case 4:
	case 5:
	case 6:
	case 7:
		SystemXTALClock = 40000000;
		reg |= 1 << CM4_PWRCTLCR_CM4_XTAL_FREQ_40M_OFFSET;
		break;
    }
    TOP_AON_CM4_PWRCTLCR = reg;
	
    // Step1. Power on PLL1 & 2
	TOP_CFG_CM4_PWR_CTL_CR = (TOP_CFG_CM4_PWR_CTL_CR & (~(CM4_MCU_960_EN_MASK | CM4_NEED_RESTORE_MASK))) | (0 << CM4_MCU_960_EN_SHIFT);

	TOP_CFG_CM4_PWR_CTL_CR = (TOP_CFG_CM4_PWR_CTL_CR & (~(CM4_MPLL_EN_MASK    | CM4_NEED_RESTORE_MASK))) | (CM4_MPLL_EN_PLL1_ON_PLL2_ON << CM4_MPLL_EN_SHIFT);
    
    while((TOP_CFG_CM4_PWR_CTL_CR & CM4_WF_PLL_RDY_MASK) == 0) __NOP();

    TOP_CFG_CM4_PWR_CTL_CR = (TOP_CFG_CM4_PWR_CTL_CR | (1 << CM4_MCU_960_EN_SHIFT));

    // Step2. CM4_RF_CLK_SW set to PLL2(960)
    TOP_CFG_CM4_CKG_EN0 = (TOP_CFG_CM4_CKG_EN0 & (~CM4_WBTAC_MCU_CK_SEL_MASK)) | (CM4_WBTAC_MCU_CK_SEL_WIFI_PLL_960 << CM4_WBTAC_MCU_CK_SEL_SHIFT);

    while((TOP_CFG_CM4_CKG_EN0 & CM4_WBTAC_MCU_CK_SEL_MASK) == 0) __NOP();

    // Step3. set divider to 1+8/2=5, ->  960/5=192Mhz
    TOP_CFG_CM4_CKG_EN0 = (TOP_CFG_CM4_CKG_EN0 & (~CM4_MCU_DIV_SEL_MASK)) | (8 << CM4_MCU_DIV_SEL_SHIFT);

    // Step4. CM4_HCLK_SW set to PLL_CK
    TOP_CFG_CM4_CKG_EN0 = (TOP_CFG_CM4_CKG_EN0 & (~CM4_HCLK_SEL_MASK)) | (CM4_HCLK_SEL_PLL << CM4_HCLK_SEL_SHIFT);
	
	
	/***************************************************************************
	*                Switch flash clock from XTAL to SYS 64Mhz
	***************************************************************************/
	TOP_CFG_HCLK_2M_CKGEN = (TOP_CFG_HCLK_2M_CKGEN & (~SF_TOP_CLK_SEL_MASK)) | (SF_TOP_CLK_SEL_SYS_64M << SF_TOP_CLK_SEL_SHIFT);
}

void SystemCoreClockUpdate(void)
{
    
}

void CachePreInit(void)
{
    CACHE->CR = 0x00;	// CACHE disable

    CACHE->OP = 0x13;	// Flush all cache lines

    CACHE->OP = 0x03;	// Invalidate all cache lines

    /* Set cacheable region */
    CACHE->ENTRY_N[0]     = 0x10079100;
    CACHE->END_ENTRY_N[0] = 0x10200000;

    CACHE->REGION_EN = 1;

    CACHE->CR = 0x30D;	// CACHE enable, 64K TCM/32K CACHE
}
