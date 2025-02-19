#include "chip/M480.h"

#include "misc/usb_msc.h"


uint8_t gu8DeviceDescriptor[] =
{
    LEN_DEVICE,         // bLength
    DESC_DEVICE,        // bDescriptorType
    0x00, 0x02,         // bcdUSB
    0x00,               // bDeviceClass
    0x00,               // bDeviceSubClass
    0x00,               // bDeviceProtocol
    EP0_MAX_PKT_SIZE,   // bMaxPacketSize0
    USBD_VID & 0xFF, USBD_VID >> 8,		// idVendor
    USBD_PID & 0xFF, USBD_PID >> 8,		// idProduct
    0x00, 0x01,        // bcdDevice
    0x01,              // iManufacture
    0x02,              // iProduct
    0x03,              // iSerialNumber -  is required for BOT device
    0x01               // bNumConfigurations
};


uint8_t gu8ConfigDescriptor[] =
{
    LEN_CONFIG,         // bLength
    DESC_CONFIG,        // bDescriptorType
#define TOTAL_LEN  (LEN_CONFIG + (LEN_INTERFACE + LEN_ENDPOINT * 2) + (8 + LEN_INTERFACE + 5 + 5 + 4 + 5 + LEN_ENDPOINT + LEN_INTERFACE + LEN_ENDPOINT + LEN_ENDPOINT))
    TOTAL_LEN & 0xFF, TOTAL_LEN >> 8,	// wTotalLength
    0x03,               // bNumInterfaces
    0x01,               // bConfigurationValue
    0x00,               // iConfiguration
    0xC0,               // bmAttributes, D6: self power  D5: remote wake-up
    0x64,               // MaxPower, 100 * 2mA = 200mA

    // I/F descriptor: MSC
    LEN_INTERFACE,  	// bLength
    DESC_INTERFACE, 	// bDescriptorType
    0x00,           	// bInterfaceNumber
    0x00,           	// bAlternateSetting
    0x02,           	// bNumEndpoints
    0x08,           	// bInterfaceClass
    0x06,           	// bInterfaceSubClass
    0x50,           	// bInterfaceProtocol
    0x00,           	// iInterface

    LEN_ENDPOINT,           		// bLength
    DESC_ENDPOINT,          		// bDescriptorType
    (EP_INPUT | MSC_BULK_IN_EP),	// bEndpointAddress
    EP_BULK,                		// bmAttributes
    EP2_MAX_PKT_SIZE, 0x00, 		// wMaxPacketSize
    0x00,                   		// bInterval

    LEN_ENDPOINT,           		// bLength
    DESC_ENDPOINT,          		// bDescriptorType
    (EP_OUTPUT | MSC_BULK_OUT_EP),  // bEndpointAddress
    EP_BULK,                		// bmAttributes
    EP3_MAX_PKT_SIZE, 0x00,  		// wMaxPacketSize
    0x00,                    		// bInterval

    // Interface Association Descriptor (IAD)
    0x08,               // bLength
    0x0B,               // bDescriptorType: IAD
    0x01,               // bFirstInterface
    0x02,               // bInterfaceCount
    0x02,               // bFunctionClass: CDC
    0x02,               // bFunctionSubClass
    0x01,               // bFunctionProtocol
    0x00,               // iFunction, 描述字符串索引

    // I/F descriptor: VCP Ctrl
    LEN_INTERFACE,      // bLength
    DESC_INTERFACE,     // bDescriptorType
    0x01,               // bInterfaceNumber
    0x00,               // bAlternateSetting
    0x01,               // bNumEndpoints
    0x02,               // bInterfaceClass
    0x02,               // bInterfaceSubClass
    0x01,               // bInterfaceProtocol
    0x00,               // iInterface

    // Communication Class Specified INTERFACE descriptor
    0x05,               // Size of the descriptor, in bytes
    0x24,               // CS_INTERFACE descriptor type
    0x00,               // Header functional descriptor subtype
    0x10, 0x01,         // Communication device compliant to the communication spec. ver. 1.10

    // Communication Class Specified INTERFACE descriptor
    0x05,               // Size of the descriptor, in bytes
    0x24,               // CS_INTERFACE descriptor type
    0x01,               // Call management functional descriptor
    0x00,               // BIT0: Whether device handle call management itself.
                        // BIT1: Whether device can send/receive call management information over a Data Class Interface 0
    0x02,               // Interface number of data class interface optionally used for call management

    // Communication Class Specified INTERFACE descriptor
    0x04,               // Size of the descriptor, in bytes
    0x24,               // CS_INTERFACE descriptor type
    0x02,               // Abstract control management functional descriptor subtype
    0x00,               // bmCapabilities

    // Communication Class Specified INTERFACE descriptor
    0x05,               // bLength
    0x24,               // bDescriptorType: CS_INTERFACE descriptor type
    0x06,               // bDescriptorSubType
    0x01,               // bMasterInterface
    0x02,               // bSlaveInterface0

    // ENDPOINT descriptor
    LEN_ENDPOINT,                   // bLength
    DESC_ENDPOINT,                  // bDescriptorType
    (EP_INPUT | CDC_INT_IN_EP),   	// bEndpointAddress
    EP_INT,                         // bmAttributes
    EP4_MAX_PKT_SIZE, 0x00,         // wMaxPacketSize
    10,                           	// bInterval

    // I/F descriptor: VCP Data
    LEN_INTERFACE,      // bLength
    DESC_INTERFACE,     // bDescriptorType
    0x02,               // bInterfaceNumber
    0x00,               // bAlternateSetting
    0x02,               // bNumEndpoints
    0x0A,               // bInterfaceClass
    0x00,               // bInterfaceSubClass
    0x00,               // bInterfaceProtocol
    0x00,               // iInterface

    // ENDPOINT descriptor
    LEN_ENDPOINT,                   // bLength
    DESC_ENDPOINT,                  // bDescriptorType
    (EP_INPUT | CDC_BULK_IN_EP),  	// bEndpointAddress
    EP_BULK,                        // bmAttributes
    EP5_MAX_PKT_SIZE, 0x00,         // wMaxPacketSize
    0x00,                           // bInterval

    // ENDPOINT descriptor
    LEN_ENDPOINT,                   // bLength
    DESC_ENDPOINT,                  // bDescriptorType
    (EP_OUTPUT | CDC_BULK_OUT_EP),	// bEndpointAddress
    EP_BULK,                        // bmAttributes
    EP6_MAX_PKT_SIZE, 0x00,         // wMaxPacketSize
    0x00,                           // bInterval
};


