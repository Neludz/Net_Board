/*
******************************************************************************
File:
Info:

******************************************************************************
*/

//======================Includes========================================
#include <stdio.h>
#include "main.h"
#include <modbus_reg.h>
#include <modbus_hard.h>

#include "clock.h"

#include <stdbool.h>
#include "inttypes.h"
#include "system_stm32f1xx.h"
#include "stm32f1xx.h"
#include <stm32f1xx_ll_bus.h>
#include <stm32f1xx_ll_usart.h>

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "limits.h"
#include "stm32f103xb.h"
#include "IO.h"
#include "optic_data.h"
#include "stm32f1xx_hal.h"

#include "emfat.h"

#include "usb_device.h"

#define LOG_MODULE  MAIN
#include "log_nel.h"
//====================== Variables ======================
extern uint16_t MBbuf_main[];

volatile uint32_t Error=0;
int32_t Time_Blink_On = TIME_ON_NO_USB_MS;
int32_t Time_Blink_Off = TIME_OFF_NO_USB_MS;

extern emfat_t emfat;
extern  emfat_entry_t entries[];

//====================== Interrupt ======================

//====================== Task ======================

//-------------------------------------------------------------------------

void vBlinker (void *pvParameters)
{
    for (uint32_t i=0; i<(START_BLINK_TIME_MS/START_BLINK_PERIOD_MS); i++)
    {
        IO_SetLine(io_LED, ON);
        vTaskDelay((START_BLINK_PERIOD_MS>>1)/portTICK_RATE_MS);
        IO_SetLine(io_LED, OFF);
        vTaskDelay((START_BLINK_PERIOD_MS>>1)/portTICK_RATE_MS);
        IO_IWDG_res();
    }
    while(1)
    {
        IO_SetLine(io_LED, ON);
        vTaskDelay(Time_Blink_On/portTICK_RATE_MS);
        IO_SetLine(io_LED, OFF);
        vTaskDelay(Time_Blink_Off/portTICK_RATE_MS);
        IO_IWDG_res();
    }
}

void vPrint (void *pvParameters)
{
    LOG_INFO ("vPrint");
    while(1)
    {
        vTaskDelay(1500/portTICK_RATE_MS);
    }
}

//====================== Function ======================


//-------------------------------------------------------------------------
void Set_Time_For_Blink (uint32_t On_Timer, uint32_t Off_Timer)
{
    Time_Blink_On = On_Timer;
    Time_Blink_Off = Off_Timer;
}

/*
**===========================================================================
**
**  Abstract: main program
**
**===========================================================================
*/

int main(void)
{
    ClockInit();
    IO_Init();
    mh_buf_init();
#ifndef DEBUG_TARGET
    IO_flash_btock();
#endif

    emfat_init(&emfat, "NB", entries);
    MX_USB_DEVICE_Init();
#ifndef DEBUG_TARGET
    IO_Init_IWDG(WATCH_DOG_TIME_MS);
#endif
    optic_init();

    if(pdTRUE != xTaskCreate(vBlinker, "Blinker", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL)) ERROR_ACTION(TASK_NOT_CREATE,0);
#ifdef DEBUG_TARGET
    if(pdTRUE != xTaskCreate(vPrint, "Print", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL)) ERROR_ACTION(TASK_NOT_CREATE,0);
#endif
    mh_modbus_init();   //create task for modbus

    /*	start OS	*/

    LOG_INFO ("Program start now");

    vTaskStartScheduler();
    return 0;
}
void Error_Handler(void)
{
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

void vApplicationIdleHook( void )
{
}

void vApplicationMallocFailedHook( void )
{
    for( ;; );
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, char * pcTaskName )
{
    ( void ) pcTaskName;
    ( void ) xTask;
    for( ;; );
}

void vApplicationTickHook( void )
{
}

