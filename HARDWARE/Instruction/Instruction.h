#ifndef _INSTRUCTION_H
#define _INSTRUCTION_H

#include "sys.h"

//上位机传至下位机
#define CMD_ADD_USER          0X01      //添加人员
#define CMD_DELETE_USER       0X02      //删除人员
#define CMD_ONLINE            0X03      //检查联机
#define CMD_GET_USER_LIST     0X04      //获取下位机用户列表
#define CMD_GET_ALL_LIST      0X05      //获取下位机全部人员信息
#define CMD_GET_USER_NUM			0X06			//获取下位机总人数
#define CMD_SET_TIME				  0X07			//设置下位机时间
#define CMD_GTE_I_O					  0X08			//获取人员进出信息
#define CMD_SAVE_PHOTO				0X09			//存储照片
#define CMD_DELETE_PHOTO			0X0A			//删除照片
#define CMD_MEMBER			 		  0X0B			//下传人员信息表
#define CMD_CHECK_PHOTO				0X0C			//查看图片是否存在


typedef struct
{
	char order[4];					//顺序号
	char CardNum[8];				//ID卡号
	char StudentNum[11];		//学号
	char Name[6];						//姓名
	char Jurisdiction[1];		//权限
	char state[1];					//人员进出状态		1门外刷卡，已进入 ； 0门内刷卡已外出		
	char Fingerprint[193];	//指纹
    char Totalnum[4];
}TransferTypeDef;					//227byte


typedef union 
{ 
	TransferTypeDef userinfo;
	char userinfo_arrary[227];
}Union_userinfo;

typedef struct
{
    u8 year[4];
    u8 month[2];
    u8 date[2];
    u8 hour[2];
    u8 minute[2];
    u8 second[2];
}TimeTypeDef;
    
typedef union
{
    TimeTypeDef TIME;
    u8 time_arrary[14];
}Union_Time;

u8 adduser(void);      		 		//下传指纹特征值添加用户
u8 deleteuse(void);    		 		//删除指定用户
void Sendonline(void);       	//发送在线联机应答
void Uploaduserlist(void);   	//获取下位机用户列表  不含指纹
void Uploadalluserlist(void);	//获取下位机用户全部信息
void settime(void);    		 		//设置时间
void Save_access(void);		 		//存储个人进出门信息
void Upload_access(void);    	//进出门时上传个人信息
void ClearnSDCache(void);	 		//清空SD卡缓存信息
void switch_CMD(void);       	//指令选择
void GetMaxUserOrder(void);  	//获取SD卡内用户总数
void Uploadusernum(void);	 		//上传总人数
void Save_photo(void);		 		//存储照片
void new_member(void);		 		//新建人员信息表
void photo_compare(void);	 		//同步图片
void check_photo(void);		 		//查询图片是否存在

#endif

