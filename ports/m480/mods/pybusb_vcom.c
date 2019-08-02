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

    uint len = RX_BUFF_SIZE - vcom.rx_tail;
    if(len >= vcom.out_bytes)
    {
        USBD_MemCopy((uint8_t *)&vcom.rx_buff[vcom.rx_tail], (uint8_t *)vcom.out_ptr, vcom.out_bytes);
        vcom.rx_tail += vcom.out_bytes;

        if(vcom.rx_tail == RX_BUFF_SIZE) vcom.rx_tail = 0;
    }
    else
    {
        USBD_MemCopy((uint8_t *)&vcom.rx_buff[vcom.rx_tail], (uint8_t *)vcom.out_ptr, len);

        if(vcom.rx_head - 1 >= vcom.out_bytes - len)
        {
            USBD_MemCopy((uint8_t *)&vcom.rx_buff[0], ((uint8_t *)vcom.out_ptr) + len, vcom.out_bytes - len);
            vcom.rx_tail = vcom.out_bytes - len;
        }
        else
        {
            USBD_MemCopy((uint8_t *)&vcom.rx_buff[0], ((uint8_t *)vcom.out_ptr) + len, vcom.rx_head - 1);
            vcom.rx_tail = vcom.rx_head - 1;
        }
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
    vcom.rx_head = 0;
    vcom.rx_tail = 0;

    vcom.tx_bytes = 0;
    vcom.tx_head = 0;
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
    return (vcom.tx_head == vcom.tx_bytes);
}

uint USB_VCP_send(uint8_t *buffer, uint32_t len, uint32_t timeout)
{
    memcpy(vcom.tx_buff, buffer, len);
    vcom.tx_bytes = len;
    vcom.tx_head = 0;

    USB_VCP_sendPack();

    uint startTime = mp_hal_ticks_ms();
    while((mp_hal_ticks_ms() - startTime) < timeout) {
        if(vcom.tx_head == vcom.tx_bytes) break;
    }

    return vcom.tx_head;
}

void USB_VCP_sendPack(void)
{
    uint len = vcom.tx_bytes - vcom.tx_head;

    if(len == 0)
    {
        /* Prepare a zero packet if previous packet size is EP2_MAX_PKT_SIZE and
           no more data to send at this moment to note Host the transfer has been done */
        if(USBD_GET_PAYLOAD_LEN(EP2) == EP2_MAX_PKT_SIZE)
            USBD_SET_PAYLOAD_LEN(EP2, 0);

        return;
    }

    if(len > EP2_MAX_PKT_SIZE) len = EP2_MAX_PKT_SIZE;

    USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP2)), (uint8_t *)&vcom.tx_buff[vcom.tx_head], len);
    USBD_SET_PAYLOAD_LEN(EP2, len);

    vcom.tx_head += len;
}

uint USB_VCP_canRecv(void)
{
    return (vcom.rx_head != vcom.rx_tail);
}

uint USB_VCP_recv(uint8_t *buffer, uint32_t len, uint32_t timeout)
{
    uint cnt;
    uint rcved = 0;

    uint startTime = mp_hal_ticks_ms();
    while((mp_hal_ticks_ms() - startTime) < timeout) {
        if(vcom.rx_head < vcom.rx_tail)
        {
            cnt = vcom.rx_tail - vcom.rx_head;
            if(cnt > len) cnt = len;

            memcpy(&buffer[rcved], &vcom.rx_buff[vcom.rx_head], cnt);
            vcom.rx_head += cnt;
            rcved += cnt;
        }
        else if(vcom.rx_tail < vcom.rx_head)
        {
            cnt = RX_BUFF_SIZE - vcom.rx_head;
            if(cnt > len) cnt = len;

            memcpy(&buffer[rcved], &vcom.rx_buff[vcom.rx_head], cnt);
            vcom.rx_head += cnt;
            rcved += cnt;

            if(vcom.rx_head == RX_BUFF_SIZE) vcom.rx_head = 0;
        }

        if(rcved == len) break;
    }

    return rcved;
}


void VCOM_TransferData(void)
{
    int32_t i, len;

    /* Check whether USB is ready for next packet or not */
    if(vcom.in_bytes == 0)
    {
        /* Check whether we have new COM Rx data to send to USB or not */
        if(vcom.rx_bytes)
        {
            len = vcom.rx_bytes;
            if(len > EP2_MAX_PKT_SIZE)
                len = EP2_MAX_PKT_SIZE;

            for(i = 0; i < len; i++)
            {
                vcom.in_buff[i] = vcom.rx_buff[vcom.rx_head++];
                if(vcom.rx_head >= RX_BUFF_SIZE)
                    vcom.rx_head = 0;
            }

            __set_PRIMASK(1);
            vcom.rx_bytes -= len;
            __set_PRIMASK(0);

            vcom.in_bytes = len;
            USBD_MemCopy((uint8_t *)(USBD_BUF_BASE + USBD_GET_EP_BUF_ADDR(EP2)), (uint8_t *)vcom.in_buff, len);
            USBD_SET_PAYLOAD_LEN(EP2, len);
        }
        else
        {

        }
    }

    /* Process the Bulk out data when bulk out data is ready. */
    if(vcom.out_ready && (vcom.out_bytes <= TX_BUFF_SIZE - vcom.tx_bytes))
    {


        __set_PRIMASK(1);

        __set_PRIMASK(0);

        vcom.out_bytes = 0;
        vcom.out_ready = 0; /* Clear bulk out ready flag */


    }

    /* Process the software Tx FIFO */
    if(vcom.tx_bytes)
    {
        /* Check if Tx is working */
        if((UART0->INTEN & UART_INTEN_THREIEN_Msk) == 0)
        {
            /* Send one bytes out */
            UART0->DAT = vcom.tx_buff[vcom.tx_head++];
            if(vcom.tx_head >= TX_BUFF_SIZE)
                vcom.tx_head = 0;

            __set_PRIMASK(1);
            vcom.tx_bytes--;
            __set_PRIMASK(0);

            /* Enable Tx Empty Interrupt. (Trigger first one) */
            UART0->INTEN |= UART_INTEN_THREIEN_Msk;
        }
    }
}
