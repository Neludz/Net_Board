#ifndef OPTIC_DATA_H_INCLUDED
#define OPTIC_DATA_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

//-------------------------------------------------------------------------
// user define
#define OPTIC_UART_BAUD                 38400
#define OPTIC_TIME_OUT_MODERN_MS        50
#define OPTIC_TIME_OUT_LEGACY_MS        70
#define OPTIC_WAIT_BEFORE_UART_MS       1
#define OPTIC_CHANNEL_COUNT_PHY         6   // from 0 to 8 ( use 74HCT4051)
#define OPTIC_CHANNELS_LIST             {2, 1, 0, 3, 4, 5}
#define OPTIC_LEGACY_CHANNEL_COUNT_PHY  2   // first from list
#define MB_OPTIC_START_REG              0
#define LEGACY_MULTIPLEXER_TIMER_MS     100
#define TASK_NOTIFY_INTERRUPT           0x01
#define TASK_NOTIFY_TIMER               0x02
//-------------------------------------------------------------------------
// user prototype
void optic_init(void);

#endif /* OPTIC_DATA_H_INCLUDED */
