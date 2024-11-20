// modbus_reg.c
#include "modbus_config.h"
#include "modbus_reg.h"
#include "modbus.h"

//-----------------------------------------------------------------------
//	X macro
//-----------------------------------------------------------------------
#if ((MB_CALLBACK_REG == 1) || (MB_LIMIT_REG == 1))
const mbreg_parameters_t mbreg_param[MB_NUM_BUF] =
{
#define X_BUF(a,b,c,d,e,f,g)	[b]={d,e,f,g},
    MB_BUF_TABLE
#undef X_BUF
};
#endif

#if (MB_USER_ARG1_REG == 1)
const void *mbreg_arg1[MB_NUM_BUF] =
{
#define X_BUF(a,b,c,d,e,f,g)	[b]=c,
    MB_BUF_TABLE
#undef X_BUF
};
#endif
//-----------------------------------------------------------------------
// function
//-----------------------------------------------------------------------
//
#if ((MB_CALLBACK_REG == 1) || (MB_LIMIT_REG == 1))
mb_error_t mbreg_instance_idle_check (mb_slave_t *p_instance)
{
    if ((p_instance->cb_state == MB_CB_FREE) && (p_instance->mb_state == MB_STATE_IDLE))
    {
        return MB_OK;
    }
    return MB_ERROR;
}

//-----------------------------------------------------------------------
mb_error_t mbreg_limit_check(uint16_t number, uint16_t value)
{
    switch(mbreg_param[number].options & LIM_BIT_MASK)
    {
    case 0:
        break;	//not use limit for this register
    case LIM_MASK:
        if (value & (~(mbreg_param[number].max_level_mask)))
        {
            return MB_ERROR;
        }
        break;
    case LIM_SIGN:
        if ((int16_t)value > (int16_t)mbreg_param[number].max_level_mask ||
                (int16_t)value < (int16_t)mbreg_param[number].min_level)
        {
            return MB_ERROR;
        }
        break;
    case LIM_UNSIGN:
        if ((uint16_t)value > (uint16_t)mbreg_param[number].max_level_mask ||
                (uint16_t)value < (uint16_t)mbreg_param[number].min_level)
        {
            return MB_ERROR;
        }
        break;
    default:
        break;
    }
    return MB_OK;
}

//-----------------------------------------------------------------------
mb_error_t mbreg_option_check(uint16_t number, uint16_t option_mask)
{
    if ((mbreg_param[number].options & option_mask) == option_mask)
    {
        return MB_OK;
    }
    return MB_ERROR;
}

//-----------------------------------------------------------------------
mbreg_parameters_t mbreg_get_param(uint16_t number)
{
    return mbreg_param[number];
}
#endif

//-----------------------------------------------------------------------
#if (MB_USER_ARG1_REG == 1)
const void* mbreg_get_user_arg1(uint16_t number)
{
    return mbreg_arg1[number];
}
#endif

// func for modbus.c
mb_error_t mbreg_write_option_check (uint16_t number)
{
    return mbreg_option_check(number, WRITE_R);
}

// func for modbus.c
mb_error_t mbreg_cb_option_check (uint16_t number)
{
    return mbreg_option_check(number, CB_WR);
}
