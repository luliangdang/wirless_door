#include "compare.h"
#include "sdio_sdcard.h" 
#include "ff.h"
#include "lcd.h"
#include "text.h"
#include "string.h"			//字符串操作
#include "delay.h"
#include "sys.h"
#include "fingerprint.h"
#include "Instruction.h"
#include "my_math.h"
#include "piclib.h"

UINT readnum;
extern FIL fdsts;  
extern char Card_flag;			//内读卡器检测到卡标志  0 ：无，1：有
extern char P_Card_flag;		//外读卡器检测到卡标志  0 ：无，1：有
extern u8 finger_ask;				//指纹模块请求标志位
extern u8 u3finger_ask;			//指纹模块请求标志位
extern FILINFO fileinfop;		//显示图片
extern FRESULT res;
extern DIR dirp;
Union_info Client;					//单个人员信息存储联合体
LinkInfoTypeDef LinkInfo[MaxUserNum];	//用户信息链表

/*********************************************************************
*功    能：获取用户信息链表
*入口参数：
*出口参数：
*********************************************************************/
void GetLinkInfo(void)
{
	u16 i,k;
	char SdUserNumArray[5]={0};	//接收SD卡存储用户数量
	u16 SdUserNum;							//SD卡存储用户数量
	char SdUserInfo[18]={0};		//接收SD用户信息链表
	if(f_open(&fdsts,"Member.txt",FA_READ)==FR_OK)	//读SD卡用户总数
	{
		f_lseek(&fdsts,44);  
		f_read(&fdsts,SdUserNumArray,4,&readnum);
		f_close(&fdsts);
	}
	SdUserNum=my_atoi(SdUserNumArray);	//转换SD存储用户数
	for(i=0;i<MaxUserNum;i++)						//清空用户信息链表
	{
		LinkInfo[i].UserNum=0;
		LinkInfo[i].ICID=0;
		LinkInfo[i].StateAndLimits=0;
	}
	if(f_open(&fdsts,"Member.txt",FA_READ)==FR_OK)
	{
		for(i=0;i<SdUserNum;i++)		//读SD用户信息链表
		{
			f_lseek(&fdsts,i*425+50); 
			f_read(&fdsts,SdUserInfo,14,&readnum);	//读SD用户号、学号
			f_lseek(&fdsts,i*425+50+33);
			f_read(&fdsts,&SdUserInfo[14],3,&readnum);	//读SD权限、状态
			LinkInfo[i].UserNum=my_atoi(SdUserInfo);		//存储用户链表的用户号
			for(k=0;k<8;k++)	//存储用户链表IC卡号
			{
				if(SdUserInfo[k+5]>0x39)	//为大写字母
				{
					LinkInfo[i].ICID=(LinkInfo[i].ICID<<4)+SdUserInfo[k+5]-0x37;
				}
				else	//为数字
				{
					LinkInfo[i].ICID=(LinkInfo[i].ICID<<4)+SdUserInfo[k+5]-0x30;
				}
			}
			if(SdUserInfo[14]>0x39)	//存储用户链表的用户权限
			{
				LinkInfo[i].StateAndLimits=SdUserInfo[14]-0x37;
			}
			else
			{
				LinkInfo[i].StateAndLimits=SdUserInfo[14]-0x30;
			}
			if(SdUserInfo[16]==0x30)	//存储用户链表的用户状态
			{
				LinkInfo[i].StateAndLimits&=0x7f;	//在门内 最高位为0
			}
			else
			{
				LinkInfo[i].StateAndLimits|=1<<7;	//在门外 最高位为1
			}
		}
		f_close(&fdsts);	  							
	}
}



