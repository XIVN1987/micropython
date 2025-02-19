#include <string.h>
#include "chip/M480.h"

#include "misc/spi_nor.h"
#include "misc/usb_msc.h"


void usb_disk_init(void)
{
    SYS_UnlockReg();

    SYS->GPA_MFPH &= ~(SYS_GPA_MFPH_PA12MFP_Msk      | SYS_GPA_MFPH_PA13MFP_Msk     | SYS_GPA_MFPH_PA14MFP_Msk     | SYS_GPA_MFPH_PA15MFP_Msk);
    SYS->GPA_MFPH |=  (SYS_GPA_MFPH_PA12MFP_USB_VBUS | SYS_GPA_MFPH_PA13MFP_USB_D_N | SYS_GPA_MFPH_PA14MFP_USB_D_P | SYS_GPA_MFPH_PA15MFP_USB_OTG_ID);

    SYS->USBPHY = (SYS->USBPHY & ~SYS_USBPHY_USBROLE_Msk) | SYS_USBPHY_USBEN_Msk | SYS_USBPHY_SBO_Msk;

    CLK->CLKDIV0 = (CLK->CLKDIV0 & ~CLK_CLKDIV0_USBDIV_Msk) | CLK_CLKDIV0_USB(4);
    CLK_EnableModuleClock(USBD_MODULE);

    SYS_LockReg();


    USBD_Open(&gsInfo, MSC_ClassRequest, NULL);

    USBD_SetConfigCallback(MSC_SetConfig);

    MSC_Init(); 	// Endpoint configuration

    USBD_Start();

    NVIC_EnableIRQ(USBD_IRQn);
}


struct CBW g_CBW = {.dCBWSignature = CBW_SIGNATURE};   // Command Block Wrapper
struct CSW g_CSW = {.dCSWSignature = CSW_SIGNATURE};   // Command Status Wrapper

static uint8_t MSC_State;

#define MSC_STATE_CBW  0
#define MSC_STATE_IN   1
#define MSC_STATE_OUT  2
#define MSC_STATE_CSW  3


void MSC_Init(void)
{
	/* Buffer range for setup packet -> [0 ~ 0x7] */
	USBD->STBUFSEG = SETUP_BUF_BASE;

	/*****************************************************/
	/* EP0 ==> control IN endpoint, address 0 */
	USBD_CONFIG_EP(EP0, USBD_CFG_CSTALL | USBD_CFG_EPMODE_IN | 0);
	USBD_SET_EP_BUF_ADDR(EP0, EP0_BUF_BASE);

	/* EP1 ==> control OUT endpoint, address 0 */
	USBD_CONFIG_EP(EP1, USBD_CFG_CSTALL | USBD_CFG_EPMODE_OUT | 0);
	USBD_SET_EP_BUF_ADDR(EP1, EP1_BUF_BASE);

	/*****************************************************/
	/* EP2 ==> Bulk IN endpoint, address 1 */
	USBD_CONFIG_EP(EP2, USBD_CFG_EPMODE_IN | MSC_BULK_IN_EP);
	USBD_SET_EP_BUF_ADDR(EP2, EP2_BUF_BASE);

	/* EP3 ==> Bulk Out endpoint, address 1 */
	USBD_CONFIG_EP(EP3, USBD_CFG_EPMODE_OUT | MSC_BULK_OUT_EP);
	USBD_SET_EP_BUF_ADDR(EP3, EP3_BUF_BASE);
	/* trigger to receive OUT data */
	USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);

	/*
	   Generate Mass-Storage Device serial number
	   To compliant USB-IF MSC test, we must enable serial string descriptor.
	   However, window may fail to recognize the devices if PID/VID and serial number are all the same
	   when plug them to Windows at the sample time.
	   Therefore, we must generate different serial number for each device to avoid conflict
	   when plug more then 2 MassStorage devices to Windows at the same time.

	   NOTE: We use compiler predefine macro "__TIME__" to generate different number for serial
	   at each build but each device here for a demo.
	   User must change it to make sure all serial number is different between each device.
	 */
	uint8_t *stringSerial = gsInfo.gu8StringDesc[3];
	for(int i = 0; i < 8; i++)  stringSerial[stringSerial[0] - 16 + i * 2] = __TIME__[i];
}


void MSC_AckCmd(void);
void EP2_Handler(void)  // Bulk IN
{
	MSC_AckCmd();
}

