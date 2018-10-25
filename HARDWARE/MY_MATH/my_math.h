#ifndef _MY_MATH_H
#define _MY_MATH_H
#include "sys.h"

#define NULL 0

int my_atoi(char* pstr) ;
u8 CharToHex(u8 high,u8 low);/*两个字节转换成一个十六进制数*/
extern u8 char_temp[2];       /*一个十六进制数转换成两个字节*/
void HexToChar(u8 Hex);       /*结果存放在char_temp[2]中，高位在前*/


#endif


