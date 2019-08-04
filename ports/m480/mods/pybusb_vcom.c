#include <string.h>
#include "py/mphal.h"

#include "NuMicro.h"
#include "pybusb_vcom.h"


STR_VCOM_LINE_CODING LineCfg = {115200, 0, 0, 8};   // Baud rate : 115200, stop bits, parity bits, data bits

volatile VCOM vcom;


void USBD_IRQHandler(void)
{
    uint32_t u32IntSts = USBD_GET_INT_FLAG();
    uint32_t u32State = USBD_GET_BUS_STATE();

//------------------------------------------------------------------
    if (u32IntSts & USBD_INTSTS_FLDET)
    {
        // Floating detect
        USBD_CLR_INT_FLAG(USBD_INTSTS_FLDET);

        if (USBD_IS_ATTACHED())
        {
            /* USB Plug In */
            USBD_ENABLE_USB();
        }
        else
        {
            /* USB Un-plug */
            USBD_DISABLE_USB();
        }
    }

//------------------------------------------------------------------
    if (u32IntSts & USBD_INTSTS_BUS)
    {
        /* Clear event flag */
        USBD_CLR_INT_FLAG(USBD_INTSTS_BUS);

        if (u32State & USBD_STATE_USBRST)
        {
            /* Bus reset */
            USBD_ENABLE_USB();
            USBD_SwReset();
        }
        if (u32State & USBD_STATE_SUSPEND)
        {
            /* Enable USB but disable PHY */
            USBD_DISABLE_PHY();
        }
        if (u32State & USBD_STATE_RESUME)
        {
            /* Enable USB and enable PHY */
            USBD_ENABLE_USB();
        }
    }

//------------------------------------------------------------------
    if(u32IntSts & USBD_INTSTS_WAKEUP)
    {
        /* Clear event flag */
        USBD_CLR_INT_FLAG(USBD_INTSTS_WAKEUP);
    }

//------------------------------------------------------------------
    if (u32IntSts & USBD_INTSTS_USB)
    {
        extern uint8_t g_usbd_SetupPacket[];
        // USB event
        if (u32IntSts & USBD_INTSTS_SETUP)
        {
            // Setup packet
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_SETUP);

            /* Clear the data IN/OUT ready flag of control end-points */
            USBD_STOP_TRANSACTION(EP0);
            USBD_STOP_TRANSACTION(EP1);

            USBD_ProcessSetupPacket();
        }

        // EP events
        if (u32IntSts & USBD_INTSTS_EP0)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP0);

            // control IN
            USBD_CtrlIn();
        }

        if (u32IntSts & USBD_INTSTS_EP1)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP1);

            // control OUT
            USBD_CtrlOut();

            // In ACK of SET_LINE_CODE
            if(g_usbd_SetupPacket[1] == SET_LINE_CODE)
            {
                VCOM_LineCoding();  // Apply UART settings
            }
        }

        if (u32IntSts & USBD_INTSTS_EP2)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP2);
            // Bulk IN
            EP2_Handler();
        }

        if (u32IntSts & USBD_INTSTS_EP3)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP3);
            // Bulk Out
            EP3_Handler();
        }

        if (u32IntSts & USBD_INTSTS_EP4)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP4);
        }

        if (u32IntSts & USBD_INTSTS_EP5)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP5);
        }

        if (u32IntSts & USBD_INTSTS_EP6)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP6);
        }

        if (u32IntSts & USBD_INTSTS_EP7)
        {
            /* Clear event flag */
            USBD_CLR_INT_FLAG(USBD_INTSTS_EP7);
        }
    }
}


void EP2_Handler(void)
{
    /* Bulk IN */
    USB_VCP_sendPack();
}

void EP3_Handler(void)
{
    /* Bulk OUT */
    vcom.out_bytes = USBD_GET_PAYLOAD_LEN(EP3);
    vcom.out_ptr = (uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP3));

    if(vcom.rx_read <= vcom.rx_write)
    {
        uint n = RX_BUFF_SIZE - vcom.rx_write;
        if(n > vcom.out_bytes) n = vcom.out_bytes;

        USBD_MemCopy((uint8_t *)&vcom.rx_buff[vcom.rx_write], (uint8_t *)vcom.out_ptr, n);
        vcom.rx_write += n;
        if(vcom.rx_write == RX_BUFF_SIZE) vcom.rx_write = 0;

        vcom.out_ptr += n;
        vcom.out_bytes -= n;
    }

    if(vcom.out_bytes)
    {
        uint n = (vcom.rx_read - 1) - vcom.rx_write;
        if(n > vcom.out_bytes) n = vcom.out_bytes;

        USBD_MemCopy((uint8_t *)&vcom.rx_buff[vcom.rx_write], (uint8_t *)vcom.out_ptr, n);
        vcom.rx_write += n;
    }

    /* Ready to get next BULK out */
    USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);
}