volatile uint8_t EP3Ready = 0;
void EP3_Handler(void)  // Bulk OUT
{
	EP3Ready = 1;

	MSC_ProcessCmd();
}

void USBD_IRQHandler(void)
{
	uint32_t usb_if = USBD_GET_INT_FLAG();
	uint32_t bus_st = USBD_GET_BUS_STATE();

	if(usb_if & USBD_INTSTS_FLDET)      // Floating detect
	{
		USBD_CLR_INT_FLAG(USBD_INTSTS_FLDET);

		if(USBD_IS_ATTACHED())          // USB Plug In
		{
			USBD_ENABLE_USB();
		}
		else                            // USB Un-plug
		{
			USBD_DISABLE_USB();
		}
	}

	if(usb_if & USBD_INTSTS_BUS)        // Bus event
	{
		USBD_CLR_INT_FLAG(USBD_INTSTS_BUS);

		if(bus_st & USBD_STATE_USBRST)
		{
			USBD_ENABLE_USB();
			USBD_SwReset();
		}
		if(bus_st & USBD_STATE_SUSPEND)
		{
			USBD_DISABLE_PHY();
		}
		if(bus_st & USBD_STATE_RESUME)
		{
			USBD_ENABLE_USB();
		}
	}

	if(usb_if & USBD_INTSTS_WAKEUP)
	{
		USBD_CLR_INT_FLAG(USBD_INTSTS_WAKEUP);
	}

	if(usb_if & USBD_INTSTS_USB)        // USB event
	{
		if(usb_if & USBD_INTSTS_SETUP)
		{
			USBD_CLR_INT_FLAG(USBD_INTSTS_SETUP);

			/* Clear the data IN/OUT ready flag of control end-points */
			USBD_STOP_TRANSACTION(EP0);
			USBD_STOP_TRANSACTION(EP1);

			USBD_ProcessSetupPacket();
		}
		
		if(usb_if & USBD_INTSTS_EP0)    // Control IN
		{
			USBD_CLR_INT_FLAG(USBD_INTSTS_EP0);
			
			USBD_CtrlIn();
		}

		if(usb_if & USBD_INTSTS_EP1)    // Control OUT
		{
			USBD_CLR_INT_FLAG(USBD_INTSTS_EP1);

			USBD_CtrlOut();
		}

		if(usb_if & USBD_INTSTS_EP2)    // MSC Bulk IN
		{
			USBD_CLR_INT_FLAG(USBD_INTSTS_EP2);
			
			EP2_Handler();
		}

		if(usb_if & USBD_INTSTS_EP3)    // MSC Bulk OUT
		{
			USBD_CLR_INT_FLAG(USBD_INTSTS_EP3);
			
			EP3_Handler();
		}

        if(usb_if & USBD_INTSTS_EP4)	// CDC Int IN
        {
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP4);
        }

		if(usb_if & USBD_INTSTS_EP5)	// CDC Bulk IN
        {
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP5);
            
            EP5_Handler();
        }

        if(usb_if & USBD_INTSTS_EP6)	// CDC Bulk OUT
        {
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP6);
            
            EP6_Handler();
        }
	}
}


void MSC_SetConfig(void)
{
	// Clear stall status and ready
	USBD->EP[2].CFGP = 1;
	USBD->EP[3].CFGP = 1;
	
	/* trigger to receive OUT data */
	USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);

	MSC_State = MSC_STATE_CBW;
}


