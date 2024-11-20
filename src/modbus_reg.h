#ifndef MODBUS_REG_H_INCLUDED
#define MODBUS_REG_H_INCLUDED

#include <stdint.h>
#include "modbus_config.h"
#include "modbus.h"

//-----------------------------------------------------------------------
// Configurations
//-----------------------------------------------------------------------

typedef struct
{
    uint16_t		default_value;
    uint16_t		min_level;
    uint16_t		max_level_mask;
    uint32_t		options;
} mbreg_parameters_t;

#if (MB_REG_END_TO_END == 1)
enum
{
#define X_BUF(a,b,c,d,e,f) b,
    MB_BUF_TABLE
#undef  X_BUF
    MB_NUM_BUF  //Reg count
};
#else
enum
{
#define X_BUF(a,b,c,d,e,f,g) b=a,
    MB_BUF_TABLE
#undef  X_BUF
    MB_NUM_BUF=(REG_END_REGISTER+1)
};
#endif

//-----------------------------------------------------------------------
//  prototype
//-----------------------------------------------------------------------
mb_error_t mbreg_instance_idle_check (mb_slave_t *p_instance);
mb_error_t mbreg_limit_check (uint16_t number_reg, uint16_t value);
mb_error_t mbreg_option_check (uint16_t number, uint16_t option_mask);
mbreg_parameters_t mbreg_get_param (uint16_t number);
const void* mbreg_get_user_arg1 (uint16_t number);
mb_error_t mbreg_write_option_check (uint16_t number);
mb_error_t mbreg_cb_option_check (uint16_t number);
#endif /* MODBUS_REG_H_INCLUDED*/