/*********************************************************************
*功    能：对比卡号或用户号
*入口参数：
*出口参数：
*********************************************************************/
extern unsigned char SN[4];
u16 compare(void)
{
	u16 i;
	u16 FingerUserNum=0;		//检测到的用户号
	u32 RFID_ICID=0;
	if((u3finger_ask==1)||(finger_ask==1))	//如果检测到指纹
	{
		FingerUserNum=receive_pack.pack.Q1*100+receive_pack.pack.Q2;			//检测到的用户号
		if(FingerUserNum==0)
		{
			return 0xffff;		//不在用户链表内则返回0xffff
		}
		for(i=0;i<MaxUserNum;i++)	//在用户链表中查询
		{
			if(LinkInfo[i].UserNum==FingerUserNum)	//对比用户号
			{
				if(f_open(&fdsts,"Member.txt",FA_READ)==FR_OK)	//读取SD卡中用户信息
				{
					f_lseek(&fdsts,i*425+50); 
					f_read(&fdsts,Client.info_arrary,37,&readnum);	
					f_close(&fdsts);	  							
				}
				return i;			//返回用户链表序号
			}
		}
	}
	else if(P_Card_flag||Card_flag)	//如果检测到IC卡
	{
		for(i=0;i<4;i++)	//转换IC卡号
		{
			RFID_ICID=(RFID_ICID<<8)+SN[i];
			SN[i]=0;
		}
		for(i=0;i<MaxUserNum;i++)	//在用户链表中查询
		{
			if(LinkInfo[i].ICID==RFID_ICID)		//对比IC卡号
			{
				if(f_open(&fdsts,"Member.txt",FA_READ)==FR_OK)	//读SD卡用户信息
				{
					f_lseek(&fdsts,i*425+50);  
					f_read(&fdsts,Client.info_arrary,37,&readnum);	
					f_close(&fdsts);
				}
				return i;		//返回用户链表序号
			}
		}
	}
	return 0xffff;		//不在用户链表内则返回0xffff
}


/*********************************************************************
*功    能：对比权限和进出状态,控制开门
*入口参数：
*出口参数：
*********************************************************************/
void Check_Status(u16 LinkNum)					
{
	u16 k=0,Menber_site,finger_save;
	u16 order_temp=0;						//名单序号
	char set_val[2]={"1"};					//状态位清零数组
	char reset_val[2]={"0"};				//状态位清零数组
	
	if((P_Card_flag==1)||(u3finger_ask==1))	//门内刷卡
	{
		if((LinkInfo[LinkNum].StateAndLimits>>7)==0)	//0成员从内出门
		{
			Door_state=1;					//开门信号
			Buzzer_state=1;					//蜂鸣器
			delay_ms(100);				
			Door_state=0;
			Buzzer_state=0;
			LCD_LED=1;						//点亮背光
			LCD_Fill(195,95,301,121,LGRAY);
			LCD_Fill(205,135,258,161,LGRAY);
			LCD_Fill(205,175,303,201,LGRAY);
			Show_Str(200,100,96,16,"谢谢光临！  ",16,0);
			Show_Str(150,140,48,16,"姓名：",16,1);							//显示成员信息
			Show_Str(150,180,48,16,"学号：",16,1);
			Show_Str(210,140,48,16,(u8*) Client.Student_Information[0].Name,16,0);
			Show_Str(210,180,88,16,(u8*) Client.Student_Information[0].StudentNum,16,0);
			show_photo();
			LinkInfo[LinkNum].StateAndLimits|=0x80;			
			if(f_open(&fdsts,"Member.txt",FA_WRITE)==FR_OK)
			{	 	
				f_lseek(&fdsts,LinkNum *425+35+50);                          	 	//移动文件指针
				f_write(&fdsts,set_val,1,&readnum);	
				f_close(&fdsts);	  							
			}
							
//			delay_ms(1500);
			offlcd();				//定时熄屏	
			Client.Student_Information[0].state[0]='1';
            Save_access();     		//进出门时上传个人信息
		}
		else
		{

			LCD_LED=1;				//点亮背光
			LCD_Fill(195,95,301,121,LGRAY);
			LCD_Fill(205,135,258,161,LGRAY);
			LCD_Fill(205,175,303,201,LGRAY);
			Show_Str(200,100,96,16,"您进门未刷卡",16,0);	
			Show_Str(210,140,48,16,(u8*) Client.Student_Information[0].Name,16,0);
			Show_Str(210,180,88,16,(u8*) Client.Student_Information[0].StudentNum,16,0);		
			show_photo();
//			delay_ms(1500);
			offlcd();				//定时熄屏			
		}
	}
	else if((Card_flag==1)||(finger_ask==1))	//门外刷卡
	{			
		if((LinkInfo[LinkNum].StateAndLimits>>7)==1)		//1成员从外进门
		{
			Door_state=1;			//开门信号
			Buzzer_state=1;
			delay_ms(100);																																																	
			Door_state=0;
			Buzzer_state=0;
			LCD_LED=1;				//点亮背光
			LCD_Fill(195,95,301,121,LGRAY);
			LCD_Fill(205,135,258,161,LGRAY);
			LCD_Fill(205,175,303,201,LGRAY);
			Show_Str(200,100,96,16,"欢迎光临！  ",16,0);
			Show_Str(150,140,48,16,"姓名：",16,1);							//显示成员信息
			Show_Str(150,180,48,16,"学号：",16,1);
			Show_Str(210,140,48,16,(u8*) Client.Student_Information[0].Name,16,0);
			Show_Str(210,180,88,16,(u8*) Client.Student_Information[0].StudentNum,16,0);
			show_photo();
			LinkInfo[LinkNum].StateAndLimits&=0x7f;			
			if(f_open(&fdsts,"Member.txt",FA_WRITE)==FR_OK)
			{	 	
				f_lseek(&fdsts,LinkNum *425+35+50);                          	 	//移动文件指针
				f_write(&fdsts,reset_val,1,&readnum);	
				f_close(&fdsts);	  							
			}
//			delay_ms(1500);
			offlcd();			//定时熄屏	
			Client.Student_Information[0].state[0]='0';
            Save_access();      //进出门时上传个人信息
		}	
		else
		{
			LCD_LED=1;				//点亮背光
			LCD_Fill(195,95,301,121,LGRAY);
			LCD_Fill(205,135,258,161,LGRAY);
			LCD_Fill(205,175,303,201,LGRAY);
			Show_Str(200,100,96,16,"您出门未刷卡",16,0);
			Show_Str(210,140,48,16,(u8*) Client.Student_Information[0].Name,16,0);
			Show_Str(210,180,88,16,(u8*) Client.Student_Information[0].StudentNum,16,0);
			show_photo();
//			delay_ms(1500);
			offlcd();				//定时熄屏		
		}	
	}		
}

