#include <optic_data_sys.h>
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

extern uint16_t mb_buf_main[];
OPTIC_INSTANCE_DEF(optic_data, OPTIC_CHANNELS_LIST, OPTIC_CHANNEL_COUNT_PHY, &mb_buf_main[MB_OPTIC_START_REG]);
//-------------------------------------------------------------------------
// user interrupt
void USART1_IRQHandler (void)
{
    uint16_t cnt;
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
    (void) cnt;
    if (USART1->SR & (USART_SR_FE | USART_SR_ORE | USART_SR_NE))
    {
        cnt = USART1->DR;
        optic_data.frame_error = OPT_RET_DATA_ERROR;
    }
    else if (LL_USART_IsActiveFlag_RXNE(USART1))
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

    if (mb_buf_main[Reg_54_Optic_Mode] == MB_MODE_LEGACY)
        LL_USART_ConfigCharacter(USART1, LL_USART_DATAWIDTH_8B, LL_USART_PARITY_NONE, LL_USART_STOPBITS_1);
    else
        LL_USART_ConfigCharacter(USART1, LL_USART_DATAWIDTH_9B, LL_USART_PARITY_NONE, LL_USART_STOPBITS_1);

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
    uint32_t i = 0, ulNotifiedValue;
    IO_SetLine(io_Mul_E, LOW);
    BaseType_t notify_ret;
    IO_Uart_Optic_Init();
    while(1)
    {
        // check if one size error when multiplexer switch on a middle of the frame.
        if (optic_data.frame_error != OPT_RET_SIZE_ERROR && optic_data.error_count[i] != 1)
        {
            i++;
            if (i >= OPTIC_CHANNEL_COUNT_PHY)
                i=0;
            set_multiplexer_channel(i);
            vTaskDelay(OPTIC_WAIT_BEFORE_UART_MS/portTICK_RATE_MS);
        }
        optic_data.main_state = OPT_STATE_WAIT;
        notify_ret = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, OPTIC_TIME_OUT_MODERN_MS/portTICK_RATE_MS);
        optic_data.main_state = OPT_STATE_PARS;
        if(notify_ret == pdFALSE)
        {
            optic_data.frame_error = OPT_RET_TIMEOUT;
        }
        optic_parse_modern((OpticStruct_t*)&optic_data, i);
    }
}
//-------------------------------------------------------------------------
void v_optic_legacy_task (void *pvParameters)
{
    uint32_t i=0, ulNotifiedValue;
    BaseType_t notify_ret;
    IO_SetLine(io_Mul_E, LOW);
    IO_Uart_Optic_Init();
    xTimerStart(multiplexer_timer_handle, 0);
    while(1)
    {
        optic_data.main_state = OPT_STATE_WAIT;
        notify_ret = xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, OPTIC_TIME_OUT_LEGACY_MS/portTICK_RATE_MS );
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
            }
            set_multiplexer_channel(i);
            vTaskDelay(OPTIC_WAIT_BEFORE_UART_MS/portTICK_RATE_MS);
        }
    }
}
//-------------------------------------------------------------------------
void multiplexer_timer_callback (xTimerHandle xTimer)
{
    xSemaphoreGive( xSemaphore );
}
//-------------------------------------------------------------------------
// create task
void optic_init(void)
{
    if(mb_buf_main[Reg_54_Optic_Mode] == MB_MODE_LEGACY)
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
