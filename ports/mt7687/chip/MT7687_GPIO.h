#ifndef __MT7687_GPIO_H__
#define __MT7687_GPIO_H__


#define GPIO_DIR_IN		0
#define GPIO_DIR_OUT	1

#define GPIO_PULL_NO 	0
#define GPIO_PULL_PU	1
#define GPIO_PULL_DOWN	2
#define GPIO_OPEN_DRAIN	3


void PORT_Init(uint32_t pin, uint32_t func);

void GPIO_Init(uint32_t pin, uint32_t dir);

void GPIO_SetBit(uint32_t pin);
void GPIO_ClrBit(uint32_t pin);
void GPIO_InvBit(uint32_t pin);
uint32_t GPIO_GetBit(uint32_t pin);


#define GPIO0       0
#define GPIO1       1
#define GPIO2       2
#define GPIO3       3
#define GPIO4       4
#define GPIO5       5
#define GPIO6       6
#define GPIO7       7
#define GPIO24      24
#define GPIO25      25
#define GPIO26      26
#define GPIO27      27
#define GPIO28      28
#define GPIO29      29
#define GPIO30      30
#define GPIO31      31
#define GPIO32      32
#define GPIO33      33
#define GPIO34      34
#define GPIO35      35
#define GPIO36      36
#define GPIO37      37
#define GPIO38      38
#define GPIO39      39
#define GPIO57      57
#define GPIO58      58
#define GPIO59      59
#define GPIO60      60



#define GPIO0_EINT0              3
#define GPIO0_UART1_RTS          7
#define GPIO0_GPIO               8
#define GPIO0_PWM0               9

#define GPIO1_EINT1              3
#define GPIO1_UART1_CTS          7
#define GPIO1_GPIO               8
#define GPIO1_PWM1               9

#define GPIO2_UART1_RX           7
#define GPIO2_GPIO               8
#define GPIO2_PWM23              9

#define GPIO3_EINT2              3
#define GPIO3_UART1_TX           7
#define GPIO3_GPIO               8
#define GPIO3_PWM24              9

#define GPIO4_EINT3              3
#define GPIO4_GPIO               8
#define GPIO4_PWM2               9

#define GPIO5_EINT4              3
#define GPIO5_GPIO               8
#define GPIO5_PWM3               9

#define GPIO6_EINT5              3
#define GPIO6_SPIM_CS1           7
#define GPIO6_GPIO               8
#define GPIO6_PWM4               9

#define GPIO7_EINT6              3
#define GPIO7_SPIS_MISO          5
#define GPIO7_SPIM_CS0           6
#define GPIO7_GPIO               8
#define GPIO7_PWM5               9

#define GPIO24_I2C2_SCK          4
#define GPIO24_SPIS_MOSI         5
#define GPIO24_SPIM_MOSI         6
#define GPIO24_GPIO              8
#define GPIO24_PWM25             9

#define GPIO25_I2C2_SDA          4
#define GPIO25_SPIS_SCK          5
#define GPIO25_SPIM_MISO         6
#define GPIO25_GPIO              8
#define GPIO25_PWM26             9

#define GPIO26_I2S_TX            4
#define GPIO26_SPIS_CS0          5
#define GPIO26_SPIM_SCK          6
#define GPIO26_GPIO              8
#define GPIO26_PWM27             9

#define GPIO27_I2C1_SCK          4
#define GPIO27_GPIO              8
#define GPIO27_PWM28             9

#define GPIO28_I2C1_SDA          4
#define GPIO28_GPIO              8
#define GPIO28_PWM29             9

#define GPIO29_I2S_MCLK          4
#define GPIO29_SPIS_MOSI         6
#define GPIO29_SPIM_MOSI         7
#define GPIO29_GPIO              8
#define GPIO29_PWM30             9

#define GPIO30_I2S_FS            4
#define GPIO30_SPIS_MISO         6
#define GPIO30_SPIM_MISO         7
#define GPIO30_GPIO              8
#define GPIO30_PWM31             9

#define GPIO31_I2S_RX            4
#define GPIO31_I2S_TX            5
#define GPIO31_SPIS_SCK          6
#define GPIO31_SPIM_SCK          7
#define GPIO31_GPIO              8
#define GPIO31_PWM32             9

#define GPIO32_I2S_BCLK          4
#define GPIO32_SPIS_CS0          6
#define GPIO32_SPIM_CS0          7
#define GPIO32_GPIO              8
#define GPIO32_PWM33             9

#define GPIO33_IR_TX             7
#define GPIO33_GPIO              8
#define GPIO33_PWM34             9

#define GPIO34_IR_RX             7
#define GPIO34_GPIO              8
#define GPIO34_PWM35             9

#define GPIO35_EINT19            3
#define GPIO35_I2S_TX            5
#define GPIO35_GPIO              8
#define GPIO35_PWM18             9

#define GPIO36_UART2_RX          7
#define GPIO36_GPIO              8
#define GPIO36_PWM19             9

#define GPIO37_EINT20            3
#define GPIO37_UART2_TX          7
#define GPIO37_GPIO              8
#define GPIO37_PWM20             9

#define GPIO38_EINT21            3
#define GPIO38_UART2_RTS         7
#define GPIO38_GPIO              8
#define GPIO38_PWM21             9

#define GPIO39_EINT22            3
#define GPIO39_UART2_CTS         7
#define GPIO39_GPIO              8
#define GPIO39_PWM22             9

#define GPIO57_GPIO              8
#define GPIO57_PWM36             9

#define GPIO58_GPIO              8
#define GPIO58_PWM37             9

#define GPIO59_GPIO              8
#define GPIO59_PWM38             9

#define GPIO60_GPIO              8
#define GPIO60_PWM39             9


#endif // __MT7687_GPIO_H__