/*********************************************************************
*功    能：显示照片
*入口参数：
*出口参数：
*********************************************************************/
void show_photo(void)
{
	char photo_addr[12]={"0:/"};
	char photo_name[9]={0};
	char jpg[5]={".jpg"};
	//0:/Desktop.jpg
	memcpy(photo_name,Client.Student_Information[0].order,4);		//拷贝出用户号
	memcpy(&photo_name[4],jpg,4);
	memcpy(&photo_addr[3],photo_name,8);	
	res = f_opendir(&dirp,(const TCHAR*)"0:/"); //打开一个目录
  if (res == FR_OK)
	{
		while(1)
		{
			res = f_readdir(&dirp, &fileinfop);                   //读取目录下的一个文件
			if (res != FR_OK || fileinfop.fname[0] == 0) 
			{
				piclib_init();
				ai_load_picfile("0:/someone.jpg",10,60,120,150,1);//显示图片
				break;  //错误了/到末尾了,退出
			}
			fileinfop.fname[8]=0;
			if((strcmp(photo_name,fileinfop.fname))==0)
			{
				piclib_init();
				ai_load_picfile(photo_addr,10,60,120,150,1);//显示图片	
				return;
			}
		}
	}
}

/*********************************************************************
*功    能：门和蜂鸣器端口初始化
*入口参数：
*出口参数：
*********************************************************************/
void Door_Buzzer_Init(void)			
{ 
 GPIO_InitTypeDef  GPIO_InitStructure; 	
 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOE, ENABLE);	

 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_3;	 //door	PE2,    
 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz	
 GPIO_Init(GPIOE, &GPIO_InitStructure);					 //根据设定参数初始化
 GPIO_ResetBits(GPIOE,GPIO_Pin_2);						 //复位
}

