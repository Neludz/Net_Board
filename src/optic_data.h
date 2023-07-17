#ifndef OPTIC_DATA_H_INCLUDED
#define OPTIC_DATA_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

//-------------------------------------------------------------------------
// user define
#define OPTIC_UART_BAUD                 38400
#define OPTIC_TIME_OUT_MS               50
#define OPTIC_WAIT_BEFORE_UART_MS       1
#define OPTIC_CHANNEL_COUNT_PHY         6   // from 0 to 8 ( use 74HCT4051)
#define OPTIC_CHANNELS_LIST             {5, 4, 3, 0, 1, 2}
#define OPTIC_LEGACY_CHANNEL_COUNT_PHY  2   // first from list
#define MB_OPTIC_START_REG              0
#define LEGACY_MULTIPLEXER_TIMER_MS     100
#define TASK_NOTIFY_INTERRUPT           0x01
#define TASK_NOTIFY_TIMER               0x02

//-------------------------------------------------------------------------
// user prototype
void optic_init(void);

//-------------------------------------------------------------------------
// system define
#define OPTIC_BUF_SIZE              8
#define OPTIC_ERROR_MAX             2
#define OPTIC_LEGACY_ERROR_MAX      4
#define LEGACY_CHANNEL_COUNT        12
//-------------------------------------------------------------------------
typedef enum
{
    OPT_STATE_IDLE,         // Ready to get a frame
    OPT_STATE_RCVE,			// Frame is being received
    OPT_STATE_WAIT,	        // Frame is wait to start
    OPT_STATE_PARS,			// Frame is being parsed (may take some time)
} OpticState_t;
//-------------------------------------------------------------------------
typedef enum
{
    OPT_NO_IDLE = 0,
    OPT_IDLE_DETECT = 1,
} OpticIdle_t;
//-------------------------------------------------------------------------
typedef enum
{
    OPT_RET_OK = 0,
    OPT_RET_TIMEOUT,
    OPT_RET_CRC,
    OPT_RET_DATA_ERROR,
    OPT_RET_SIZE_ERROR,
} OpticError_t;
//-------------------------------------------------------------------------
typedef enum
{
    OPT_MODE_LEGACY = 0,
    OPT_MODE_MODERN,
} OpticMode_t;
//-------------------------------------------------------------------------
typedef struct
{
    OpticState_t main_state;
    OpticIdle_t idle_state;
    OpticError_t frame_error;
    OpticMode_t mode;
    uint32_t count_data;
    uint8_t *error_count;
    uint8_t *legacy_error_count;
    uint8_t *error_lan_flag;
    const uint8_t *multiplexer_channels_order;
    uint16_t *mb_reg_p;
    uint16_t buf[OPTIC_BUF_SIZE];
    uint8_t channel_count;
} OpticStruct_t;
//-------------------------------------------------------------------------
#define OPTIC_INSTANCE_DEF(instance_name,                                       \
                          mult_channels_order,                                            \
                         optic_channels_count,\
                         mb_reg_pointer)                                             \
    static const uint8_t instance_name##_mult_channels_order[optic_channels_count] = mult_channels_order;  \
    static uint8_t instance_name##_lan_error_flag_array[optic_channels_count];                       \
    static uint8_t instance_name##_lan_error_count_array[optic_channels_count]; \
    static uint8_t instance_name##_lan_legacy_error_count_array[LEGACY_CHANNEL_COUNT]; \
    static OpticStruct_t (instance_name) = {                                                \
        .channel_count = optic_channels_count,\
        .main_state = OPT_STATE_IDLE,                                             \
        .idle_state = OPT_NO_IDLE,                                    \
        .frame_error = OPT_RET_OK, \
        .mb_reg_p = mb_reg_pointer,\
        .error_count = instance_name##_lan_error_count_array,\
        .legacy_error_count = instance_name##_lan_legacy_error_count_array,\
        .error_lan_flag = instance_name##_lan_error_flag_array,\
        .multiplexer_channels_order = instance_name##_mult_channels_order,\
    };

//-------------------------------------------------------------------------
// prototype
void optic_parse_modern(OpticStruct_t *opt_data, uint32_t opt_channel);
void optic_parse_legacy(OpticStruct_t *opt_data, uint32_t opt_channel);
void optic_parsing(OpticStruct_t *opt_data, uint32_t opt_channel);
void optic_legacy_periodic_lan_error_set(OpticStruct_t *opt_data);

#endif /* OPTIC_DATA_H_INCLUDED */
