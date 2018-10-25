#ifndef _FINGERPRINT_H
#define _FINGERPRINT_H

#include <stdio.h>
#include "sys.h"


#define TRUE  1
#define FALSE 0

//基本应答信息定义
#define ACK_SUCCESS      		0x00		//操作成功
#define ACK_FAIL          		0x01		//操作失败
#define ACK_FULL          		0x04		//指纹数据库已满
#define ACK_NO_USER		  	0x05		//无此用户
#define ACK_USER_OPD			0x06		//用户已存在
#define ACK_FIN_OPD 			0x07 		//指纹已存在
#define ACK_TIMEOUT       		0x08		//采集超时
#define ACK_GO_OUT		  	0x0F		//错误命令

//用户信息定义
#define ACK_ALL_USER       		0x00
#define ACK_GUEST_USER 	  		0x01
#define ACK_NORMAL_USER 		0x02
#define ACK_MASTER_USER    		0x03

#define USER_MAX_CNT	   		1001		//设置容量 MAX = 1000

//命令定义
#define CMD_HEAD		  	0xF5		//标志头
#define CMD_TAIL		  	0xF5		//标志尾
#define CMD_ADD_1  		  	0x01		//添加指纹第1次命令
#define CMD_ADD_2 		  	0x02		//添加指纹第2次命令
#define CMD_ADD_3	  	  	0x03		//添加指纹第3次命令
#define CMD_DEL_ONE  	  		0x04		//删除指定用户
#define CMD_DEL_ALL  	  		0x05		//删除所有用户
#define CMD_USER_CNT    		0x09		//取用户总数
#define CMD_GETLIM			0x0A		//取用户权限
#define CMD_MATCH_ONE			0x0B		//对比1:1
#define CMD_MATCH_N		  	0x0C		//对比1:N
#define CMD_GET_EIGEN			0x23		//采集图像并提取特征值上传
#define CMD_GET_IMAGE			0x24		//采集图像并上传
#define CMD_GET_VER			0x26		//取 DSP 模块版本号
#define CMD_COM_LEV			0x28		//设置/读取比对等级
#define CMD_GET_USERNUM 		0x2B		//取已登录所有用户用户号及权限
#define CMD_LP_MODE		  	0x2C		//使模块进入休眠状态
#define CMD_SET_READ_ADDMODE		0x2D		//设置/读取指纹添加模式
#define CMD_TIMEOUT		 	0x2E		//设置/读取指纹采集等待超时时间
#define CMD_UP_EIGEN    		0x31		//上传 DSP 模块数据库内指定用户特征值
#define CMD_DOWN_ADD    		0x41		//下传特征值并按指定用户号存入 DSP 模块数据库
#define CMD_DOWN_MATCH_ONE    		0x42		//下传指纹特征值与 DSP 模块数据库指纹比对 1:1
#define CMD_DOWN_MATCH_N      		0x43		//下传指纹特征值与 DSP 模块数据库指纹比对 1:N
#define CMD_DOWN_MATCH_NOW    		0x44		//下传特征值与采集指纹比对

#define CMD_FINGER_DETECTED 		0x14


__packed typedef struct
{
	unsigned char HEAD;				//1
	unsigned char CMD;				//1
	unsigned char Q1;			 		//1
	unsigned char Q2;					//1
	unsigned char Q3;					//1
	unsigned char NONE;				//1
	unsigned char CHK;				//1
	unsigned char TAIL;				//1
} Recieve_Pack;

typedef union
{
	unsigned char buf[8];
	Recieve_Pack pack;
} Serialize_Pack;

typedef struct
{
	char HEAD[4];			
	char CMD[4];			
	char Q1[4];
	char Q2[4];					
	char Q3[4];					
	char NONE[4];				
	char CHK[4];				
	char TAIL[4];				
} Show_Text;

extern Serialize_Pack receive_pack;
extern u8 receive_short_ok;
extern u8 receive_long_ok;
extern u8 u3receive_short_ok;
extern u8 u3receive_long_ok;
extern Show_Text show_text;
extern u8 receiveMore[200];
extern u8 i;					//for循环用变量

void TxByte(USART_TypeDef* USARTx,u8 temp);
u8 setLpMode(USART_TypeDef* USARTx);					//使模块进入休眠状态
u8 setAndReadMode(USART_TypeDef* USARTx,u8 com,u8 mode);		//设置/读取指纹添加模式
u8 addUser(USART_TypeDef* USARTx,u16 userNum,u8 lim);			//添加指纹
u8 deleteOneUser(USART_TypeDef* USARTx,u16 userNum);			//删除指定用户
u8 deleteAllUser(USART_TypeDef* USARTx);		  		//删除所有用户
u8 getUserCount(USART_TypeDef* USARTx);					//取用户总数
u8 matchOne(USART_TypeDef* USARTx,u8 userNum);				//比对 1:1
u8 matchN(USART_TypeDef* USARTx);					//比对指纹1:N
void fastMatchN(USART_TypeDef* USARTx);					//快速发送比对指纹1:N指令
u8 getLim(USART_TypeDef* USARTx,u16 userNum);				//取用户权限
u8 getVer(USART_TypeDef* USARTx);					//取 DSP 模块版本号
u8 setAndReadLevel(USART_TypeDef* USARTx,u8 cmd,u8 level);		//设置/读取比对等级-比对等级0-9 取值越大比对越严格，默认值为5
u8 getEigen(USART_TypeDef* USARTx);					//采集图像并提取特征值上传
u8 downloadAndMatchONE(USART_TypeDef* USARTx,u16 userNum,u8 *eigen);	//下传指纹特征值与 DSP 模块数据库指纹比对 1： 1
u8 downloadAndMatchN(USART_TypeDef* USARTx,u8 *eigen);			//下传指纹特征值与 DSP 模块数据库指纹比对 1:N
u8 uploadEigen(USART_TypeDef* USARTx,u16 userNum);			//上传 DSP 模块数据库内指定用户特征值
u8 downloadAddUser(USART_TypeDef* USARTx,u16 userNum,u8 lim,u8 *eigen);//下传特征值并按指定用户号存入 DSP 模块数据库
u8 getAllUser(USART_TypeDef* USARTx);					//取已登录所有用户用户号及权限
u8 setAndGetTimeOut(USART_TypeDef* USARTx,u8 cmd,u8 timeOut);		//设置/读取指纹采集等待超时时间
void ResetFingerFlag(void);		//指纹模块标志位复位
void uart2_init(u32 bound);		//串口2初始化
void uart3_init(u32 bound);		//串口3初始化
#endif /*_FINGERPRINT_H*/

