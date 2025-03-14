#include "modbus_hard.h"
#include "modbus.h"
#include "modbus_reg.h"
#include <stm32f1xx_ll_bus.h>
#include <stm32f1xx_ll_usart.h>

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "IO.h"
#include "main.h"

#include <stdbool.h>
#include "inttypes.h"
#include <stdio.h>

#include "usbd_cdc_if.h"
#include <eeprom_emulation.h>

#define LOG_MODULE  MODBUS_HARD
#include "log_nel.h"
//-----------------------------------------------------------------------
// Variable
//-----------------------------------------------------------------------
uint16_t mb_buf_main[MB_NUM_BUF]= {};
xQueueHandle xModbusQueue;
TimerHandle_t rs485_timer_handle;
TaskHandle_t modbus_task_handle;
const uint32_t baud_rate[BAUD_NUMBER]= RS_485_BAUD_LIST;

//-----------------------------------------------------------------------
// Prototype
//-----------------------------------------------------------------------
static void mh_enable_transmission(const bool enable);
static void mh_rs485_transmit_start (mb_slave_t *p_instance);
static void rs485_timer_callback (xTimerHandle xTimer);
static void mh_usb_transmit_start (mb_slave_t *p_instance);
static void io_uart3_init(void);
//-----------------------------------------------------------------------
// create MODBUS instance
//-----------------------------------------------------------------------
MB_SLAVE_INSTANCE_DEF(mb_rs485,\
                      mb_buf_main,\
                      mh_write_eeprom,\
                      mh_rs485_transmit_start,\
                      NULL)

MB_SLAVE_INSTANCE_DEF(mb_usb,\
                      mb_buf_main,\
                      mh_write_eeprom,\
                      mh_usb_transmit_start,\
                      NULL)
//-----------------------------------------------------------------------
// Task function
//-----------------------------------------------------------------------
void mh_task_modbus (void *pvParameters)
{
    mb_slave_t *p_instance;
    vTaskDelay(3000);
    LOG_INFO ("start modbus task");
    while(1)
    {
        xQueueReceive(xModbusQueue, &p_instance,portMAX_DELAY);
        mb_parsing(p_instance);
        taskYIELD();
    }
}

//-----------------------------------------------------------------------
// Interrupt function
//-----------------------------------------------------------------------
void USART3_IRQHandler (void)
{
    uint8_t cnt;
    (void) cnt;
    if (USART3->SR & (USART_SR_FE | USART_SR_ORE | USART_SR_NE))
    {
        mb_rs485.mb_state = MB_STATE_IDLE;
        cnt = USART3->DR;
    }
    else if (USART3->SR & USART_SR_RXNE)
    {
        // xTimerResetFromISR(rs485_timer_handle, NULL);	// Timer reset anyway: received symbol means NO SILENCE
        if( MB_STATE_RCVE == mb_rs485.mb_state)
        {
            if(mb_rs485.mb_index >= MB_FRAME_MAX-1)
            {
                mb_rs485.er_frame_bad = EV_HAPPEND;	                 // This error will be processed later
                USART3->SR = ~USART_SR_RXNE;                         // Nothing more to do in RECEIVE state
            }
            else
            {
                mb_rs485.p_mb_buff[mb_rs485.mb_index++] = USART3->DR;	 // MAIN DOING: New byte to buffer
            }
        }
        else if(mbreg_instance_idle_check((mb_slave_t*)&mb_rs485)==MB_OK)
        {
            // 1-st symbol come!
            mb_rs485.p_mb_buff[0] = USART3->DR; 		// Put it to buffer
            mb_rs485.mb_index = 1;						// "Clear" the rest of buffer
            mb_rs485.er_frame_bad = EV_NOEVENT;			// New buffer, no old events
            mb_rs485.mb_state=MB_STATE_RCVE;				// MBMachine: begin of receiving the request
        }
        else
        {
            USART3->SR = ~USART_SR_RXNE;
        }
    }
    if (USART3->SR & USART_SR_TC)
    {
        USART3->SR = ~(USART_SR_TC);
        mb_rs485.mb_state = MB_STATE_IDLE;
        mh_enable_transmission(false);
    }
    if (USART3->SR & USART_SR_TXE)
    {
        if( MB_STATE_SEND == mb_rs485.mb_state)
        {
            if( mb_rs485.mb_index < mb_rs485.response_size)
            {
                USART3->DR = mb_rs485.p_mb_buff[mb_rs485.mb_index++];   //  sending of the next byte
            }
            else
            {
                mb_rs485.mb_state=MB_STATE_SENT;
                USART3->CR1 &= ~USART_CR1_TXEIE;
            }
        }
        else
        {
            mh_enable_transmission(false);
            USART3->CR1 &= ~USART_CR1_TXEIE;
        }
    }
    if (USART3->SR & USART_SR_IDLE)
    {
        cnt = USART3->DR;
        if (MB_STATE_RCVE == mb_rs485.mb_state)
        {
            xTimerResetFromISR(rs485_timer_handle, NULL);
        }
    }
}

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
static void mh_enable_transmission(const bool enable)
{
    if (enable)
        IO_SetLine(io_rs485_switch, HIGH);
    else
        IO_SetLine(io_rs485_switch, LOW);
}