void MSC_ClassRequest(void)
{
	uint8_t buf[8];

	USBD_GetSetupPacket(buf);

	if(buf[0] & EP_INPUT)   // Device to host
	{
		switch(buf[1])
		{
		case GET_MAX_LUN:
			/* buf[4] 存储的是 bInterfaceNumber */
			if((buf[4] == gsInfo.gu8ConfigDesc[LEN_CONFIG + 2]) && (buf[2] + buf[3] + buf[6] + buf[7] == 1))
			{
				/* Data stage */
				*(volatile uint8_t *)(USBD_BUF_BASE + EP0_BUF_BASE) = 0;
				USBD_SET_DATA1(EP0);
				USBD_SET_PAYLOAD_LEN(EP0, 1);
				
				/* Status stage, Control OUT (EP1) */
				USBD_PrepareCtrlOut(0, 0);
			}
			else
			{
				/* Stall when wrong parameter, Control OUT */
				USBD_SET_EP_STALL(EP1);
			}
			break;

		default:
			/* Setup error, stall the device */
			USBD_SetStall(0);
			break;
		}
	}
	else                    // Host to device
	{
		switch(buf[1])
		{
		case MSC_STORAGE_RESET:
			/* buf[4] 存储的是 bInterfaceNumber */
			if((buf[4] == gsInfo.gu8ConfigDesc[LEN_CONFIG + 2]) && (buf[2] + buf[3] + buf[6] + buf[7] == 0))
			{				
				/* Clear ready */
				USBD->EP[EP2].CFGP |= USBD_CFGP_CLRRDY_Msk;
				USBD->EP[EP3].CFGP |= USBD_CFGP_CLRRDY_Msk;

				/* Prepare to receive the CBW */
				EP3Ready = 0;
				MSC_State = MSC_STATE_CBW;

				USBD_SET_DATA1(EP3);
				USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);
				
				/* Status stage */
				USBD_SET_DATA1(EP0);
				USBD_SET_PAYLOAD_LEN(EP0, 0);
			}
			else
			{
				/* Stall when wrong parameter */
				USBD_SET_EP_STALL(EP1);
			}
			break;

		default:
			/* Setup error, stall the device */
			USBD_SetStall(0);
			break;
		}
	}
}



/*----------------------------------------------------------------------------*/
static uint8_t Buffer[256];

static uint32_t g_Address;
static uint32_t g_Size;

static uint8_t  g_Remove = 0;
static uint8_t  g_PreventRemove = 0;

static uint8_t  g_SenseKey[4];

static uint8_t InquiryData[36] =
{
	0x00,                   // Peripheral Device Type
	0x80,                   // Removable Medium Bit
	0x00,                   // ISO/ECMA, ANSI Version
	0x00,                   // Response Data Format
	0x1F, 0x00, 0x00, 0x00, // Additional Length

	/* Vendor Identification */
	'N', 'u', 'v', 'o', 't', 'o', 'n', ' ',

	/* Product Identification */
	'U', 'S', 'B', ' ', 'M', 'a', 's', 's', ' ', 'S', 't', 'o', 'r', 'a', 'g', 'e',

	/* Product Revision */
	'1', '.', '0', '0'
};


static __INLINE uint32_t min(uint32_t a, uint32_t b)
{
	return (a < b) ? a : b;
}

void MSC_Read(void);
void MSC_Write(void);
void MediaRead(uint32_t addr, uint32_t size, uint8_t *buffer);
void MediaWrite(uint32_t addr, uint32_t size, uint8_t *buffer);


