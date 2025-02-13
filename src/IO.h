#ifndef _IO_H
#define _IO_H

#include <stdint.h>
#include <stdbool.h>
#include <stm32f1xx.h>

#define IN	  	  (0x00)    //MODE_1
#define OUT_10MHz (0x01)    //MODE_1
#define OUT_2MHz  (0x02)    //MODE_1
#define OUT_50MHz (0x03)    //MODE_1


#define OUT_PP   (0x00) // MODE_2 - General purpose output push-pull
#define OUT_OD   (0x04) // MODE_2 - General purpose output Open-drain
#define OUT_APP  (0x08) // MODE_2 - Alternate function output Push-pull
#define OUT_AOD  (0x0C) // MODE_2 - Alternate function output Open-drain

#define IN_ADC   (0x00)     //MODE_2
#define IN_HIZ   (0x04)     //MODE_2
#define IN_PULL  (0x08)     //MODE_2



typedef struct
{
    GPIO_TypeDef* GPIOx;
    uint16_t GPIO_Pin;
    uint8_t MODE;
    uint8_t DefState;
    uint8_t ActiveState;
} tGPIO_Line;


//------========IO_Start_Table========------
//	 NAME  					GPIOx   GPIO_Pin    MODE_1 		MODE_2	 DefState  ActiveState
#define IO_TABLE\
	X_IO(io_LED,			GPIOB,	12,			OUT_2MHz,	OUT_PP,		LOW,  	HIGH)	\
	X_IO(io_RX_Optic,       GPIOA,  10, 		IN,			IN_HIZ,  	HIGH,  	HIGH)	\
    X_IO(io_rs485_switch,	GPIOB,  1, 			OUT_2MHz,	OUT_PP,  	LOW,  	HIGH)	\
	X_IO(io_RX,             GPIOB,  11, 		IN,			IN_PULL,  	HIGH,  	HIGH)	\
	X_IO(io_TX,				GPIOB,  10,         OUT_50MHz,	OUT_APP, 	HIGH,  	HIGH)	\
	X_IO(io_addr0,			GPIOB,  13,         IN,			IN_HIZ,	    HIGH,  	LOW)	\
	X_IO(io_addr1,			GPIOB,  15,  		IN,			IN_HIZ,	    HIGH,  	LOW)	\
	X_IO(io_addr2,			GPIOA,  8,  		IN,			IN_HIZ,   	HIGH,  	LOW)	\
	X_IO(io_addr3,			GPIOB,  14,  		IN,			IN_HIZ,     HIGH,  	LOW)	\
	X_IO(io_Mul_E,			GPIOB,  7,  		OUT_2MHz,	OUT_PP,		LOW,  	HIGH)	\
    X_IO(io_Mul_S0,			GPIOB,  4,  		OUT_2MHz,	OUT_PP,		LOW,  	HIGH)	\
    X_IO(io_Mul_S1,			GPIOB,  5,  		OUT_2MHz,	OUT_PP,		LOW,  	HIGH)	\
    X_IO(io_Mul_S2,			GPIOB,  6,  		OUT_2MHz,	OUT_PP,		LOW,  	HIGH)	\


//USB pins init in default state
//------========IO_End_Table========------

typedef enum
{
#define X_IO(a,b,c,d,e,f,g)	a,
    IO_TABLE
#undef X_IO
    NUM_IO		//count
} tIOLine;

typedef enum
{
    OFF = 0,
    ON = 1,
    LOW = 0,
    HIGH =1,
} tIOState;


__STATIC_INLINE void IO_optic_uart_start(void)
{

}

__STATIC_INLINE void IO_optic_uart_stop(void)
{

}


void IO_Init(void);
void IO_SetLine(tIOLine Line, bool State);
bool IO_GetLine(uint8_t Input);
void IO_SetLineActive(tIOLine Line, bool State);
bool IO_GetLineActive(uint8_t Input);
void IO_ConfigLine(tIOLine Line, uint8_t Mode, uint8_t State);
void IO_delay_ms(uint32_t ms);
void IO_Init_IWDG(uint16_t tw);
void IO_IWDG_res(void);
void IO_flash_btock(void);

#endif /* _IO_H */