//-----------------------------------------------------------------------
static void mh_rs485_transmit_start (mb_slave_t *p_instance)
{
    mh_enable_transmission(true);
    LL_USART_EnableIT_TXE(USART3);
}

//-----------------------------------------------------------------------
static void rs485_timer_callback (xTimerHandle xTimer)
{
    if( MB_STATE_RCVE == mb_rs485.mb_state)
    {
        // If we are receiving, it's the end event: t3.5
        mb_rs485.mb_state = MB_STATE_PARS;					// Begin parsing of a frame.
        mb_slave_t *st_mb = (mb_slave_t*)&mb_rs485;
        xQueueSend(xModbusQueue, &st_mb, 0);
    }
}

//-----------------------------------------------------------------------
static void mh_usb_transmit_start (mb_slave_t *p_instance)
{
    CDC_Transmit_FS (p_instance->p_mb_buff, p_instance->response_size);
    p_instance->mb_state=MB_STATE_IDLE;
}

//-----------------------------------------------------------------------
static void io_uart3_init(void)
{
    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART3);
    LL_USART_SetBaudRate(USART3, UART_FRIQ, baud_rate[mb_buf_main[reg_rs485_baud_rate]&0x3]);

    LL_USART_SetTransferDirection(USART3, LL_USART_DIRECTION_TX_RX);
    LL_USART_EnableIT_IDLE(USART3);
    LL_USART_EnableIT_RXNE(USART3);
    LL_USART_EnableIT_TC(USART3);
    //LL_USART_EnableIT_TXE(USART3);
    switch (mb_buf_main[reg_parity_stop_bits])
    {
    case NO_PARITY_1_STOP:
        break; //default setting
    case NO_PARITY_2_STOP:
        LL_USART_SetStopBitsLength(USART3, LL_USART_STOPBITS_2);
        break;
    case EVEN_PARITY_1_STOP:
        LL_USART_SetDataWidth(USART3, LL_USART_DATAWIDTH_9B);
        LL_USART_SetParity(USART3, LL_USART_PARITY_EVEN);
        break;
    case ODD_PARITY_1_STOP:
        LL_USART_SetDataWidth(USART3, LL_USART_DATAWIDTH_9B);
        LL_USART_SetParity(USART3, LL_USART_PARITY_ODD);
        break;
    default:
        break;
    }
    LL_USART_Enable(USART3);

    NVIC_SetPriority(USART3_IRQn,14);
    NVIC_EnableIRQ (USART3_IRQn);
}

