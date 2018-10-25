#ifndef _ZIGBEE_H_
#define _ZIGBEE_H_

#include "stm32f10x.h"
#include "sys.h"

#define StorageADDRESS			0x03		//仓库地址
#define THISADDRESS					0x01		//本机地址
#define UPPERADDRESS		    0x00		//上位机地址
#define Z_HEAD 					0x5123A456	//包头
#define Z_TAIL 					0xA7895ABC	//包尾
#define ReSendTimes  			50			//最大重发次数
__packed typedef struct
{
	u32 HEAD;			//0-3		包头
	u8 ADDRESS;			//4			地址
	u8 CMD;				//5			命令
	u16 BLOCK;			//6-7		数据块编号
	u8 BYTE;			//8			本数据块有效字节数
	u32 TOTAL;			//9-12		数据包总字节数
	u8 DATA[128];		//13-140	128字节数据
	u32 CRC_DATA;		//141-144	CRC
	u32 TAIL;			//145-148	包尾
}Data_Pack;

__packed typedef struct
{
	u32 HEAD;			//0-3
	u8 ADDRESS;			//4
	u8 CMD;				//5
	u16 BLOCK;			//6-7
	u8 BYTE;			//8
	u32 TOTAL;			//9-12
	u32 CRC_DATA;		//13-16
	u32 TAIL;			//17-20
}Data_PackHead;

typedef union
{
	u8 buf[149];		//接收缓冲区
	Data_Pack pack;		//数据包
	Data_PackHead packHead;//应答
}Z_Serialize_Pack;

extern u8 receive_ok;				//接收数据包完成标志
extern u16 receive_len;				//8字节接收计数器
extern u8 receiveNByte_ok;			//接收完成标志

void uart1_init(u32 bound);		//初始化串口
u8 sendNByte(u8 *data,u8 cmd,u8 addr,u32 byte,u16 outTime);	//发送N字节
void cleanReceiveData(void);		//清除接收缓存和标志位
void sendHead(u8 addr,u8 cmd,u16 block,u8 byte,u32 total);  //发送应答
//参数：data：待发送数据 addr：地址 cmd：命令 block：数据块编号 byte：本块有效字节数     total：本次数据传输总字节数
void sendBlock(u8 *data,u8 addr,u8 cmd,u16 block,u8 byte,u32 total);

void Zigbee_Init(void);						//zigbee初始化
void MODULE_SOFTWARE_RESTARRT(void);	//模块重启
void DATA_TRANS_MODE(u8 mode);			//数据传输模式
u8 READ_CHANNEL(void);					//读取模块频道
void SET_CHANNEL(u8 Channel);			//设置模块频道
u8 READ_NODE(void);						//读取节点类型
void SET_ROUTER(void);					//设置模块为从模式
void SET_COORDINATOR(void);				//设置模块为主模式
void SET_BOUND(u8 bound);				//设置模块串口波特率
u16 READ_PIN_ID(void);					//读取模块PIN ID
u16 SET_PIN_ID(u8 high,u8 low);			//设定模块PIN ID为特定值XX XX
void SetBound(void);					//zigbee波特率设置

#endif