void VCOM_Init(void)
{
    /* Init setup packet buffer */
    /* Buffer for setup packet -> [0 ~ 0x7] */
    USBD->STBUFSEG = SETUP_BUF_BASE;

    /*****************************************************/
    /* EP0 ==> control IN endpoint, address 0 */
    USBD_CONFIG_EP(EP0, USBD_CFG_CSTALL | USBD_CFG_EPMODE_IN | 0);
    /* Buffer range for EP0 */
    USBD_SET_EP_BUF_ADDR(EP0, EP0_BUF_BASE);

    /* EP1 ==> control OUT endpoint, address 0 */
    USBD_CONFIG_EP(EP1, USBD_CFG_CSTALL | USBD_CFG_EPMODE_OUT | 0);
    /* Buffer range for EP1 */
    USBD_SET_EP_BUF_ADDR(EP1, EP1_BUF_BASE);

    /*****************************************************/
    /* EP2 ==> Bulk IN endpoint, address 1 */
    USBD_CONFIG_EP(EP2, USBD_CFG_EPMODE_IN | BULK_IN_EP_NUM);
    /* Buffer offset for EP2 */
    USBD_SET_EP_BUF_ADDR(EP2, EP2_BUF_BASE);

    /* EP3 ==> Bulk Out endpoint, address 2 */
    USBD_CONFIG_EP(EP3, USBD_CFG_EPMODE_OUT | BULK_OUT_EP_NUM);
    /* Buffer offset for EP3 */
    USBD_SET_EP_BUF_ADDR(EP3, EP3_BUF_BASE);
    /* trigger receive OUT data */
    USBD_SET_PAYLOAD_LEN(EP3, EP3_MAX_PKT_SIZE);

    /* EP4 ==> Interrupt IN endpoint, address 3 */
    USBD_CONFIG_EP(EP4, USBD_CFG_EPMODE_IN | INT_IN_EP_NUM);
    /* Buffer offset for EP4 ->  */
    USBD_SET_EP_BUF_ADDR(EP4, EP4_BUF_BASE);
}


void VCOM_ClassRequest(void)
{
    uint8_t buf[8];

    USBD_GetSetupPacket(buf);

    if(buf[0] & 0x80)
    {
        // Device to host
        switch(buf[1])
        {
        case GET_LINE_CODE:
            USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP0)), (uint8_t *)&LineCfg, 7);

            /* Data stage */
            USBD_SET_DATA1(EP0);
            USBD_SET_PAYLOAD_LEN(EP0, 7);

            /* Status stage */
            USBD_PrepareCtrlOut(0,0);
            break;

        default:
            /* Setup error, stall the device */
            USBD_SetStall(0);
            break;
        }
    }
    else
    {
        // Host to device
        switch (buf[1])
        {
        case SET_CONTROL_LINE_STATE:
            vcom.hw_flow = buf[3];
            vcom.hw_flow = (vcom.hw_flow << 8) | buf[2];

            /* Status stage */
            USBD_SET_DATA1(EP0);
            USBD_SET_PAYLOAD_LEN(EP0, 0);
            break;

        case SET_LINE_CODE:
            USBD_PrepareCtrlOut((uint8_t *)&LineCfg, 7);

            /* Status stage */
            USBD_SET_DATA1(EP0);
            USBD_SET_PAYLOAD_LEN(EP0, 0);
            break;

        default:
            /* Setup error, stall the device */
            USBD_SetStall(0);
            break;
        }
    }
}