/*********************************************************************
*功    能：SD卡内信息与指纹模块内信息同步
*入口参数：串口号
*出口参数：
*********************************************************************/
void SD_Finger_Compare(USART_TypeDef* USARTx)		
{
	u16 i,j,k;
	u8 m;
	u8 eigen[193];
	u8 fing_temp[2];
	u16 FingerAllNum=0;
	u16 FingerNum=0;
	u8 user_ok=0;
	char SdUserNumArray[5]={0};	//接收SD卡存储用户数量
	u16 SdUserNum;				//SD卡存储用户数量
	if(f_open(&fdsts,"Member.txt",FA_READ)==FR_OK)	//读SD卡用户总数
	{
		f_lseek(&fdsts,44);
		f_read(&fdsts,SdUserNumArray,4,&readnum);
		f_close(&fdsts);
	}
	SdUserNum=my_atoi(SdUserNumArray);	//转换SD存储用户数
	getAllUser(USARTx);
	FingerAllNum=(receiveMore[1]<<8)+receiveMore[2];
	for(k=0;k<FingerAllNum;k++)
	{
		FingerNum=(receiveMore[3*k+3]<<8)+receiveMore[3*k+4];
		for(i=0;i<SdUserNum;i++)
		{
			if(FingerNum==LinkInfo[i].UserNum)
			{
				user_ok=1;
				break;
			}
		}
		if(user_ok == 0)
		{
			deleteOneUser(USARTx,FingerNum);					//删除指定用户
		}
		user_ok=0;
	}
	Show_Str(210,140,48,16,"用户号",16,0);	
	for(i=0;i<SdUserNum;i++)
	{
		if(f_open(&fdsts,"Member.txt",FA_READ)==FR_OK)
		{
			f_lseek(&fdsts,i*425+50);                         					//移动文件指针
			f_read(&fdsts,Client.info_arrary,sizeof(Client.info_arrary),&readnum);	//读取人员信息
			f_close(&fdsts);
		}

		LCD_ShowxNum(210,180,i+1,3,16,0);	//显示 数字
		for(j=0;j<193;j++)				//转换指纹特征值
		{
			if((Client.Student_Information[0].Fingerprint[j*2] > 0x29)&&(Client.Student_Information[0].Fingerprint[j*2] < 0x3A))
			{				fing_temp[0]=Client.Student_Information[0].Fingerprint[j*2]-48;			}
			else if((Client.Student_Information[0].Fingerprint[j*2] > 0x40)&&(Client.Student_Information[0].Fingerprint[j*2] < 0x5B))
			{				fing_temp[0]=Client.Student_Information[0].Fingerprint[j*2]-55;			}
			else {fing_temp[0]=0;}
			if((Client.Student_Information[0].Fingerprint[j*2+1] > 0x29)&&(Client.Student_Information[0].Fingerprint[j*2+1] < 0x3A))
			{				fing_temp[1]=Client.Student_Information[0].Fingerprint[j*2+1]-48;			}
			else if((Client.Student_Information[0].Fingerprint[j*2+1] > 0x40)&&(Client.Student_Information[0].Fingerprint[j*2+1] < 0x5B))
			{				fing_temp[1]=Client.Student_Information[0].Fingerprint[j*2+1]-55;			}
			else {fing_temp[1]=0;}
			eigen[j] = (fing_temp[0]<<4)+fing_temp[1];				//转换特征值
		}
		if(downloadAndMatchONE(USARTx,LinkInfo[i].UserNum,eigen)==ACK_FAIL)	//下传指纹特征值与 DSP 模块数据库指纹比对 1： 1		若失败则用户信息有错
		{			
			ResetFingerFlag();										//指纹模块标志位复位
			deleteOneUser(USARTx,LinkInfo[i].UserNum);
			ResetFingerFlag();										//指纹模块标志位复位
			m=downloadAddUser(USARTx,LinkInfo[i].UserNum,1,eigen);			//下传特征值并按指定用户号存入 DSP 模块数据库
			if(m==ACK_SUCCESS )										//下传特征值并按指定用户号存入 DSP 模块数据库		建立新用户
			{
				Show_Str(235,180,48,16,"号成功",16,0);
			}
			else
			{
				Show_Str(235,180,48,16,"号失败",16,0);
			}			
		}	
		else 
		{
			Show_Str(235,180,48,16,"号成功",16,0);
		}
		ResetFingerFlag();				//指纹模块标志位复位	
	}
}

/*********************************************************************
*功    能：定时熄屏
*入口参数：
*出口参数：
*********************************************************************/
void offlcd(void)
{
	TIM6->CR1&=~0x0001;		//失能TIMx	
	TIM6->CNT = 0X0000;		//定时器6重新计数
	TIM6->DIER|=0x0001;		//使能指定的TIM6中断,允许更新中断
	TIM6->CR1|=0x0001;		//使能TIMx
}
