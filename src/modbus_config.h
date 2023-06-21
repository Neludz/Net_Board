#ifndef MODBUS_X_H_INCLUDED
#define MODBUS_X_H_INCLUDED

#include <stdint.h>
#include "main.h"
#include "modbus_hard.h"
//-----------------------------------------------------------------------
// Modbus registers X macros
//-----------------------------------------------------------------------

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
	X_BUF(0,	Reg_Start,		        0,		0,		0,      0,      READ_R)\
	X_BUF(1,	Reg_T_0_Channel,	    0,		0,		0, 		0,	 	READ_R)\
	X_BUF(2,	Reg_T_1_Channel,	    0,		0,		0, 		0,	 	READ_R)\
	X_BUF(3,	Reg_T_2_Channel,	    0,		0,		0, 		0,	 	READ_R)\
	X_BUF(4,	Reg_T_3_Channel,	    0,		0,		0, 		0,		READ_R)\
	X_BUF(5,	Reg_T_4_Channel,	    0,		0,		0, 		0,		READ_R)\
	X_BUF(6,	Reg_T_5_Channel,	    0,		0,		0, 		0,		READ_R)\
	X_BUF(7,	Reg_T_6_Channel,	    0,		0,		0, 		0,		READ_R)\
	X_BUF(8,	Reg_T_7_Channel,	    0,		0,		0, 		0,		READ_R)\
	X_BUF(9,	Reg_T_8_Channel,	    0,		0,		0, 		0,		READ_R)\
	X_BUF(10,	Reg_Cur_RMS_W1,		    0,		0,		0, 		0,		READ_R)\
	X_BUF(19,	Reg_End,				0,	    0,      0,      0,       READ_R)\


#endif /* MODBUS_X_H_INCLUDED */