void VCOM_LineCoding(void)
{
    /*
    uint32_t data_len, parity, stop_len;

    NVIC_DisableIRQ(UART0_IRQn);
    // Reset software FIFO
    vcom.rx_bytes = 0;
    vcom.rx_read = 0;
    vcom.rx_write = 0;

    vcom.tx_write = 0;
    vcom.tx_read = 0;
    vcom.tx_tail = 0;

    // Reset hardware FIFO
    UART0->FIFO = 0x3;

    switch(LineCfg.u8DataBits)
    {
    case 5:  data_len = UART_WORD_LEN_5; break;
    case 6:  data_len = UART_WORD_LEN_6; break;
    case 7:  data_len = UART_WORD_LEN_7; break;
    case 8:  data_len = UART_WORD_LEN_8; break;
    default: data_len = UART_WORD_LEN_8; break;
    }

    switch(LineCfg.u8ParityType)
    {
    case 0:  parity = UART_PARITY_NONE;  break;
    case 1:  parity = UART_PARITY_ODD;   break;
    case 2:  parity = UART_PARITY_EVEN;  break;
    case 3:  parity = UART_PARITY_MARK;  break;
    case 4:  parity = UART_PARITY_SPACE; break;
    default: parity = UART_PARITY_NONE;  break;
    }

    switch(LineCfg.u8CharFormat)
    {
    case 0:  stop_len = UART_STOP_BIT_1;   break;
    case 1:  stop_len = UART_STOP_BIT_1_5; break;
    case 2:  stop_len = UART_STOP_BIT_2;   break;
    default: stop_len = UART_STOP_BIT_1;   break;
    }

    UART_SetLineConfig(UART0, LineCfg.u32DTERate, data_len, parity, stop_len);

    // Re-enable UART interrupt
    NVIC_EnableIRQ(UART0_IRQn);
    */
}


uint USB_VCP_canSend(void)
{
    return (vcom.tx_read == vcom.tx_write);
}

uint USB_VCP_send(uint8_t *buffer, uint32_t len, uint32_t timeout)
{
    if(len > TX_BUFF_SIZE) len = TX_BUFF_SIZE;

    memcpy(vcom.tx_buff, buffer, len);
    vcom.tx_write = len;
    vcom.tx_read = 0;

    USB_VCP_sendPack();

    uint tx_read_save = vcom.tx_read;

    uint start = mp_hal_ticks_ms();
    while((mp_hal_ticks_ms() - start) < timeout) {
        if(tx_read_save != vcom.tx_read) {
            tx_read_save = vcom.tx_read;

            start = mp_hal_ticks_ms();
        }

        //if(vcom.tx_read == vcom.tx_write) break;  at this moment, data hasn't sent out
    }

    return 1;
}

void USB_VCP_sendPack(void)
{
    uint n = vcom.tx_write - vcom.tx_read;

    if(n == 0)
    {
        /* Prepare a zero packet if previous packet size is EP2_MAX_PKT_SIZE and
           no more data to send at this moment to note Host the transfer has been done */
        if(USBD_GET_PAYLOAD_LEN(EP2) == EP2_MAX_PKT_SIZE)
            USBD_SET_PAYLOAD_LEN(EP2, 0);

        return;
    }

    if(n > 64) n = 64;

    USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP2)), (uint8_t *)&vcom.tx_buff[vcom.tx_read], n);
    USBD_SET_PAYLOAD_LEN(EP2, n);

    vcom.tx_read += n;
}

uint USB_VCP_canRecv(void)
{
    return (vcom.rx_read != vcom.rx_write);
}

uint USB_VCP_recv(uint8_t *buffer, uint32_t len, uint32_t timeout)
{
    uint rcvd = 0;

    uint start = mp_hal_ticks_ms();
    while((mp_hal_ticks_ms() - start) < timeout)
    {
        if(vcom.rx_read < vcom.rx_write)
        {
            uint n = vcom.rx_write - vcom.rx_read;
            if(n > len) n = len;

            memcpy(&buffer[rcvd], &vcom.rx_buff[vcom.rx_read], n);
            vcom.rx_read += n;
            rcvd += n;
        }
        else if(vcom.rx_write < vcom.rx_read)
        {
            uint n = RX_BUFF_SIZE - vcom.rx_read;
            if(n > len) n = len;

            memcpy(&buffer[rcvd], &vcom.rx_buff[vcom.rx_read], n);
            vcom.rx_read += n;
            rcvd += n;

            if(vcom.rx_read == RX_BUFF_SIZE) vcom.rx_read = 0;
        }

        if(rcvd == len) break;
    }

    return rcvd;
}
