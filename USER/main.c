#include "delay.h"
#include "sys.h"
#include "lcd.h"
#include "sdio_sdcard.h"
#include "ff.h"
#include "exfuns.h"
#include "rc522.h"
#include "text.h"
#include "compare.h"
#include "timer.h"
#include "fingerprint.h"
#include "rtc.h"
#include "ZigBee.h"
#include "Instruction.h"
#include "my_math.h"
#include "wdg.h"
#include "piclib.h"

const u8 zero[900]={0};
u8 data[1024];
extern char num[9];	//读到的卡号
extern char Card_flag;			//内读卡器检测到卡标志  0 ：无，1：有
extern char P_Card_flag;			//外读卡器检测到卡标志  0 ：无，1：有
extern Union_info Client;					//单个人员信息存储联合体

FIL fdsts;      	 //存储文件信息
FATFS files;		 //存储工作区信息
u32 len_datar=0;
u32 SD_capp;
u32 shuzhi=0;
u8 finger_ask=0;	//指纹模块请求标志位
u8 u3finger_ask=0;	//指纹模块请求标志位

extern FIL fdsts_recive;
extern UINT readnum;

void delete_all_Finger(void);//删除指纹模块所有用户
extern LinkInfoTypeDef LinkInfo[MaxUserNum];	//用户信息链表
int main(void)
{
	u8 t=0;
	u16 ret;
	delay_init();	    							 //延时函数初始化
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_CRC,ENABLE); //时钟选择
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //设置NVIC中断分组2:2位抢占优先级，2位响应优先级	
	LCD_Init();				//LCD初始化
	
	exfuns_init();				//为fatfs相关变量申请内存
 	f_mount(fs[0],"0:",1); 		//挂载SD卡
	piclib_init();
	ai_load_picfile("0:/Desktop.jpg",0,0,320,240,1);//显示图片
	Show_Str(150,60,144,16,"欢迎走进创新实验室",16,1);
	
	BACK_COLOR=LGRAY;  //背景色
	LCD_Fill(195,95,301,121,LGRAY);
	LCD_Fill(205,135,258,161,LGRAY);
	LCD_Fill(205,175,303,201,LGRAY);
	Show_Str(200,100,96,16,"正在初始化..",16,0);
//	Zigbee_Init();        	//Zigbee初始化
	uart1_init(115200);     //Zigbee初始化
	uart2_init(115200);			//指纹模块串口2初始化   门外
	uart3_init(115200);			//指纹	模块串口3初始化   门内
	
	InitRC522();			//初始化射频卡模块      门外
	P_InitRC522();			//初始化射频卡模块      门内
	Tim6_Init(); 			//TIM6初始化
	Tim7_Init(); 			//TIM7初始化
	Door_Buzzer_Init();	 	//门和蜂鸣器初始化
	RTC_Init();	  			//RTC初始化 
	ResetFingerFlag();			//指纹模块标志位复位
	setAndReadLevel(USART2,1,0);	//设置、读取比对等级
	if(receive_pack.pack.Q2!=5)
	{
		setAndReadLevel(USART2,0,5);	//设置/读取比对等级
	}
	
	setAndGetTimeOut(USART2,0,0);	//设置/读取指纹采集等待超时时间
	setAndReadLevel(USART3,1,0);	//设置/读取比对等级
	if(receive_pack.pack.Q2!=5)
	{
		setAndReadLevel(USART3,0,5);	//设置/读取比对等级
	}
	setAndGetTimeOut(USART3,0,0);	//设置/读取指纹采集等待超时时间
   	GetLinkInfo();
	SD_Finger_Compare(USART2);		//SD卡内信息与指纹模块内信息同步
	SD_Finger_Compare(USART3);		//SD卡内信息与指纹模块内信息同步
	photo_compare();
	LCD_Fill(195,95,301,121,LGRAY);
	LCD_Fill(205,135,258,161,LGRAY);
	LCD_Fill(205,175,303,201,LGRAY);
	
	
	Show_Str(200,100,96,16,"初始化完成！",16,0);
	delay_ms(1000);
	LCD_LED=0;					//关闭背光
	ResetFingerFlag();	//指纹模块标志位复位
	fastMatchN(USART2);	//开启门外指纹模块
	fastMatchN(USART3);	//开启门内指纹模块

