#ifndef __MODS_PYBUSB_VCOM_H__
#define __MODS_PYBUSB_VCOM_H__

/* Define the vendor id and product id */
#define USBD_VID    0x0416
#define USBD_PID    0x50A1

/*!<Define CDC Class Specific Request */
#define SET_LINE_CODE           0x20
#define GET_LINE_CODE           0x21
#define SET_CONTROL_LINE_STATE  0x22

/*-------------------------------------------------------------*/
/* Define EP maximum packet size */
#define EP0_MAX_PKT_SIZE    64
#define EP1_MAX_PKT_SIZE    EP0_MAX_PKT_SIZE
#define EP2_MAX_PKT_SIZE    64
#define EP3_MAX_PKT_SIZE    64
#define EP4_MAX_PKT_SIZE    8

#define SETUP_BUF_BASE      0
#define SETUP_BUF_LEN       8
#define EP0_BUF_BASE        (SETUP_BUF_BASE + SETUP_BUF_LEN)
#define EP0_BUF_LEN         EP0_MAX_PKT_SIZE
#define EP1_BUF_BASE        (SETUP_BUF_BASE + SETUP_BUF_LEN)
#define EP1_BUF_LEN         EP1_MAX_PKT_SIZE
#define EP2_BUF_BASE        (EP1_BUF_BASE + EP1_BUF_LEN)
#define EP2_BUF_LEN         EP2_MAX_PKT_SIZE
#define EP3_BUF_BASE        (EP2_BUF_BASE + EP2_BUF_LEN)
#define EP3_BUF_LEN         EP3_MAX_PKT_SIZE
#define EP4_BUF_BASE        (EP3_BUF_BASE + EP3_BUF_LEN)
#define EP4_BUF_LEN         EP4_MAX_PKT_SIZE


/* Define the interrupt In EP number */
#define BULK_IN_EP_NUM      0x01
#define BULK_OUT_EP_NUM     0x02
#define INT_IN_EP_NUM       0x03


/* Define Descriptor information */
#define USBD_SELF_POWERED   0
#define USBD_REMOTE_WAKEUP  0
#define USBD_MAX_POWER      50  /* The unit is in 2mA. ex: 50 * 2mA = 100mA */

/************************************************/
/* for CDC class */
/* Line coding structure
  0-3 dwDTERate    Data terminal rate (baudrate), in bits per second
  4   bCharFormat  Stop bits: 0 - 1 Stop bit, 1 - 1.5 Stop bits, 2 - 2 Stop bits
  5   bParityType  Parity:    0 - None, 1 - Odd, 2 - Even, 3 - Mark, 4 - Space
  6   bDataBits    Data bits: 5, 6, 7, 8, 16  */
typedef struct
{
    uint32_t  u32DTERate;     /* Baud rate    */
    uint8_t   u8CharFormat;   /* stop bit     */
    uint8_t   u8ParityType;   /* parity       */
    uint8_t   u8DataBits;     /* data bits    */
} STR_VCOM_LINE_CODING;


#define RX_BUFF_SIZE    1024 	// RX buffer size
#define TX_BUFF_SIZE    1024 	// RX buffer size
#define TX_FIFO_SIZE	16  	// TX Hardware FIFO size

typedef struct {
	uint8_t  rx_buff[RX_BUFF_SIZE];
    uint16_t rx_write;  // Full ： rx_read == rx_write
    uint16_t rx_read;   // Empty： rx_write == rx_read - 1

	uint8_t  tx_buff[TX_BUFF_SIZE];
    uint16_t tx_write;
    uint16_t tx_read;

    uint16_t hw_flow;

	uint8_t  in_buff[64];
	uint16_t in_bytes;
	uint8_t  *out_ptr;
	uint16_t out_bytes;
} VCOM;


/*-------------------------------------------------------------*/
void VCOM_Init(void);
void VCOM_ClassRequest(void);

void EP2_Handler(void);
void EP3_Handler(void);
void VCOM_LineCoding(void);

uint USB_VCP_canSend(void);
uint USB_VCP_send(uint8_t *buffer, uint32_t len, uint32_t timeout);
void USB_VCP_sendPack(void);
uint USB_VCP_canRecv(void);
uint USB_VCP_recv(uint8_t *buffer, uint32_t len, uint32_t timeout);

#endif