void MSC_ProcessCmd(void)
{
	uint8_t size;
	
	if(EP3Ready)
	{
		EP3Ready = 0;

		if(MSC_State == MSC_STATE_CBW)
		{
			size = USBD_GET_PAYLOAD_LEN(EP3);

			USBD_MemCopy((uint8_t *)(&g_CBW), (uint8_t *)(USBD_BUF_BASE + EP3_BUF_BASE), sizeof(g_CBW));

			if((g_CBW.dCBWSignature != CBW_SIGNATURE) || (size != 31))
			{
				/* Invalid CBW */
				USBD_SET_EP_STALL(EP2);
				USBD_SET_EP_STALL(EP3);
				return;
			}

			/* Prepare to echo the tag from CBW to CSW */
			g_CSW.dCSWTag = g_CBW.dCBWTag;
			
			switch(g_CBW.u8OPCode)
			{
			case UFI_INQUIRY:
				if((g_CBW.dCBWDataTransferLength > 0) && (g_CBW.dCBWDataTransferLength <= 36))
				{
					USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + EP2_BUF_BASE), InquiryData, g_CBW.dCBWDataTransferLength);
					USBD_SET_PAYLOAD_LEN(EP2, g_CBW.dCBWDataTransferLength);
					
					g_CSW.bCSWStatus = 0;
					g_CSW.dCSWDataResidue = 0;
					
					/* EP2数据传回主机后，在MSC_AckCmd中根据MSC_State的值决定下一操作 */
					MSC_State = MSC_STATE_IN;
				}
				else
				{
					USBD_SET_EP_STALL(EP2);
					
					/* 主机检测到EP2 STALL后会使用标准请求“Clear Feature”清除，然后再读取CSW */
					g_CSW.bCSWStatus = 1;
					g_CSW.dCSWDataResidue = 0;
					
					MSC_State = MSC_STATE_IN;
					MSC_AckCmd();
				}
				return;
			
			case UFI_READ_FORMAT_CAPACITY:
				g_Address = (uint32_t)Buffer;
				g_Size = g_CBW.dCBWDataTransferLength;	// 主机不知道设备的“Capacity List Length”是多少，所以可能会读256字节
				
				memset(Buffer, 0, sizeof(Buffer));

				/*---------- Capacity List Header -------------------------*/
				// Capacity List Length
				Buffer[3] = 0x10;

				/*---------- Current/Maximum Capacity Descriptor ----------*/
				// Number of Blocks (MSB first)
				Buffer[4]  = (FLASH_SECTOR_COUNT >> 24) & 0xFF;
				Buffer[5]  = (FLASH_SECTOR_COUNT >> 16) & 0xFF;
				Buffer[6]  = (FLASH_SECTOR_COUNT >>  8) & 0xFF;
				Buffer[7]  = (FLASH_SECTOR_COUNT >>  0) & 0xFF;

				// Descriptor Code:
				// 01b = Unformatted Media - Maximum formattable capacity for this cartridge
				// 10b = Formatted Media - Current media capacity
				// 11b = No Cartridge in Drive - Maximum formattable capacity for any cartridge
				Buffer[8]  = 0x02;

				// Block Length. Fixed to be 512 (MSB first)
				Buffer[ 9] = 0x00;
				Buffer[10] = 0x02;
				Buffer[11] = 0x00;

				/*---------- Formattable Capacity Descriptor --------------*/
				// Number of Blocks (MSB first)
				Buffer[12] = (FLASH_SECTOR_COUNT >> 24) & 0xFF;
				Buffer[13] = (FLASH_SECTOR_COUNT >> 16) & 0xFF;
				Buffer[14] = (FLASH_SECTOR_COUNT >>  8) & 0xFF;
				Buffer[15] = (FLASH_SECTOR_COUNT >>  0) & 0xFF;

				// Block Length. Fixed to be 512 (MSB first)
				Buffer[17] = 0x00;
				Buffer[18] = 0x02;
				Buffer[19] = 0x00;
				
				size = min(g_Size, EP2_MAX_PKT_SIZE);

				USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + EP2_BUF_BASE), (uint8_t *)g_Address, size);
				USBD_SET_PAYLOAD_LEN(EP2, size);

				g_Address += size;
				g_Size -= size;
				
				MSC_State = MSC_STATE_IN;
				return;
			
			case UFI_READ_CAPACITY:
				g_Address = (uint32_t)Buffer;
				g_Size = g_CBW.dCBWDataTransferLength;
				
				memset(Buffer, 0, sizeof(Buffer));
				
				/* the last logical block number on the device */
				Buffer[0] = ((FLASH_SECTOR_COUNT - 1) >> 24) & 0xFF;
				Buffer[1] = ((FLASH_SECTOR_COUNT - 1) >> 16) & 0xFF;
				Buffer[2] = ((FLASH_SECTOR_COUNT - 1) >>  8) & 0xFF;
				Buffer[3] = ((FLASH_SECTOR_COUNT - 1) >>  0) & 0xFF;
				
				Buffer[5] = 0x00;
				Buffer[6] = 0x02;	// 512字节
				Buffer[7] = 0x00;

				size = min(g_Size, EP2_MAX_PKT_SIZE);

				USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + EP2_BUF_BASE), (uint8_t *)g_Address, size);
				USBD_SET_PAYLOAD_LEN(EP2, size);

				g_Address += size;
				g_Size -= size;
				
				MSC_State = MSC_STATE_IN;
				return;
			
			case UFI_READ_10:
			case UFI_READ_12:
				g_Address = __REV(*(uint32_t *)g_CBW.au8Data) * FLASH_SECTOR_SIZE;
				g_Size = g_CBW.dCBWDataTransferLength;

				size = min(g_Size, EP2_MAX_PKT_SIZE);

				MediaRead(g_Address, size, Buffer);

				USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + EP2_BUF_BASE), Buffer, size);
				USBD_SET_PAYLOAD_LEN(EP2, size);

				g_Address += size;
				g_Size -= size;
				
				MSC_State = MSC_STATE_IN;
				return;

			case UFI_WRITE_10:
			case UFI_WRITE_12:
				g_Address = __REV(*(uint32_t *)g_CBW.au8Data) * FLASH_SECTOR_SIZE;
				g_Size = g_CBW.dCBWDataTransferLength;

				USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);
				
				MSC_State = MSC_STATE_OUT;
				return;
			
			case UFI_VERIFY_10:
				MSC_State = MSC_STATE_IN;
				MSC_AckCmd();
				return;
			
			case UFI_START_STOP:
				if((g_CBW.au8Data[2] & 3) == 2)
				{
					g_Remove = 1;
				}

				MSC_State = MSC_STATE_IN;
				MSC_AckCmd();
				return;
			
			case UFI_TEST_UNIT_READY:
				if(g_CBW.dCBWDataTransferLength != 0)
				{
					if(g_CBW.bmCBWFlags == 0)	// OUT
					{
						USBD_SET_EP_STALL(EP3);
						
						g_CSW.bCSWStatus = 1;
						g_CSW.dCSWDataResidue = g_CBW.dCBWDataTransferLength;
					}
				}
				else
				{
					if(g_Remove)
					{
						g_CSW.bCSWStatus = 1;
						g_CSW.dCSWDataResidue = 0;
						
						g_SenseKey[0] = 0x02;    // Not ready
						g_SenseKey[1] = 0x3A;
						g_SenseKey[2] = 0x00;
					}
					else
					{
						g_CSW.bCSWStatus = 0;
						g_CSW.dCSWDataResidue = 0;
					}
				}

				MSC_State = MSC_STATE_IN;
				MSC_AckCmd();
				return;
			
			case UFI_PREVENT_ALLOW_MEDIUM_REMOVAL:
				if(g_CBW.au8Data[2] & 1)
				{
					g_SenseKey[0] = 0x05;  // INVALID COMMAND
					g_SenseKey[1] = 0x24;
					g_SenseKey[2] = 0x00;
					
					g_PreventRemove = 1;
				}
				else
				{
					g_PreventRemove = 0;
				}

				MSC_State = MSC_STATE_IN;
				MSC_AckCmd();
				return;

			case UFI_REQUEST_SENSE:
				if((g_CBW.dCBWDataTransferLength > 0) && (g_CBW.dCBWDataTransferLength <= 18))
				{
					memset(Buffer, 0, 18);
					
					Buffer[0]  = 0x70;
					Buffer[2]  = g_SenseKey[0];
					Buffer[7]  = 0x0A;
					Buffer[12] = g_SenseKey[1];
					Buffer[13] = g_SenseKey[2];
					
					USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + EP2_BUF_BASE), Buffer, g_CBW.dCBWDataTransferLength);
					USBD_SET_PAYLOAD_LEN(EP2, g_CBW.dCBWDataTransferLength);
					
					g_SenseKey[0] = 0;
					g_SenseKey[1] = 0;
					g_SenseKey[2] = 0;

					g_CSW.bCSWStatus = 0;
					g_CSW.dCSWDataResidue = 0;
					
					MSC_State = MSC_STATE_IN;
				}
				else
				{
					USBD_SET_EP_STALL(EP2);
					
					g_CSW.bCSWStatus = 1;
					g_CSW.dCSWDataResidue = 0;
					
					MSC_State = MSC_STATE_IN;
					MSC_AckCmd();
				}
				return;

			default:
				/* Unsupported command */
				g_SenseKey[0] = 0x05;
				g_SenseKey[1] = 0x20;
				g_SenseKey[2] = 0x00;

				/* If CBW request for data phase, just return zero len packet to end data phase */
				if(g_CBW.dCBWDataTransferLength > 0)
				{
					if((g_CBW.bmCBWFlags & 0x80) != 0)
					{
						USBD_SET_PAYLOAD_LEN(EP2, 0);
						MSC_State = MSC_STATE_IN;
					}
				}
				else
				{
					MSC_State = MSC_STATE_IN;
					MSC_AckCmd();
				}
				return;
			}
		}
		else if(MSC_State == MSC_STATE_OUT)
		{
			switch(g_CBW.u8OPCode)
			{
			case UFI_WRITE_10:
			case UFI_WRITE_12:
				MSC_Write();
				return;
			}
		}
	}
}


