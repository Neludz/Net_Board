#ifndef MODBUS_HARD_H_INCLUDED
#define MODBUS_HARD_H_INCLUDED

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"
#include "queue.h"
#include "timers.h"
#include "modbus.h"

#define	UART_FRIQ	36000000LL

#define MODBUS_TASK_PRIORITY               M_MODBUS_TASK_PRIORITY
#define MODBUS_TASK_STACK_SIZE             M_MODBUS_TASK_STACK_SIZE

#define	 BAUD_9600		    9600
#define	 BAUD_19200		    19200
#define	 BAUD_57600		    57600
#define	 BAUD_115200	    115200
#define  BAUD_NUMBER        4
#define	 RS_485_BAUD_LIST   {BAUD_9600, BAUD_19200, BAUD_57600, BAUD_115200}

typedef enum
{
    NO_PARITY_1_STOP	= 0x00,
    NO_PARITY_2_STOP	= 0x01,
    EVEN_PARITY_1_STOP	= 0x02,
    ODD_PARITY_1_STOP	= 0x03,

} parity_stop_bits_t;

void mh_write_eeprom (mb_slave_t *p_instance);
void mh_modbus_init(void);
void mh_usb_recieve(uint8_t *USB_buf, uint16_t len);
void mh_factory (void);
void mh_buf_init (void);

#endif /* MODBUS_HARD_H_INCLUDED */
