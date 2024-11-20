#ifndef MODBUS_X_H_INCLUDED
#define MODBUS_X_H_INCLUDED

#include <stdint.h>
#include "main.h"
//-----------------------------------------------------------------------
// user define
//-----------------------------------------------------------------------
#define MB_MODE_LEGACY		(0x00)
#define MB_MODE_MODERN		(0x01)
//-----------------------------------------------------------------------
// define
//-----------------------------------------------------------------------
#define MB_LIMIT_REG	    1//check limit
#define MB_CALLBACK_REG	    1//use write callback
//#define MB_USER_ARG1_REG	1//use user argument (for example: run user callback after write function)
//#define MB_USER_ARG2_REG	1//not implement now
// If not define "MB_REG_END_TO_END" then number register is determined in "a" field from X-macros
//#define MB_REG_END_TO_END

#define REG_END_REGISTER                Reg_End
//-----------------------------------------------------------------------
// Modbus registers X macros
//-----------------------------------------------------------------------
//  MAIN_BUF_Start_Table_Mask
#define USER_FUNC		(0x20)
#define USER_ARG		(0x10)
#define READ_R		    (0)
#define WRITE_R		    (0x01)	// 0 bit                        <--|
#define CB_WR		    (0x02)	// 1 bit                        <--|
#define LIM_SIGN		(0x04)	// 2 bit for limit              <--|
#define LIM_UNSIGN	    (0x08)  // 3 bit for limit	            <--|
#define LIM_MASK	    (0x0C)	// 2 and 3 bit for limit        <--|____________
//                                                                              |
#define LIM_BIT_MASK	        LIM_MASK
//	 Number		Name for enum	       Arg1  Default   Min	   Max     __________Options________
//										      Value   Level   Level   |                         |
//														     or Mask  |                         |
#define MB_BUF_TABLE\
	X_BUF(0,	Reg_0,  		        0,		0,		0,      0,      READ_R)\
    X_BUF(50,	reg_rs485_baud_rate,	0,		1,		0,		0x03,	WRITE_R | CB_WR | LIM_MASK)\
	X_BUF(51,	reg_rs485_ans_delay,	0,	    5,		0,      100,	WRITE_R | CB_WR | LIM_UNSIGN)\
	X_BUF(52,	reg_rs485_modbus_addr,  0,		127,    1,		0xFA,	WRITE_R | CB_WR | LIM_UNSIGN)\
	X_BUF(53,	Reg_Parity_Stop_Bits,	0,	    0,	    0,		0x03,	WRITE_R | CB_WR | LIM_UNSIGN)\
    X_BUF(54,	Reg_54_Optic_Mode,      0,		MB_MODE_MODERN,\
                                                        0,      1,      WRITE_R | CB_WR | LIM_MASK)\
	X_BUF(55,	Reg_End,				0,	    0,      0,      0,      READ_R)\


#endif /* MODBUS_X_H_INCLUDED */
