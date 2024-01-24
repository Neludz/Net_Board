#include <optic_data.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#include <stm32f1xx.h>
#include <stm32f1xx_ll_bus.h>
#include <stm32f1xx_ll_usart.h>
#include <IO.h>
#include <main.h>
#include <modbus_config.h>
#include <modbus_reg.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "limits.h"



//-------------------------------------------------------------------------
// user variable
TaskHandle_t optic_task_handle;
TimerHandle_t multiplexer_timer_handle;
SemaphoreHandle_t xSemaphore;

extern uint16_t MBbuf_main[];
OPTIC_INSTANCE_DEF(optic_data, OPTIC_CHANNELS_LIST, OPTIC_CHANNEL_COUNT_PHY, &MBbuf_main[MB_OPTIC_START_REG]);
//-------------------------------------------------------------------------
// user interrupt
void USART1_IRQHandler (void)
{
    uint16_t cnt;
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    (void) cnt;
    if (LL_USART_IsActiveFlag_RXNE(USART1))
    {
        switch (optic_data.main_state)
        {
        case OPT_STATE_WAIT:
            if( optic_data.idle_state == OPT_IDLE_DETECT )
            {
                optic_data.frame_error = OPT_RET_OK;
                optic_data.count_data = 1;  // +1
                optic_data.main_state = OPT_STATE_RCVE;
                optic_data.buf[0] = LL_USART_ReceiveData9(USART1);
            }
            break;
        case OPT_STATE_RCVE:
            if (optic_data.count_data < OPTIC_BUF_SIZE)
                optic_data.buf[optic_data.count_data++] = LL_USART_ReceiveData9(USART1);
            else
                optic_data.frame_error = OPT_RET_SIZE_ERROR;
            break;
        default:
            break;
        }
        LL_USART_ClearFlag_RXNE(USART1);

        if (optic_data.idle_state == OPT_IDLE_DETECT)
            optic_data.idle_state = OPT_NO_IDLE;
    }
    if (USART1->SR & USART_SR_IDLE)
    {
        cnt = LL_USART_ReceiveData9(USART1);
        optic_data.idle_state = OPT_IDLE_DETECT ;
        if (optic_data.main_state == OPT_STATE_RCVE)
        {
            optic_data.main_state = OPT_STATE_PARS;
            xTaskNotifyFromISR(optic_task_handle, TASK_NOTIFY_INTERRUPT, eSetBits, &xHigherPriorityTaskWoken);
        }
    }
}
//-------------------------------------------------------------------------
//user function
static void IO_Uart_Optic_Init(void)
{
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);    //USART1 Clock ON
    LL_USART_SetBaudRate(USART1, SYSCLK_FREQ, OPTIC_UART_BAUD);
    LL_USART_SetTransferDirection(USART1, LL_USART_DIRECTION_RX);
    LL_USART_EnableIT_IDLE(USART1);
    LL_USART_EnableIT_RXNE(USART1);
    LL_USART_ConfigCharacter(USART1, LL_USART_DATAWIDTH_9B, LL_USART_PARITY_NONE,
                             LL_USART_STOPBITS_1);
    LL_USART_Enable(USART1);

    NVIC_SetPriority(USART1_IRQn,14);
    NVIC_EnableIRQ (USART1_IRQn);
}
//-------------------------------------------------------------------------
static void set_multiplexer_channel (uint32_t channel)
{
    IO_SetLine(io_Mul_S0, (optic_data.multiplexer_channels_order[channel] & 0x01));
    IO_SetLine(io_Mul_S1, (optic_data.multiplexer_channels_order[channel] & 0x02));
    IO_SetLine(io_Mul_S2, (optic_data.multiplexer_channels_order[channel] & 0x04));
}
//-------------------------------------------------------------------------
// user task
void v_optic_task (void *pvParameters)
{
    uint32_t i, ulNotifiedValue;
    IO_SetLine(io_Mul_E, LOW);
    BaseType_t notify_ret;
    IO_Uart_Optic_Init();
    while(1)
    {
        for(i = 0; i<OPTIC_CHANNEL_COUNT_PHY; i++)
        {
            set_multiplexer_channel(i);
            vTaskDelay(OPTIC_WAIT_BEFORE_UART_MS/portTICK_RATE_MS);
            optic_data.main_state = OPT_STATE_WAIT;
            notify_ret = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, OPTIC_TIME_OUT_MS/portTICK_RATE_MS);
            optic_data.main_state = OPT_STATE_PARS;
            if(notify_ret == pdFALSE)
                optic_data.frame_error = OPT_RET_TIMEOUT;

            optic_parse_modern((OpticStruct_t*)&optic_data, i);
        }
    }
}
//-------------------------------------------------------------------------
void v_optic_legacy_task (void *pvParameters)
{
    uint32_t i, ulNotifiedValue;
    BaseType_t notify_ret;
    IO_SetLine(io_Mul_E, LOW);
    IO_Uart_Optic_Init();
    xTimerStart(multiplexer_timer_handle, 0);
    i = 0;
    while(1)
    {
        optic_data.main_state = OPT_STATE_WAIT;
        notify_ret = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, OPTIC_TIME_OUT_MS/portTICK_RATE_MS );
        optic_data.main_state = OPT_STATE_PARS;
        if(notify_ret == pdFALSE)
                optic_data.frame_error = OPT_RET_TIMEOUT;

        optic_parse_legacy((OpticStruct_t*)&optic_data, i);

        if( xSemaphoreTake(xSemaphore, 0) == pdTRUE )
        {
            i++;
            if(i >= OPTIC_LEGACY_CHANNEL_COUNT_PHY)
            {
                i = 0;
                optic_legacy_periodic_lan_error_set(&optic_data);
                //printf("_______optic_legacy_periodic_lan_error_set \n");
            }
            set_multiplexer_channel(i);
            vTaskDelay(OPTIC_WAIT_BEFORE_UART_MS/portTICK_RATE_MS);
          //  printf("_______xSemaphoreTake \n");

        }
    }
}
//-------------------------------------------------------------------------
void multiplexer_timer_callback (xTimerHandle xTimer)
{
   // printf("xSemaphoreGive \n");
    xSemaphoreGive( xSemaphore );
}
//-------------------------------------------------------------------------
// create task
void optic_init(void)
{
    if(MBbuf_main[Reg_54_Optic_Mode] == MB_MODE_LEGACY)
    {
        if(pdTRUE != xTaskCreate(v_optic_legacy_task, "OpticLegacy", configMINIMAL_STACK_SIZE*2, NULL, tskIDLE_PRIORITY + 1, &optic_task_handle)) ERROR_ACTION(TASK_NOT_CREATE,0);
        multiplexer_timer_handle = xTimerCreate( "Timer", LEGACY_MULTIPLEXER_TIMER_MS/portTICK_RATE_MS, pdTRUE, ( void * ) 0, multiplexer_timer_callback);
        vSemaphoreCreateBinary( xSemaphore );
    }

    else
    {
        if(pdTRUE != xTaskCreate(v_optic_task, "optic", configMINIMAL_STACK_SIZE*2, NULL, tskIDLE_PRIORITY + 1, &optic_task_handle)) ERROR_ACTION(TASK_NOT_CREATE,0);
    }
}
//-------------------------------------------------------------------------
// system part
const unsigned char crc_array[256] =
{
    0x00, 0x5e, 0xbc, 0xe2, 0x61, 0x3f, 0xdd, 0x83,
    0xc2, 0x9c, 0x7e, 0x20, 0xa3, 0xfd, 0x1f, 0x41,
    0x9d, 0xc3, 0x21, 0x7f, 0xfc, 0xa2, 0x40, 0x1e,
    0x5f, 0x01, 0xe3, 0xbd, 0x3e, 0x60, 0x82, 0xdc,
    0x23, 0x7d, 0x9f, 0xc1, 0x42, 0x1c, 0xfe, 0xa0,
    0xe1, 0xbf, 0x5d, 0x03, 0x80, 0xde, 0x3c, 0x62,
    0xbe, 0xe0, 0x02, 0x5c, 0xdf, 0x81, 0x63, 0x3d,
    0x7c, 0x22, 0xc0, 0x9e, 0x1d, 0x43, 0xa1, 0xff,
    0x46, 0x18, 0xfa, 0xa4, 0x27, 0x79, 0x9b, 0xc5,
    0x84, 0xda, 0x38, 0x66, 0xe5, 0xbb, 0x59, 0x07,
    0xdb, 0x85, 0x67, 0x39, 0xba, 0xe4, 0x06, 0x58,
    0x19, 0x47, 0xa5, 0xfb, 0x78, 0x26, 0xc4, 0x9a,
    0x65, 0x3b, 0xd9, 0x87, 0x04, 0x5a, 0xb8, 0xe6,
    0xa7, 0xf9, 0x1b, 0x45, 0xc6, 0x98, 0x7a, 0x24,
    0xf8, 0xa6, 0x44, 0x1a, 0x99, 0xc7, 0x25, 0x7b,
    0x3a, 0x64, 0x86, 0xd8, 0x5b, 0x05, 0xe7, 0xb9,
    0x8c, 0xd2, 0x30, 0x6e, 0xed, 0xb3, 0x51, 0x0f,
    0x4e, 0x10, 0xf2, 0xac, 0x2f, 0x71, 0x93, 0xcd,
    0x11, 0x4f, 0xad, 0xf3, 0x70, 0x2e, 0xcc, 0x92,
    0xd3, 0x8d, 0x6f, 0x31, 0xb2, 0xec, 0x0e, 0x50,
    0xaf, 0xf1, 0x13, 0x4d, 0xce, 0x90, 0x72, 0x2c,
    0x6d, 0x33, 0xd1, 0x8f, 0x0c, 0x52, 0xb0, 0xee,
    0x32, 0x6c, 0x8e, 0xd0, 0x53, 0x0d, 0xef, 0xb1,
    0xf0, 0xae, 0x4c, 0x12, 0x91, 0xcf, 0x2d, 0x73,
    0xca, 0x94, 0x76, 0x28, 0xab, 0xf5, 0x17, 0x49,
    0x08, 0x56, 0xb4, 0xea, 0x69, 0x37, 0xd5, 0x8b,
    0x57, 0x09, 0xeb, 0xb5, 0x36, 0x68, 0x8a, 0xd4,
    0x95, 0xcb, 0x29, 0x77, 0xf4, 0xaa, 0x48, 0x16,
    0xe9, 0xb7, 0x55, 0x0b, 0x88, 0xd6, 0x34, 0x6a,
    0x2b, 0x75, 0x97, 0xc9, 0x4a, 0x14, 0xf6, 0xa8,
    0x74, 0x2a, 0xc8, 0x96, 0x15, 0x4b, 0xa9, 0xf7,
    0xb6, 0xe8, 0x0a, 0x54, 0xd7, 0x89, 0x6b, 0x35,
};
//-------------------------------------------------------------------------
unsigned char dallas_crc8(const unsigned int size, uint16_t *buf)
{
    unsigned char crc = 0;
    for ( unsigned int i = 0; i < size; ++i )
    {
        crc = crc_array[(0xFF & buf[i]) ^ crc];
    }
    return crc;
}
//-------------------------------------------------------------------------
static bool invalid_frame_modern(OpticStruct_t *opt_data)
{
    if( OPT_RET_OK != opt_data->frame_error)
        return true;
    if( 7 != opt_data->count_data)
        return true;
    if(dallas_crc8( opt_data->count_data, opt_data->buf))
        return true;
    return false;
}
//-------------------------------------------------------------------------
void optic_parse_modern(OpticStruct_t *opt_data, uint32_t opt_channel)
{
    if (invalid_frame_modern(opt_data))
    {
        opt_data->error_count[opt_channel]++;
        if (opt_data->error_count[opt_channel] >= OPTIC_ERROR_MAX)
        {
            opt_data->error_count[opt_channel] = OPTIC_ERROR_MAX;
            opt_data->mb_reg_p[(opt_channel * 4) + 3] |= 0x01; // lan_error
            opt_data->error_lan_flag[opt_channel] = 1;
            // U1
            opt_data->mb_reg_p[(opt_channel * 4)] = 0;
            // U2
            opt_data->mb_reg_p[(opt_channel * 4) + 1] = 0;
            // t
            opt_data->mb_reg_p[(opt_channel * 4) + 2] = 0;
        }
    }
    else
    {
        // lan OK
        opt_data->error_lan_flag[opt_channel] = 0;
        opt_data->error_count[opt_channel] = 0;
        opt_data->mb_reg_p[(opt_channel * 4) + 3] &= ~(0x01);
        // U1
        opt_data->mb_reg_p[(opt_channel * 4)] = (opt_data->buf[2]>>1) | ((opt_data->buf[1]>>1) << 7);
        // U2
        opt_data->mb_reg_p[(opt_channel * 4) + 1] = (opt_data->buf[4]>>1) | ((opt_data->buf[3]>>1) << 7);
        // t
        opt_data->mb_reg_p[(opt_channel * 4) + 2] = ((opt_data->buf[5] & ~(0x01)) | ((opt_data->buf[0] >> 2) & 0x01)) - 60;
        if(opt_data->buf[0] & (1<<3))  opt_data->mb_reg_p[(opt_channel * 4) + 3] |= (0x01<<1);
        else opt_data->mb_reg_p[(opt_channel * 4) + 3] &= ~(0x01<<1);

        if ((opt_data->buf[0]>>4 != (opt_channel+1)) && (opt_data->buf[0]>>4 != (opt_channel+7)))
              opt_data->mb_reg_p[(opt_channel * 4) + 3] |= 0x01<<4;
        else
        opt_data->mb_reg_p[(opt_channel * 4) + 3] &= ~(0x01<<4);
    }
}
//-------------------------------------------------------------------------
static bool invalid_frame_legacy(OpticStruct_t *opt_data)
{
    uint8_t crc = 0, addr;
    if( OPT_RET_OK != opt_data->frame_error)
        return true;
    if( 5 != opt_data->count_data)
        return true;
    // ----- CRC
    for (uint32_t i=0; i < 5; i++)
    {
        crc = crc ^ (opt_data->buf[i] & 0xFF);
    }
    crc = crc ^ 0x80;
    if(crc != 0)
        return true;
    addr = ((opt_data->buf[0]>>3)&0x0F);
    if(addr > LEGACY_CHANNEL_COUNT || !(addr))
        return true;
    //-----
    return false;
}

