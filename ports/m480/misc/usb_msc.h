#ifndef __USB_MSC_H__
#define __USB_MSC_H__


#define USBD_VID 	0x0416
#define USBD_PID 	0xB005


/* Define logic EP number */
#define MSC_BULK_IN_EP      1
#define MSC_BULK_OUT_EP     1

/* Define EP maximum packet size */
#define EP0_MAX_PKT_SIZE    64
#define EP1_MAX_PKT_SIZE    EP0_MAX_PKT_SIZE
#define EP2_MAX_PKT_SIZE    64      // MSC BULK IN
#define EP3_MAX_PKT_SIZE    64      // MSC BULK OUT

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


/* MSC Class Specific Request */
#define GET_MAX_LUN         0xFE
#define MSC_STORAGE_RESET   0xFF


/* MSC CBW/CSW Signature */
#define CBW_SIGNATURE       0x43425355
#define CSW_SIGNATURE       0x53425355


/* MSC UFI Command */
#define UFI_INQUIRY                         0x12
#define UFI_READ_CAPACITY                   0x25
#define UFI_READ_FORMAT_CAPACITY            0x23
#define UFI_READ_10                         0x28
#define UFI_READ_12                         0xA8
#define UFI_WRITE_10                        0x2A
#define UFI_WRITE_12                        0xAA
#define UFI_VERIFY_10                       0x2F
#define UFI_START_STOP                      0x1B
#define UFI_MODE_SENSE_6                    0x1A
#define UFI_MODE_SENSE_10                   0x5A
#define UFI_REQUEST_SENSE                   0x03
#define UFI_MODE_SELECT_6                   0x15
#define UFI_MODE_SELECT_10                  0x55
#define UFI_TEST_UNIT_READY                 0x00
#define UFI_PREVENT_ALLOW_MEDIUM_REMOVAL    0x1E


struct __attribute__((packed)) CBW  // Command Block Wrapper
{
    uint32_t dCBWSignature;
    uint32_t dCBWTag;
    uint32_t dCBWDataTransferLength;
    uint8_t  bmCBWFlags;
    uint8_t  bCBWLUN;
    uint8_t  bCBWCBLength;
    uint8_t  u8OPCode;
    uint8_t  u8LUN;
    uint8_t  au8Data[14];
};

struct __attribute__((packed)) CSW  // Command Status Wrapper
{
    uint32_t dCSWSignature;
    uint32_t dCSWTag;
    uint32_t dCSWDataResidue;
    uint8_t  bCSWStatus;
};


void usb_disk_init(void);


void MSC_Init(void);
void MSC_SetConfig(void);
void MSC_ClassRequest(void);

void MSC_ProcessCmd(void);


#endif
