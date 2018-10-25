#include "my_math.h"
#include <ctype.h>
/*********************************************************************
*功    能：将字符串转换成整型数
*入口参数：指向字符串的字符串指针
*出口参数：整型数值
*********************************************************************/
int my_atoi(char* pstr)  
{  
    int Ret_Integer = 0;  
    int Integer_sign = 1;  
      
    /* 
    * 判断指针是否为空 
    */  
    if(pstr == NULL)  
    {  
         
        return 0;  
    }  
      
    /* 
    * 跳过前面的空格字符 
    */  
    while(isspace(*pstr) != 0)  
    {
        pstr++;
    } 
      
    /* 
    * 判断正负号 
    * 如果是正号，指针指向下一个字符 
    * 如果是符号，把符号标记为Integer_sign置-1，然后再把指针指向下一个字符 
    */  
    if(*pstr == '-')
    {  
        Integer_sign = -1;
    }  
    if(*pstr == '-' || *pstr == '+')  
    {  
        pstr++;
    }
      
    /* 
    * 把数字字符串逐个转换成整数，并把最后转换好的整数赋给Ret_Integer 
    */ 
    while(*pstr >= '0' && *pstr <= '9')  
    {
        Ret_Integer = Ret_Integer * 10 + *pstr - '0';
        pstr++;
    }
    Ret_Integer = Integer_sign * Ret_Integer;  
    
    return Ret_Integer;
}

/*********************************************************************
*功    能：两个字节转换成一个十六进制数
*入口参数：
*出口参数：
*********************************************************************/
u8 CharToHex(u8 high,u8 low)
{
    u8 char_temp[2]={0}; 
    u8 Hex=0;
    if((high>0x29)&&(high<0x3A))
    {				char_temp[0]=high-48;			}
    else if((high>0x40)&&(high<0x5B))
    {				char_temp[0]=high-55;			}
    else {char_temp[0]=0;}
    if((low>0x29)&&(low<0x3A))
    {				char_temp[1]=low-48;			}
    else if((low>0x40)&&(low<0x5B))
    {				char_temp[1]=low-55;			}
    else {char_temp[1]=0;}
    Hex=(char_temp[0]<<4)+char_temp[1];		//转换特征值
    return Hex;
}

/*********************************************************************
*功    能：一个十六进制数转换成两个字节
*入口参数：一个十六进制数
*出口参数：结果存放在char_temp[2]中，高位在前
*********************************************************************/
u8 char_temp[2]={0}; 
void HexToChar(u8 Hex)
{
    char_temp[0]=0;
    char_temp[1]=0;
    if(((Hex&0x0f)>=0)&&((Hex&0x0f)<10))
    {        char_temp[1]=(Hex&0x0f)+48;    }
    else if(((Hex&0x0f)>9)&&((Hex&0x0f)<16))
    {        char_temp[1]=(Hex&0x0f)+55;    }
    if((((Hex>>4)&0x0f)>=0)&&(((Hex>>4)&0x0f)<10))
    {        char_temp[0]=((Hex>>4)&0x0f)+48;    }
    else if((((Hex>>4)&0x0f)>9)&&(((Hex>>4)&0x0f)<16))
    {        char_temp[0]=((Hex>>4)&0x0f)+55;    }
}