void MSC_AckCmd(void)
{
	uint8_t size;
	
	if(MSC_State == MSC_STATE_CSW)
	{
		MSC_State = MSC_STATE_CBW;	// Prepare to receive the CBW

		USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);
	}
	else if(MSC_State == MSC_STATE_IN)
	{
		switch(g_CBW.u8OPCode)
		{
		case UFI_INQUIRY:
			break;
		
		case UFI_READ_FORMAT_CAPACITY:
		case UFI_READ_CAPACITY:
			if(g_Size > 0)
			{
				size = min(g_Size, EP2_MAX_PKT_SIZE);
				
				USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + EP2_BUF_BASE), (uint8_t *)g_Address, size);
				USBD_SET_PAYLOAD_LEN(EP2, size);

				g_Address += size;
				g_Size -= size;
				return;
			}
			
			g_CSW.dCSWDataResidue = 0;
			g_CSW.bCSWStatus = 0;
			break;
		
		case UFI_READ_10:
		case UFI_READ_12:
			if(g_Size > 0)
			{
				MSC_Read();
				return;
			}

			g_CSW.dCSWDataResidue = 0;
			g_CSW.bCSWStatus = 0;
			break;
		
		case UFI_WRITE_10:
		case UFI_WRITE_12:
			g_CSW.dCSWDataResidue = 0;
			g_CSW.bCSWStatus = 0;
			break;

		case UFI_VERIFY_10:
		case UFI_START_STOP:
		case UFI_PREVENT_ALLOW_MEDIUM_REMOVAL:
			g_CSW.dCSWDataResidue = (g_CBW.dCBWDataTransferLength < FLASH_SECTOR_SIZE) ? 0 : (g_CBW.dCBWDataTransferLength - FLASH_SECTOR_SIZE);
			g_CSW.bCSWStatus = 0;
			break;
		
		case UFI_TEST_UNIT_READY:
		case UFI_REQUEST_SENSE:
			break;
		
		default:
			/* Unsupported commmand. Return command fail status */
			g_CSW.dCSWDataResidue = g_CBW.dCBWDataTransferLength;
			g_CSW.bCSWStatus = 1;
			break;
		}

		USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + EP2_BUF_BASE), (uint8_t *)&g_CSW, 13);
		USBD_SET_PAYLOAD_LEN(EP2, 13);

		MSC_State = MSC_STATE_CSW;
	}
}