//-----------------------------------------------------------------------
void mh_write_eeprom (mb_slave_t *p_instance)
{
    for (int32_t i = 0; i < (p_instance->cb_index); i++)
    {
        if((mbreg_option_check(i+(p_instance->cb_reg_start), CB_WR) == MB_OK))
        {
            EE_UpdateVariable(((p_instance->cb_reg_start)+i), p_instance->p_write[i+(p_instance->cb_reg_start)]);
        }
    }
}

//-----------------------------------------------------------------------
// Callback for usb com
void mh_usb_recieve(uint8_t *usb_buf, uint16_t len)	//interrupt	function
{
    if (mbreg_instance_idle_check((mb_slave_t*)&mb_usb)==MB_OK)
    {
        if(len > MB_FRAME_MAX)
        {
            len = MB_FRAME_MAX;
        }
        mb_usb.mb_state = MB_STATE_PARS;
        mb_usb.mb_index = len;
        memcpy(mb_usb.p_mb_buff, usb_buf, len);
        mb_slave_t *st_mb=(mb_slave_t*)&mb_usb;
        xQueueSendFromISR(xModbusQueue, &st_mb, 0);
    }
}

//-----------------------------------------------------------------------
void mh_modbus_init(void)
{
    //create queue
    xModbusQueue = xQueueCreate(3, sizeof(mb_slave_t *));
    //create modbus task
    if(pdTRUE != xTaskCreate(mh_task_modbus, "rs485", MODBUS_TASK_STACK_SIZE, NULL, MODBUS_TASK_PRIORITY, &modbus_task_handle)) ERROR_ACTION(TASK_NOT_CREATE, 0);
    mb_rs485.slave_address = IO_GetLineActive(io_addr0) | IO_GetLineActive(io_addr1) << 1 | IO_GetLineActive(io_addr2) << 2 | IO_GetLineActive(io_addr3) << 3;
    if(!mb_rs485.slave_address)
        mb_rs485.slave_address = mb_buf_main[reg_rs485_modbus_addr];
    rs485_timer_handle = xTimerCreate( "rs485", mb_buf_main[reg_rs485_ans_delay]/portTICK_RATE_MS, pdFALSE, NULL, rs485_timer_callback);
    io_uart3_init();
}

//-----------------------------------------------------------------------
void mh_factory (void)
{
    taskENTER_CRITICAL();
    for (int32_t i=0; i< MB_NUM_BUF; i++)
    {
        if (mbreg_option_check(i, CB_WR)==MB_OK)
        {
            mb_buf_main[i] = mbreg_get_param(i).default_value;
            EE_UpdateVariable(i, mb_buf_main[i]);
        }
    }
    taskEXIT_CRITICAL();
}

//-----------------------------------------------------------------------
void mh_buf_init (void)
{
    int32_t i=0;
    EEPRESULT stat;
    taskENTER_CRITICAL();
    for (i=0; i< MB_NUM_BUF; i++)
    {
        if(mbreg_option_check(i, CB_WR)==MB_OK)
        {
            stat = EE_ReadVariable(i, &mb_buf_main[i]);
            if((mbreg_limit_check(i, mb_buf_main[i])==MB_ERROR) || (stat!=RES_OK))
            {
                mb_buf_main[i]=mbreg_get_param(i).default_value;
                EE_UpdateVariable(i, mb_buf_main[i]);
            }
        }
    }
    taskEXIT_CRITICAL();
}

//-----------------------------------------------------------------------
EEPRESULT GetNextVirtAddrData(uint16_t VirtAddressLast, uint16_t *VirtAddressNext, uint16_t *NextData)
{
    uint32_t i;
    VirtAddressLast ++;

    for (i = VirtAddressLast ; i< MB_NUM_BUF ; i++)
    {
        if (mbreg_option_check(i, CB_WR) == MB_OK)
        {
            *VirtAddressNext = i;
            *NextData = mb_buf_main[i];
            return RES_OK;
        }
    }
    return RES_ERROR;
}
