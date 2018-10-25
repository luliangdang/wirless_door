#ifndef __COMPARE_H
#define __COMPARE_H	 
#include "sys.h"


#define Door_state PEout(2)		// PE2			0关		1开
#define Buzzer_state PEout(3)	// PB1		0关		1开	
#define MaxUserNum 	500			//最大用户数

typedef struct		//内存中用户信息链表
{
	u16 UserNum;
	u32 ICID;
	u8  StateAndLimits;	//bit7 状态 bit3--bit0 权限
}LinkInfoTypeDef;


typedef struct
{
	char order[5];				//0--4  	顺序号
	char CardNum[9];			//5--13 	ID卡号
	char StudentNum[12];		//14--25    学号
	char Name[7];				//26--32    姓名
	char Jurisdiction[2];		//33--34    权限
	char state[2];				//35--36	人员进出状态		1门外刷卡，已进入 ； 0门内刷卡已外出		
	char Fingerprint[388];		//37--425   指纹
}Student_InformationTypeDef;	//425byte


typedef union 
{ 
	Student_InformationTypeDef Student_Information[1];
	char info_arrary[425];
}Union_info;

void GetLinkInfo(void);							//获取用户信息链表
u16 compare(void);								//对比卡号或用户号
void Check_Status(u16 LinkNum);					//对比权限和进出状态,控制开门
void Door_Buzzer_Init(void);					//门和蜂鸣器端口初始化
void SD_Finger_Compare(USART_TypeDef* USARTx);	//SD卡内信息与指纹模块内信息同步
void offlcd(void);								//定时熄屏	
void show_photo(void);							//显示照片
#endif