//-------------------------------------------------------------------------
void optic_parse_legacy(OpticStruct_t *opt_data, uint32_t opt_channel)
{
    uint32_t address, delta;

    if (!invalid_frame_legacy(opt_data))
    {
        //printf("!invalid_frame_legacy+++++++%d \n", opt_channel);
        // lan error coun reset
        address = opt_data->buf[0]>>3;
        if(address <= LEGACY_CHANNEL_COUNT && address != 0)
        {
            opt_data->legacy_error_count[address-1] = 0; // from zero
            //data set
            //U
            delta = ((opt_data->buf[1]<<3)&0xF0) | ((opt_data->buf[2]>>1)&0x0F);

            opt_data->mb_reg_p[address-1] = (delta<<8) | ((0xFF - delta));
            //T
            delta = address>>1;
            if (address & 0x01)
            {
                opt_data->mb_reg_p[12 + delta] &= 0x00FF;
                opt_data->mb_reg_p[12 + delta] |= (0xFF00 & (opt_data->buf[3]<<8));
            }
            else
            {
                opt_data->mb_reg_p[11 + delta] &= (0xFF00);
                opt_data->mb_reg_p[11 + delta] |= (0x00FF & opt_data->buf[3]);
            }
        }
    }
}
//-------------------------------------------------------------------------
void optic_legacy_periodic_lan_error_set(OpticStruct_t *opt_data)
{
    for(uint32_t i=0; i<LEGACY_CHANNEL_COUNT; i++)
    {
        (opt_data->legacy_error_count[i])++;
        if(opt_data->legacy_error_count[i] >= OPTIC_LEGACY_ERROR_MAX)
        {
            opt_data->legacy_error_count[i] = OPTIC_LEGACY_ERROR_MAX;
            if (i & 0x01) // lan_error -> set zero
                opt_data->mb_reg_p[12 + (i>>1)] &= 0xFF00;
            else
                opt_data->mb_reg_p[12 + (i>>1)] &= 0x00FF;
            opt_data->mb_reg_p[i] = 0;
        }
    }
}

