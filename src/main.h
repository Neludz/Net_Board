/*
 * main.h
 *
 *  Created on: 21 мая 2019 г.
 *
 */

#ifndef MAIN_H_
#define MAIN_H_

#include "FreeRTOS.h"
#include "timers.h"

#define VERSION_NB			201
#define WATCH_DOG_TIME_MS		500

#define RESET_VALUE					0xA01	//2561
#define FACTORY_SET_VALUE 			0xB01	//2817

// blink time
#define START_BLINK_TIME_MS			1000
#define START_BLINK_PERIOD_MS		150
#define TIME_ON_USB_MS              950
#define TIME_OFF_USB_MS             50
#define TIME_ON_NO_USB_MS           50
#define TIME_OFF_NO_USB_MS 			950

#define M_MODBUS_TASK_PRIORITY         (tskIDLE_PRIORITY + 3)
#define M_MODBUS_TASK_STACK_SIZE       (configMINIMAL_STACK_SIZE*2)

void vBlinker (void *pvParameters);
void Init_IWDG(uint16_t tw);
void IWDG_res(void);
void Set_Time_For_Blink (uint32_t On_Timer, uint32_t Off_Timer);
void Error_Handler(void);
//uint32_t Main_Timer_Set(const uint32_t AddTimeMs);
//bool Timer_Is_Expired (const uint32_t Timer);

//=========================================================================

// Отдадочная затычка. Сюда можно вписать код обработки ошибок.
#define	ERROR_ACTION(CODE,POS)		do{}while(1)

#endif /* MAIN_H_ */