//	IWDG_Init(7,0xfff);    //与分频数为256,重载值为2^11,溢出时间约为13s
  	while(1) 
	{	
		if(t!=calendar.sec)		//时间显示
		{
			LCD_ShowString(20,10,200,16,16,"    -  -  ");	   
			LCD_ShowString(120,10,200,16,16,"  :  :  ");
			t=calendar.sec;
			LCD_ShowNum(20,10,calendar.w_year,4,16);									  
			LCD_ShowNum(60,10,calendar.w_month,2,16);									  
			LCD_ShowNum(84,10,calendar.w_date,2,16);	 
			switch(calendar.week)
			{
				case 0:
					LCD_ShowString(200,10,200,16,16,"Sunday   ");
					break;
				case 1:
					LCD_ShowString(200,10,200,16,16,"Monday   ");
					break;
				case 2:
					LCD_ShowString(200,10,200,16,16,"Tuesday  ");
					break;
				case 3:
					LCD_ShowString(200,10,200,16,16,"Wednesday");
					break;
                
				case 4:
					LCD_ShowString(200,10,200,16,16,"Thursday ");
					break;
				case 5:
					LCD_ShowString(200,10,200,16,16,"Friday   ");
					break;
				case 6:
					LCD_ShowString(200,10,200,16,16,"Saturday ");
					break;  
			}
			LCD_ShowNum(120,10,calendar.hour,2,16);									  
			LCD_ShowNum(144,10,calendar.min,2,16);									  
			LCD_ShowNum(168,10,calendar.sec,2,16);
		}
	
		ReadID();			//读SPI1对应的卡	门内
		P_ReadID();			//读SPI3对应的卡	门外
		finger_ask=(receive_short_ok==1)&&(receive_pack.pack.CMD==CMD_MATCH_N);		//检测到的是门外指纹请求
		u3finger_ask=(u3receive_short_ok==1)&&(receive_pack.pack.CMD==CMD_MATCH_N);	//检测到的是门外指纹请求
		if(P_Card_flag||Card_flag||finger_ask||u3finger_ask)						//检测到开门请求
		{
			ret=compare();
			if(ret!=0xffff)			//卡号或用户号匹配成功
			{
				if(StorageADDRESS == THISADDRESS)
				{
					if((LinkInfo[ret].StateAndLimits&0x0f)==0x0f)			//次高权限E可进（可开除库房所有门）
					{
						Check_Status(ret);		//对比进出状态
					}
					else
					{					
						LCD_LED=1;			//点亮背光
						LCD_Fill(195,95,301,121,LGRAY);
						LCD_Fill(205,135,258,161,LGRAY);
						LCD_Fill(205,175,303,201,LGRAY);
						Show_Str(200,100,96,16,"您无权进入！",16,0);
						Show_Str(210,140,48,16,(u8*) Client.Student_Information[0].Name,16,0);
						Show_Str(210,180,88,16,(u8*) Client.Student_Information[0].StudentNum,16,0);
						show_photo();
						offlcd();			//定时熄屏
					}
				}
				
				else if(((LinkInfo[ret].StateAndLimits&0x0f)==THISADDRESS)||		//对应实验室编号可进
					((LinkInfo[ret].StateAndLimits&0x0f)==0x0f)||			//最高权限F可进（可开所有门）
					((LinkInfo[ret].StateAndLimits&0x0f)==0x0e))			//次高权限E可进（可开除库房所有门）
				{
					Check_Status(ret);		//对比进出状态
				}
				else
				{
					LCD_LED=1;			//点亮背光
					LCD_Fill(195,95,301,121,LGRAY);
					LCD_Fill(205,135,258,161,LGRAY);
					LCD_Fill(205,175,303,201,LGRAY);
					Show_Str(200,100,96,16,"您无权进入！",16,0);
					Show_Str(210,140,48,16,(u8*) Client.Student_Information[0].Name,16,0);
					Show_Str(210,180,88,16,(u8*) Client.Student_Information[0].StudentNum,16,0);
					show_photo();
					offlcd();			//定时熄屏
				}
			}	
			else
			{				
				
				LCD_LED=1;				//点亮背光  
				LCD_Fill(195,95,301,121,LGRAY);
				LCD_Fill(205,135,258,161,LGRAY);
				LCD_Fill(205,175,303,201,LGRAY);
				Show_Str(200,100,96,16,"您尚未注册！",16,0);
				piclib_init();
				ai_load_picfile("0:/someone.jpg",10,60,120,150,1);//显示图片				
				offlcd();				//定时熄屏
			}
			
			Card_flag=0;					//内刷卡标志清零
			P_Card_flag=0;				//内刷卡标志清零
			finger_ask=0;					//指纹模块请求标志位
			u3finger_ask=0;				//指纹模块请求标志位
			ResetFingerFlag();		//指纹模块标志位复位
			fastMatchN(USART2);		//重新开启1对N指纹检测
			fastMatchN(USART3);		//重新开启1对N指纹检测
		}
        if(receiveNByte_ok==1)     		//接收数据完成
        {
            receiveNByte_ok=0;       	//清空接收完成标志位
						delay_ms(50);
            switch_CMD();               //指令选择 
        }
	}
 }

///*********************************************************************
//*功    能：指纹模块删除所有用户
//*入口参数：
//*出口参数：
//*********************************************************************/
//void delete_all_Finger(void)
//{
//	u8 userCount = 0xff;		
//	while(userCount!=0)
//	{
//		deleteAllUser(USART2);
//		getUserCount(USART2);
//		userCount = (receive_pack.pack.Q1<<8)+receive_pack.pack.Q2;
//	}
//  userCount = 0xff;
//	while(userCount!=0)
//	{
//		deleteAllUser(USART3);
//		getUserCount(USART3);
//		userCount = (receive_pack.pack.Q1<<8)+receive_pack.pack.Q2;
//	}
//    delay_ms(1);
//}
// 
// 
