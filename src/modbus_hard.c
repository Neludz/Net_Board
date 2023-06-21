#include "modbus_hard.h"
#include "modbus.h"
#include "modbus_reg.h"

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
#include <string.h>

#include "usbd_cdc_if.h"

//-----------------------------------------------------------------------
// Variable
//-----------------------------------------------------------------------

uint16_t MBbuf_main[MB_NUM_BUF]= {};
extern uint32_t Error;

xQueueHandle xModbusQueue;
TimerHandle_t rs485_timer_handle;
TaskHandle_t m_modbus_task_handle;

volatile MBStruct_t MB_RS485;
volatile MBStruct_t MB_USB;

uint8_t RS485_MB_Buf[MB_FRAME_MAX];
uint8_t USB_MB_Buf[MB_FRAME_MAX];

const uint16_t Baud_rate[BAUD_NUMBER]= RS_485_BAUD_LIST;

extern const RegParameters_t MBRegParam[];
//-----------------------------------------------------------------------
// Task function
//-----------------------------------------------------------------------

void mh_task_Modbus (void *pvParameters)
{
    MBStruct_t *st_mb;
    vTaskDelay(3000);
    while(1)
    {
        xQueueReceive(xModbusQueue,&st_mb,portMAX_DELAY);
        mb_parsing((MBStruct_t*) st_mb);
        taskYIELD();
    }
}

//-----------------------------------------------------------------------
// Function
//-----------------------------------------------------------------------
void mh_EnableTransmission(const bool Enable)
{
   // if (Enable)
      //  IO_SetLine(io_RS485_Switch,HIGH);
//    else
      //  IO_SetLine(io_RS485_Switch,LOW);
}

void USART3_IRQHandler (void)
{


}

// Callback for usb com
void mh_USB_Recieve(uint8_t *USB_buf, uint16_t len)	//interrupt	function
{
    if (mb_instance_idle_check((MBStruct_t*)&MB_USB)==MB_OK)
    {
        if(len>MB_FRAME_MAX)
        {
            len=MB_FRAME_MAX;
        }
        MB_USB.mb_state=MB_STATE_PARS;
        MB_USB.mb_index=(len);
        memcpy (MB_USB.p_mb_buff,USB_buf,len);
        MBStruct_t *st_mb=(MBStruct_t*)&MB_USB;
        xQueueSend(xModbusQueue, &st_mb, 0);
    }
}

// Init modbus
void mh_Modbus_Init(void)
{
    //create queue
    xModbusQueue=xQueueCreate(3,sizeof(MBStruct_t *));

    //create modbus task
    if(pdTRUE != xTaskCreate(mh_task_Modbus, "RS485", MODBUS_TASK_STACK_SIZE, NULL, MODBUS_TASK_PRIORITY, &m_modbus_task_handle)) ERROR_ACTION(TASK_NOT_CREATE, 0);

    mh_USB_Init();
}

void mh_USB_Init(void)
{
    MB_USB.p_write = MBbuf_main;
    MB_USB.p_read = MBbuf_main;
    MB_USB.reg_read_last=MB_NUM_BUF-1;
    MB_USB.reg_write_last=MB_NUM_BUF-1;
    MB_USB.cb_state=MB_CB_FREE;
    MB_USB.er_frame_bad=EV_NOEVENT;
    MB_USB.slave_address=MB_ANY_ADDRESS;	//0==any address
    MB_USB.mb_state=MB_STATE_IDLE;
    MB_USB.p_mb_buff=&USB_MB_Buf[0];
    MB_USB.wr_callback = mh_Write_Eeprom;
    MB_USB.f_start_trans = mh_USB_Transmit_Start;
    MB_USB.f_start_receive = NULL;
}

void mh_RS485_Init(void)
{

}

void rs485_timer_callback (xTimerHandle xTimer)
{

}

void IO_Uart1_Init(void)
{


}

void mh_Write_Eeprom (void *mbb)
{

}

void mh_USB_Transmit_Start (void *mbb)
{
    MBStruct_t *st_mb;
    st_mb = (void*) mbb;
    CDC_Transmit_FS (st_mb->p_mb_buff, st_mb->response_size);
    MB_USB.mb_state=MB_STATE_IDLE;
}

void mh_Rs485_Transmit_Start (void *mbb)
{

}

void mh_Factory (void)
{

}

void mh_Buf_Init (void)
{

}