void MSC_Read(void)
{
	uint8_t size;
	
	size = min(g_Size, EP2_MAX_PKT_SIZE);

	MediaRead(g_Address, size, Buffer);
	g_Address += size;
	g_Size -= size;

	USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + EP2_BUF_BASE), Buffer, size);
	USBD_SET_PAYLOAD_LEN(EP2, size);
}


void MSC_Write(void)
{
	if(g_Size > EP3_MAX_PKT_SIZE)
	{
		USBD_MemCopy(Buffer, (uint8_t *)(USBD_BUF_BASE + EP3_BUF_BASE), EP3_MAX_PKT_SIZE);
		USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);
		
		MediaWrite(g_Address, EP3_MAX_PKT_SIZE, Buffer);
		g_Address += EP3_MAX_PKT_SIZE;
		g_Size -= EP3_MAX_PKT_SIZE;
	}
	else
	{
		USBD_MemCopy(Buffer, (uint8_t *)(USBD_BUF_BASE + EP3_BUF_BASE), g_Size);
		
		MediaWrite(g_Address, g_Size, Buffer);
		g_Size = 0;
		
		MSC_State = MSC_STATE_IN;
		MSC_AckCmd();
	}
}


static uint8_t RWBuffer[FLASH_SECTOR_SIZE];

void MediaRead(uint32_t addr, uint32_t size, uint8_t *buffer)
{
	uint32_t sector_number = addr / FLASH_SECTOR_SIZE;
	uint32_t offset_in_sector = addr % FLASH_SECTOR_SIZE;

	if(offset_in_sector == 0)
		flash_disk_read(RWBuffer, sector_number, 1);

	memcpy(buffer, &RWBuffer[offset_in_sector], size);
}

void MediaWrite(uint32_t addr, uint32_t size, uint8_t *buffer)
{
	uint32_t sector_number = addr / FLASH_SECTOR_SIZE;
	uint32_t offset_in_sector = addr % FLASH_SECTOR_SIZE;

	memcpy(&RWBuffer[offset_in_sector], buffer, size);

	if((offset_in_sector + size) == FLASH_SECTOR_SIZE)
		flash_disk_write(RWBuffer, sector_number, 1);
}
