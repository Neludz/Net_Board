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


#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "limits.h"

#include "IO.h"
#include "stm32f1xx_hal.h"

#include "emfat.h"

#include "usb_device.h"

//======================Variables========================================
extern uint16_t MBbuf_main[];


volatile uint32_t Error=0;
int32_t Time_Blink_On = TIME_ON_NO_USB_MS;
int32_t Time_Blink_Off = TIME_OFF_NO_USB_MS;

extern emfat_t emfat;
extern  emfat_entry_t entries[];

//======================Task========================================

//-------------------------------------------------------------------------

void vBlinker (void *pvParameters)
{
    for (uint32_t i=0; i<(START_BLINK_TIME_MS/START_BLINK_PERIOD_MS); i++)
    {
        IO_SetLine(io_LED,ON);
        vTaskDelay((START_BLINK_PERIOD_MS>>1)/portTICK_RATE_MS);
        IO_SetLine(io_LED,OFF);
        vTaskDelay((START_BLINK_PERIOD_MS>>1)/portTICK_RATE_MS);
    }
    while(1)
    {
        IO_SetLine(io_LED,ON);
        vTaskDelay(Time_Blink_On/portTICK_RATE_MS);
        IO_SetLine(io_LED,OFF);
        vTaskDelay(Time_Blink_Off/portTICK_RATE_MS);

#ifdef DEBU_USER
        printf ( "1 sec \n" );
#endif
    }
}
//-------------------------------------------------------------------------

//======================Function========================================

//-------------------------------------------------------------------------

void Set_Time_For_Blink (uint32_t On_Timer, uint32_t Off_Timer)
{
    Time_Blink_On = On_Timer;
    Time_Blink_Off = Off_Timer;
}

//-------------------------------------------------------------------------

void flash_btock(void)
{
    if (!(FLASH->OBR & FLASH_OBR_RDPRT))
    {
        mh_Factory();	//when first start -> set MBbuf

        FLASH->KEYR = FLASH_KEY1;
        FLASH->KEYR = FLASH_KEY2;

        FLASH->OPTKEYR = FLASH_KEY1;
        FLASH->OPTKEYR = FLASH_KEY2;
        FLASH->CR |= FLASH_CR_OPTER;
        FLASH->CR|= FLASH_CR_STRT;
        while ((FLASH->SR & FLASH_SR_BSY) != 0 );

        FLASH->CR |= FLASH_CR_LOCK;
    }
}
//-------------------------------------------------------------------------

void Init_IWDG(uint16_t tw) // Параметр tw от 7мс до 26200мс
{
// Для IWDG_PR=7 Tmin=6,4мс RLR=Tмс*40/256
    IWDG->KR=0x5555; // Ключ для доступа к таймеру
    IWDG->PR=7; // Обновление IWDG_PR
    IWDG->RLR=tw*40/256; // Загрузить регистр перезагрузки
    IWDG->KR=0xAAAA; // Перезагрузка
    IWDG->KR=0xCCCC; // Пуск таймера
}
//-------------------------------------------------------------------------

// Функция перезагрузки сторожевого таймера IWDG

void IWDG_res(void)
{
    IWDG->KR=0xAAAA; // Перезагрузка
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
    ClockInit();//SystemInit();  // Фукнция CMSIS которая установит тактовую частоту
    IO_Init();
   // flash_btock();
    emfat_init(&emfat, "NB", entries);
    mh_Buf_Init();
    MX_USB_DEVICE_Init();
   // Init_IWDG(WATCH_DOG_TIME_MS);

    if(pdTRUE != xTaskCreate(vBlinker,	"Blinker", 	configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL)) ERROR_ACTION(TASK_NOT_CREATE,0);

    mh_Modbus_Init();   //create task for modbus

    /*	start OS	*/
#ifdef DEBU_USER
    printf ( "[ INFO ] Program start now\n" );
#endif
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