uint8_t gu8StringLang[4] =
{
    4,              	// bLength
    DESC_STRING,    	// bDescriptorType
    0x09, 0x04
};

uint8_t gu8VendorStringDesc[] =
{
    16,                 // bLength
    DESC_STRING,        // bDescriptorType
    'N', 0, 'u', 0, 'v', 0, 'o', 0, 't', 0, 'o', 0, 'n', 0
};

uint8_t gu8ProductStringDesc[] =
{
    22,             	// bLength
    DESC_STRING,    	// bDescriptorType
    'U', 0, 'S', 0, 'B', 0, ' ', 0, 'D', 0, 'e', 0, 'v', 0, 'i', 0, 'c', 0, 'e', 0
};

uint8_t gu8StringSerial[] =
{
    26,             	// bLength
    DESC_STRING,    	// bDescriptorType
    'A', 0, '0', 0, '0', 0, '0', 0, '0', 0, '8', 0, '0', 0, '4', 0, '0', 0, '1', 0, '1', 0, '5', 0

};

const uint8_t *gpu8UsbString[4] =
{
    gu8StringLang,
    gu8VendorStringDesc,
    gu8ProductStringDesc,
    gu8StringSerial
};


const S_USBD_INFO_T gsInfo =
{
    (uint8_t *)gu8DeviceDescriptor,
    (uint8_t *)gu8ConfigDescriptor,
    (uint8_t **)gpu8UsbString,
    NULL,
    NULL,
    NULL,
    NULL
};
