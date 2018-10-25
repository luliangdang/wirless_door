#include "Instruction.h"
#include "sdio_sdcard.h"   
#include "ff.h"
#include "string.h"			//字符串操作
#include "fingerprint.h"
#include "compare.h"
#include "ZigBee.h"
#include "rtc.h" 
//#include "stdlib.h"
#include "my_math.h"

extern LinkInfoTypeDef LinkInfo[MaxUserNum];	//用户信息链表
extern u16 USER_SUM;
extern Union_info Client;		//单个人员信息存储联合体
extern FIL fdsts;      		 	//存储文件信息
u16 SD_NUM_OVER;						//SD卡人员信息OVER结束位
extern FIL fdsts_recive; 
extern UINT readnum;
extern const u8 zero[900];
extern u8 data[1024];
u8 eigen[193];                      //存储指纹信息

UINT bws;        	 // File R/W count
UINT bws_p;
FIL fds_pho;
u16 USER_SUM=0;	
FRESULT res;
DIR dirp;
FILINFO fileinfop;

/*********************************************************************
*功    能：下传指纹特征值添加用户
*入口参数：
*出口参数：成功或失败
*时间：2015年9月18日 22:12:15
*********************************************************************/
u8 adduser(void)
{
    static Union_userinfo transferinfo;        //临时存储在SD卡内的下传信息
    u16 order_temp=0;                   //人员信息在SD卡文本中的位置
    u16 userNum=0;                      //用户号
    u8 i,m,n;
    u8 TAB[1]={9};						//TAB asc2码
		u8 set_over[5]={"OVER"};			//设置SD卡用户信息结束标志位OVER的数组
    char order_array[5]={0};			//用于计算用户号
    u8  Huiche[2];                      //回车换行
    Huiche[0]=13;
    Huiche [1]=10;
	
    if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)               //拷贝临时存储在SD卡的下传信息至transferinfo
    {	 	
			f_lseek(&fdsts_recive,5);                          				 //移动文件指针
			f_read(&fdsts_recive,order_array,4,&readnum);					 //读用户号
			f_lseek(&fdsts_recive,5);
			f_read(&fdsts_recive,transferinfo.userinfo_arrary,227,&readnum); //读用户信息
			f_close(&fdsts_recive);	  							
    }
    ClearnSDCache();        											 //清空SD卡缓存信息
		userNum=my_atoi(order_array);										 //计算用户号
//    userNum=(transferinfo.userinfo.order[0]-0x30)*1000;		
//    userNum+=(transferinfo.userinfo.order[1]-0x30)*100;
//    userNum+=(transferinfo.userinfo.order[2]-0x30)*10;
//    userNum+=transferinfo.userinfo.order[3]-0x30;		
    if(userNum>USER_SUM)												 //如果添加人员的用户号大于现存用户号的最大值
    {   		
		if(f_open(&fdsts,"Member.txt",FA_WRITE)==FR_OK)               	 //更改SD卡存储的用户总数
    {	
			f_lseek(&fdsts,44);                         				 //移动文件指针
			f_write(&fdsts,transferinfo.userinfo.order,4,&readnum);		 //修改最大用户号
			f_lseek(&fdsts,USER_SUM*619+50);                         	 //修改OVER结束位
			f_write(&fdsts,zero,4,&readnum);	
			f_lseek(&fdsts,userNum*619+50);                         
			f_write(&fdsts,set_over,4,&readnum);
			f_close(&fdsts);	  							
		}
		for(i=USER_SUM;i<userNum;i++)
		{
			if(f_open(&fdsts,"Member.txt",FA_WRITE)==FR_OK)               	//清空该用户SD卡内的信息
			{
				order_temp=i*619+50;
				f_lseek(&fdsts,order_temp);
				f_write(&fdsts,zero ,619,&readnum);			 	  	//清空用户信息  保留回车换行
				f_close(&fdsts);
			}
		}
        USER_SUM=userNum;												 //更新最大用户号 		
    } 
	for(i=0;i<193;i++)													//将指纹特征值转换成字符存进SD卡
	{
		HexToChar(transferinfo.userinfo.Fingerprint[i]);
		data[3*i]=char_temp[0];
		data[3*i+1]=char_temp[1];
		data[3*i+2]=32;
	}
	data[578]=TAB[0];										//SD卡的格式
	order_temp=(userNum-1)*619+50;    									//计算存入SD卡人员信息库的位置	    
	for(i=0;i<193;i++)
	{
			eigen[i]=transferinfo.userinfo.Fingerprint[i];
	}
	ResetFingerFlag();													//指纹模块标志位复位
	m=downloadAddUser(USART2,userNum,1,eigen);
	if(m==ACK_SUCCESS)													//下传特征值并按指定用户号存入 DSP 模块数据库
	{
	ResetFingerFlag();												//指纹模块标志位复位
	n=downloadAddUser(USART3,userNum,1,eigen);
	if(n==ACK_SUCCESS)
	{
		//写一个人的所有信息
		if(f_open(&fdsts,"Member.txt",FA_WRITE)==FR_OK)              		//检查是否有该文件并以写打开  
		{																	//如有该文件写入数据  如没有该文件进入下一操作		
			f_lseek(&fdsts,order_temp);                          			//移动文件指针
			f_write(&fdsts,transferinfo.userinfo.order,4,&readnum);			//存入SD卡人员信息
			f_write(&fdsts,TAB,1,&readnum);					//添加TAB键
			f_write(&fdsts,transferinfo.userinfo.CardNum,8,&readnum);			
			f_write(&fdsts,TAB,1,&readnum);
			f_write(&fdsts,transferinfo.userinfo.StudentNum,11,&readnum);
			f_write(&fdsts,TAB,1,&readnum);
			f_write(&fdsts,transferinfo.userinfo.Name,6,&readnum);
			f_write(&fdsts,TAB,1,&readnum);
			f_write(&fdsts,transferinfo.userinfo.Jurisdiction,1,&readnum);
			f_write(&fdsts,TAB,1,&readnum);
			f_write(&fdsts,transferinfo.userinfo.state,1,&readnum);
			f_write(&fdsts,TAB,1,&readnum);
			f_write(&fdsts,data,193*3,&readnum);
			f_write(&fdsts,TAB,1,&readnum);
			f_write(&fdsts,Huiche ,2,&readnum);
			f_close(&fdsts);
			sendHead(THISADDRESS,CMD_ADD_USER,0,n,0xffffffff);			//发送成功应答
		}	    
		return ACK_SUCCESS; 
		}
		else	//失败
		{
			sendHead(THISADDRESS,CMD_ADD_USER,0,n,0xffffffff);			//发送失败应答
			return n;
		}
    }
    else		//失败
    {   
			sendHead(THISADDRESS,CMD_ADD_USER,0,m,0xffffffff);				//发送失败应答
      return m;
    }		
}

/*********************************************************************
*功    能：删除指定用户
*入口参数：
*出口参数：成功或失败
*时间：2015年9月18日 22:12:15
*********************************************************************/
u8 deleteuse(void)
{
    u16 order_temp;														//指向SD卡存储要删除人员的位置
    u16 userNum;														//用户号
	u8 delete_order[5]={0};												//存储用户号
	u8 delete_name[7]={0};												//存储用户名
	
    if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)               //检查是否有该文件并以写打开  如有该文件写入数据  如没有该文件进入下一操作
    {	 	
        f_lseek(&fdsts_recive,5);                          				 //移动文件指针
        f_read(&fdsts_recive,delete_order,4,&readnum);					 //读取用户号
		f_read(&fdsts_recive,delete_name,6,&readnum);					 //读取用户名
        f_close(&fdsts_recive);	  							
    }
    userNum=my_atoi(delete_order);										 //计算用户号	
//    userNum=(transferinfo.userinfo.order[0]-0x30)*1000;				 //计算用户号
//    userNum+=(transferinfo.userinfo.order[1]-0x30)*100;
//    userNum+=(transferinfo.userinfo.order[2]-0x30)*10;
//    userNum+=transferinfo.userinfo.order[3]-0x30;	
    order_temp=(userNum-1)*619+50;    									 //计算存入SD卡人员信息库的位置    
																		 //619是每个人信息字节数，50是SD卡开头占的字节数
    if(f_open(&fdsts,"Member.txt",FA_READ)==FR_OK)               
	{	 	
        f_lseek(&fdsts,order_temp);                          			 //移动文件指针
        f_read(&fdsts,Client.info_arrary,sizeof(Client.info_arrary),&readnum);	//读取SD卡存储的人员信息
        f_close(&fdsts);	  							
    }		
    Client.Student_Information[0].Name[6]=0;  	
	if(strcmp(Client.Student_Information[0].Name,delete_name)==0)		//比较用户名是否相同
	{
		Client.Student_Information[0].order[4]=0;						//字符串结尾
		if(strcmp(Client.Student_Information[0].order,delete_order)!=0) //如果用户号不正确
        {
			sendHead(THISADDRESS,CMD_DELETE_USER,0,ACK_FAIL,0xffffffff);//发送应答失败
			return ACK_FAIL;
		}
	}
	else
	{
		sendHead(THISADDRESS,CMD_DELETE_USER,0,ACK_FAIL,0xffffffff);	//若用户名不对发送失败应答
		return ACK_FAIL;
	}
     if((deleteOneUser(USART2,userNum)==ACK_SUCCESS)&&
		(deleteOneUser(USART3,userNum)==ACK_SUCCESS))
    { 																  	//若内外指纹模块均删除成功
		ClearnSDCache();            								 	//清空SD卡缓存信息
		if(f_open(&fdsts,"Member.txt",FA_WRITE)==FR_OK)               	//清空该用户SD卡内的信息
		{	
			f_lseek(&fdsts,order_temp);                          
			f_write(&fdsts,zero ,617,&readnum);			 	  	//清空用户信息  保留回车换行
			f_close(&fdsts);	  							
		}
		sendHead(THISADDRESS,CMD_DELETE_USER,0,ACK_SUCCESS,0xffffffff); //发送添加成功应答
        return ACK_SUCCESS; 
    }
    else
    {  
		sendHead(THISADDRESS,CMD_DELETE_USER,0,ACK_FAIL,0xffffffff);    //若指纹模块删除不成功，发送失败应答
        return ACK_FAIL;
    }
}

/*********************************************************************
*功    能：获取下位机用户列表
*入口参数：
*出口参数：
*时间：2015年9月18日 22:12:15
*********************************************************************/
void  Uploaduserlist(void)
{
  u8 i,k;		
	u16 j=0;
	u8 cmd_user_start[5]={0};	//从第几个用户开始
	u8 cmd_user_add[5]={0};		//上传用户数
	u16 user_order_start=0;		//起始用户号
	u16 user_order_add=0;		//终止用户号
	u16 order_temp=0;			//SD卡原存的用户号
    u32 Menber_site=0;			//读取人员信息的字节位置
	u16 num_t=0;				//有效上传的用户人数
	char SdUserNumArray[5]={0};	//接收SD卡存储用户数量
	u16 SdUserNum;				//SD卡存储用户数量
	if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)               
	{	 	
        f_lseek(&fdsts_recive,5);                          				//移动文件指针
        f_read(&fdsts_recive,cmd_user_start,4,&readnum);   				//读取要上传的第几个用户和上传用户数
		f_read(&fdsts_recive,cmd_user_add,4,&readnum);     				//读取要上传的第几个用户和上传用户数
        f_close(&fdsts_recive);	  							
    }
	ClearnSDCache();        							   				//清空SD卡缓存信息
	user_order_start=my_atoi(cmd_user_start);			   				//计算第几个用户
	user_order_add=my_atoi(cmd_user_add);			       				//计算上传用户数
	
	
	if(f_open(&fdsts,"Member.txt",FA_READ)==FR_OK)	//读SD卡用户总数
	{	 	
		f_lseek(&fdsts,44);  
		f_read(&fdsts,SdUserNumArray,4,&readnum);	
		f_close(&fdsts);	  							
	}
	SdUserNum=my_atoi(SdUserNumArray);	//转换SD存储用户数
	if((user_order_start+user_order_add)<=SdUserNum)
	{
		for(j=0;j<user_order_add;j++)
		{
			 if(f_open(&fdsts,"Member.txt",FA_READ)==FR_OK)
			{	 	
				f_lseek(&fdsts,(user_order_start+j-1)*425+50);                  				 //移动文件指针
				f_read(&fdsts,Client.info_arrary,sizeof(Client.info_arrary),&readnum);	//读取用户信息，拷贝至Clients
				f_close(&fdsts);	  							
			}
			for(i=0;i<4;i++)
			{   data[i+j*31+4]=Client.Student_Information[0].order[i];  }   		//用户号
			for(i=0;i<8;i++)
			{   data[i+j*31+4+4]=Client.Student_Information[0].CardNum[i];  }		//卡号
			for(i=0;i<11;i++)
			{   data[i+j*31+4+12]=Client.Student_Information[0].StudentNum[i];  }	//学号
			for(i=0;i<6;i++)
			{   data[i+j*31+4+23]=Client.Student_Information[0].Name[i];  }    		//姓名
			data[j*31+4+29]=Client.Student_Information[0].Jurisdiction[0]; 			//权限
			data[j*31+4+30]=Client.Student_Information[0].state[0];                 //状态            
		}
		HexToChar(j>>8);													//将实际上传的人数住换成字符，用于发送
		data[0]=char_temp[0];
		data[1]=char_temp[1];
		HexToChar(j&0xff);
		data[2]=char_temp[0];
		data[3]=char_temp[1];	
		sendNByte(data,CMD_GET_USER_LIST,UPPERADDRESS,j*31+4,500);			//发送用户信息	
	}
	if((user_order_start+user_order_add)>SdUserNum)
	{
		for(j=0;j<SdUserNum-user_order_start;j++)
		{
			 if(f_open(&fdsts,"Member.txt",FA_READ)==FR_OK)
			{
				f_lseek(&fdsts,(user_order_start+j-1)*425+50);                  				 //移动文件指针
				f_read(&fdsts,Client.info_arrary,sizeof(Client.info_arrary),&readnum);	//读取用户信息，拷贝至Clients
				f_close(&fdsts);	  							
			}
			for(i=0;i<4;i++)
			{   data[i+j*31+4]=Client.Student_Information[0].order[i];  }   		//用户号
			for(i=0;i<8;i++)
			{   data[i+j*31+4+4]=Client.Student_Information[0].CardNum[i];  }		//卡号
			for(i=0;i<11;i++)
			{   data[i+j*31+4+12]=Client.Student_Information[0].StudentNum[i];  }	//学号
			for(i=0;i<6;i++)
			{   data[i+j*31+4+23]=Client.Student_Information[0].Name[i];  }    		//姓名
			data[j*31+4+29]=Client.Student_Information[0].Jurisdiction[0]; 			//权限
			data[j*31+4+30]=Client.Student_Information[0].state[0];                 //状态            
		}
		HexToChar(j>>8);													//将实际上传的人数住换成字符，用于发送
		data[0]=char_temp[0];
		data[1]=char_temp[1];
		HexToChar(j&0xff);
		data[2]=char_temp[0];
		data[3]=char_temp[1];	
		sendNByte(data,CMD_GET_USER_LIST,UPPERADDRESS,j*31+4,500);			//发送用户信息	
	}
}


/*********************************************************************
*功    能：获取下位机用户全部信息
*入口参数：
*出口参数：
*时间：2015年9月18日 22:12:15
*********************************************************************/
void  Uploadalluserlist(void)
{
    u8 i;		
	u16 j=0,k=0;
	u8 cmd_user_start[5]={0};	//从第几个用户开始
	u8 cmd_user_add[5]={0};		//上传用户数
	u16 user_order_start=0;		//起始用户号
	u16 user_order_add=0;		//终止用户号
	u16 order_temp=0;
    u32 Menber_site=0;		//读取人员信息的字节位置
	u16 num_t=0;
	
	if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)               
	{	 	
        f_lseek(&fdsts_recive,5);                          				//移动文件指针
        f_read(&fdsts_recive,cmd_user_start,4,&readnum);   				//读取要上传的第几个用户和上传用户数
		f_read(&fdsts_recive,cmd_user_add,4,&readnum);     				//读取要上传的第几个用户和上传用户数
        f_close(&fdsts_recive);	  							
    }
	ClearnSDCache();        							   				//清空SD卡缓存信息
	user_order_start=my_atoi(cmd_user_start);			   				//计算第几个用户
	user_order_add=my_atoi(cmd_user_add);			       				//计算上传用户数
    do
    {
        Menber_site=k*619+50;											 //计算读取SD卡数据位置
		k++;
        if(f_open(&fdsts,"Member.txt",FA_READ)==FR_OK)              
		{	 	
            f_lseek(&fdsts,Menber_site);                         		 //移动文件指针
            f_read(&fdsts,Client.info_arrary,sizeof(Client.info_arrary),&readnum);	//读取SD卡人员信息，拷贝至Clients
            f_close(&fdsts);	  							
        }	
		Client.Student_Information[0].order[4]=0;
		order_temp=my_atoi(Client.Student_Information[0].order);		 //SD卡原存的用户号
//		order_temp=(Client.Student_Information[0].order[0]-0x30)*1000;	 //计算用户号
//		order_temp+=(Client.Student_Information[0].order[1]-0x30)*100;
//		order_temp+=(Client.Student_Information[0].order[2]-0x30)*10;
//		order_temp+=(Client.Student_Information[0].order[3]-0x30);
		if(k>user_order_start+100)										 //修正人员信息最后全是0是的死循环
		{
			return;
		}
		if(order_temp==0)												 //用户号为空，该用户已将被删除
		{
			continue;
		}	
		num_t++;														 //有效上传的用户人数+1
		if(num_t<user_order_start)										 //如果有效上传的人数小于开始上传的人数
		{
			continue;
		}		
        for(i=0;i<4;i++)
        {   data[i+j*224+4]=Client.Student_Information[0].order[i];  }          //用户号
        for(i=0;i<8;i++)
        {   data[i+j*224+4+4]=Client.Student_Information[0].CardNum[i];  }  	//卡号
        for(i=0;i<11;i++)
        {   data[i+j*224+4+12]=Client.Student_Information[0].StudentNum[i];  }  //学号
        for(i=0;i<6;i++)
        {   data[i+j*224+4+23]=Client.Student_Information[0].Name[i];  }   	    //姓名
        data[j*224+4+29]=Client.Student_Information[0].Jurisdiction[0]; 		//权限
        data[j*224+4+30]=Client.Student_Information[0].state[0];                //状态   
		for(i=0;i<193;i++)														//指纹
		{
			data[j*224+4+31+i]=CharToHex(Client.Student_Information[0].Fingerprint[3*i],Client.Student_Information[0].Fingerprint[3*i+1]);
		}		
		j++;		
		if(j>4)																	//一次最多上传4人信息
		{
			return;																//防止程序在此死循环
		}
    }while(j<user_order_add);													//实际上传人数不等于需要上传人数时循环
	HexToChar(j>>8);															//将实际上传的人数住换成字符，用于发送
	data[0]=char_temp[0];
	data[1]=char_temp[1];
	HexToChar(j&0xff);
	data[2]=char_temp[0];
	data[3]=char_temp[1];
//发送任意字节数据
//参数：data：待发送的数据 cmd：命令：addr：地址    byte： 总字节数     outTime(ms)：超时时间
//返回值 0：成功 1：超时 2:超重发次数 3:通信错误
	sendNByte(data,CMD_GET_ALL_LIST,UPPERADDRESS,j*224+4,500);					//发送用户信息
}

/*发送在线联机应答*/
void Sendonline(void)
{
    ClearnSDCache();         /*清空SD卡缓存信息*/
    sendHead(THISADDRESS,CMD_ONLINE,0,0,0xffffffff);
}

/*********************************************************************
*功    能：设置时间
*入口参数：
*出口参数：
*时间：2015年9月18日 22:12:15
*********************************************************************/
void settime(void)
{
  u16 year;
  u8 mon,day,hour,min,sec;
	u8 n=0;
  Union_Time Gettime;
    if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
    {	 	
        f_lseek(&fdsts_recive,5);                          		//移动文件指针
        f_read(&fdsts_recive,Gettime.time_arrary,14,&readnum);	//读取设定时间
        f_close(&fdsts_recive);	  							
    }
    ClearnSDCache();            								//清空SD卡缓存信息
    year=(Gettime.TIME.year[0]-0x30)*1000;     					//计算时间
		year+=(Gettime.TIME.year[1]-0x30)*100;
		year+=(Gettime.TIME.year[2]-0x30)*10;
		year+=(Gettime.TIME.year[3]-0x30);
	
		mon=(Gettime.TIME.month[0]-0x30)*10;
		mon+=(Gettime.TIME.month[1]-0x30);
	
		day=(Gettime.TIME.date[0]-0x30)*10;
		day+=(Gettime.TIME.date[1]-0x30);
	
		hour=(Gettime.TIME.hour[0]-0x30)*10;
		hour+=(Gettime.TIME.hour[1]-0x30);
	
		min=(Gettime.TIME.minute[0]-0x30)*10;
		min+=(Gettime.TIME.minute[1]-0x30);
	
		sec=(Gettime.TIME.second[0]-0x30)*10;
		sec+=(Gettime.TIME.second[1]-0x30);
	
    n=RTC_Set(year,mon,day,hour,min,sec);						//设置时间	
		sendHead(THISADDRESS,CMD_SET_TIME,0,n,0xffffffff);			//发送设置时间返回值 0 成功；1失败
}

/*********************************************************************
*功    能：存储个人进出门信息
*入口参数：
*出口参数：
*********************************************************************/
void Save_access(void)
{
	u32 IO_num=0;				//存储的出入信息条数
	u32 IO_SD_SATE=0;			//指向本次存储的位置
	char IO_num_array[9]={0};	//存储的出入信息条数
	char time_array[16]={0};	//存储时间
	char time_temp[6]={0};		//时间缓冲区
	if(f_open(&fdsts,"SaveIO.txt",FA_READ)==FR_OK)               
	{	 	
		f_lseek(&fdsts,0);                         		 //移动文件指针
		f_read(&fdsts,IO_num_array,8,&readnum);			 //读取已存信息条数
		f_close(&fdsts);
	}
	IO_num=my_atoi(IO_num_array);						 //将字符串转换成整数
	IO_SD_SATE=IO_num*36+8;								 //指向本次存储的位置
	IO_num++;											 //存储条数+1
	sprintf(IO_num_array,"%08d",IO_num); 				 //将整数转换成字符串	
	sprintf(time_array, "%04d",calendar.w_year);			 //格式化输出 时间	
	sprintf(time_temp, "%02d",calendar.w_month);
	strcat(time_array,time_temp);						 //字符串拼接
	sprintf(time_temp, "%02d",calendar.w_date);
	strcat(time_array,time_temp);
	sprintf(time_temp, "%02d",calendar.hour);
	strcat(time_array,time_temp);
	sprintf(time_temp, "%02d",calendar.min);
	strcat(time_array,time_temp);
	sprintf(time_temp, "%02d",calendar.sec);
	strcat(time_array,time_temp);
//	if(Client.Student_Information[0].state[0]=='1')		 //改变刷卡用户存储的状态
//	{
//		Client.Student_Information[0].state[0]='0';		 //0已在门内，1已在门外
//	}
//	else
//	{
//		Client.Student_Information[0].state[0]='1';
//	}
	if(f_open(&fdsts,"SaveIO.txt",FA_WRITE)==FR_OK)
    {			
        f_lseek(&fdsts,0);                         		 //移动文件指针
		f_write(&fdsts,IO_num_array,8,&readnum);		 //更新已存出入信息条数
		f_lseek(&fdsts,IO_SD_SATE); 
        f_write(&fdsts,Client.Student_Information[0].order,4,&readnum);		 //存储用户号	
		f_write(&fdsts,Client.Student_Information[0].StudentNum,11,&readnum);//存储学号
        f_write(&fdsts,Client.Student_Information[0].Name,6,&readnum);		 //存储用户名
        f_write(&fdsts,Client.Student_Information[0].state,1,&readnum);	   	 //存储进出状态  
		f_write(&fdsts,time_array,14,&readnum);               				 //存储时间
        f_close(&fdsts);	  							
    }	
}

/*********************************************************************
*功    能：上传个人进出门信息
*入口参数：
*出口参数：
*********************************************************************/
void Upload_access(void)
{
	u8 ret=0;
	char IO_num_array[9]={0};		//存储进出信息条数
	u16 IO_num=0;								//进出信息条数	
	u16 i;
	u8 clean_IO_num[9]={"00000000"};		//用于进出条数归零
	char seed_num[9]={0};
	for(i=0;i<1024;i++)
	{
		data[i]=0;
	}
	if(f_open(&fdsts,"SaveIO.txt",FA_READ)==FR_OK)
	{	 	
		f_lseek(&fdsts,0);                      //移动文件指针
		f_read(&fdsts,data,8,&readnum);					//读取存储进出信息条数
		f_close(&fdsts);	  							
	}
	IO_num=my_atoi(data);								//已存进出信息条数
	if(IO_num==0)
	{
		strncpy(data,clean_IO_num,8);
	}
	if(IO_num>=10)										//如果进出信息条数大于10
	{
		sprintf(seed_num,"%08d",10);
		seed_num[8]=0;
		strncpy(&data[8],seed_num,8);
		if(f_open(&fdsts,"SaveIO.txt",FA_READ)==FR_OK)
		{	 	
			f_lseek(&fdsts,(IO_num-10)*36+8);         //移动文件指针
			f_read(&fdsts,&data[16],360,&readnum);		//读取10条进出信息
			f_close(&fdsts);
		}
		ret=sendNByte(data,CMD_GTE_I_O,UPPERADDRESS,376,500	);//发送进出信息
		if(ret==0)
		{
			IO_num-=10;										//信息条数-10
			sprintf(IO_num_array, "%08d",IO_num); 			//转换成字符串
			if(f_open(&fdsts,"SaveIO.txt",FA_WRITE)==FR_OK)
			{	 	
				f_lseek(&fdsts,0); 
				f_write(&fdsts,IO_num_array,8,&readnum);	//更新存储信息条数			
				f_lseek(&fdsts,IO_num*36+8);                        		
				f_truncate(&fdsts);							//截断文件
				f_close(&fdsts);			
			}
		}
		
	}
	else
	{
		sprintf(seed_num,"%08d",IO_num);
		seed_num[8]=0;
		strncpy(&data[8],seed_num,8);
		if(IO_num > 0)
		{
			if(f_open(&fdsts,"SaveIO.txt",FA_READ)==FR_OK)
			{	 	
				f_lseek(&fdsts,8);                          
				f_read(&fdsts,&data[16],IO_num*36,&readnum);	//读取剩余进出信息
				f_close(&fdsts);
			}
		
		}
		ret=sendNByte(data,CMD_GTE_I_O,UPPERADDRESS,IO_num*36+16,500	);			
		if(ret==0)
		{
			if(f_open(&fdsts,"SaveIO.txt",FA_WRITE)==FR_OK) 
			{	 	
				f_lseek(&fdsts,0); 
				f_write(&fdsts,clean_IO_num,8,&readnum);					
				f_truncate(&fdsts);							//截断文件
				f_close(&fdsts);			
			}	
		}
			
	}
}

/*********************************************************************
*功    能：获取SD卡内最大用户号
*入口参数：
*出口参数：
*********************************************************************/
void GetMaxUserOrder(void)
{

	char USER_SUM_ARRAY[5]={0};
	if(f_open(&fdsts,"Member.txt",FA_READ)==FR_OK)               	 //更改SD卡存储的用户总数
	{	
		f_lseek(&fdsts,44);                         				 //移动文件指针
		f_read(&fdsts,USER_SUM_ARRAY,4,&readnum);		 //修改最大用户号
		f_close(&fdsts);	  							
	}		
	USER_SUM=my_atoi(USER_SUM_ARRAY);	
 }

/*********************************************************************
*功    能：上传总人数
*入口参数：
*出口参数：
*********************************************************************/
void Uploadusernum(void)
{
	u8 k=0;
	u16 order_temp=0;
	u16 USER_SUM_temp=0;
	u32 Menber_site=0;		//读取人员信息的字节位置	
	char SD_info_OVER[5]={"OVER"};
	do
	{
		Menber_site=k*619+50;														//计算读取SD卡数据位置
		k++;
		if(f_open(&fdsts,"Member.txt",FA_READ)==FR_OK)
		{	 	
			f_lseek(&fdsts,Menber_site);                          					//移动文件指针
			f_read(&fdsts,Client.info_arrary,sizeof(Client.info_arrary),&readnum);	//读取人员信息
			f_close(&fdsts);	  							
		}
		Client.Student_Information[0].order[4]=0;
		order_temp=my_atoi(Client.Student_Information[0].order);					//将字符串转换成整数
		if(order_temp==0)															//用户号为零
		{																			//结束本次循环
			continue;
		}
		USER_SUM_temp++;															//用户号不为零则总人数+1
	}while(strcmp(Client.Student_Information[0].order,SD_info_OVER));				//直到遇到结束标志OVER
	sendHead(THISADDRESS,CMD_GET_USER_NUM,0,0xff,USER_SUM_temp);					//发送总人数应答
}

/*********************************************************************
*功    能：存储照片
*入口参数：
*出口参数：
*********************************************************************/
void Save_photo(void)
{
	u8 data_num_arrary[5]={0};
	u8 photo_name[9]={0};
	u32 data_num=0;
	u16 k=0;

	
	if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
	{	 	
		f_lseek(&fdsts_recive,1);           //移动文件指针
        f_read(&fdsts_recive,data_num_arrary,4,&readnum);
		f_read(&fdsts_recive,photo_name,4,&readnum);
		f_lseek(&fdsts_recive,16);           //移动文件指针
		f_read(&fdsts_recive,&photo_name[4],4,&readnum);
        f_close(&fdsts_recive);	  							
	}	
	if(f_open(&fds_pho,photo_name,FA_CREATE_ALWAYS)==FR_OK)
	{
		f_close(&fds_pho);
	}
	
	data_num=(data_num_arrary[3]<<24)+(data_num_arrary[2]<<16)+(data_num_arrary[1]<<8)+data_num_arrary[0]-15;
//	data_num=30060;
	while(data_num>=1024)
	{
		if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
		{	 	
			f_lseek(&fdsts_recive,k*1024+20);           //移动文件指针
			f_read(&fdsts_recive,data,1024,&readnum);
			f_close(&fdsts_recive);	  							
		}
		if(f_open(&fds_pho,photo_name,FA_WRITE)==FR_OK)
		{	 	
			f_lseek(&fds_pho,k*1024);           //移动文件指针
			f_write(&fds_pho,data,1024,&bws_p);
			f_close(&fds_pho);	  							
		}	
		data_num-=1024;
		k++;
	}
	if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
	{	 	
		f_lseek(&fdsts_recive,k*1024+20);           //移动文件指针
		f_read(&fdsts_recive,data,data_num,&readnum);
		f_close(&fdsts_recive);	  							
	}
	if(f_open(&fds_pho,photo_name,FA_WRITE)==FR_OK)
	{	 	
		f_lseek(&fds_pho,k*1024);           //移动文件指针
		f_write(&fds_pho,data,data_num,&bws_p);
		f_close(&fds_pho);	  							
	}
	sendHead(THISADDRESS,CMD_SAVE_PHOTO,0,0xff,0);					//发送接收照片完成应答
}

/*********************************************************************
*功    能：删除照片
*入口参数：
*出口参数：
*********************************************************************/
void deletephoto(void)
{
	u8 res=0;
	u8 photo_name[9]={0};
	if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
	{	 	
		f_lseek(&fdsts_recive,5);            //移动文件指针
		f_read(&fdsts_recive,photo_name,4,&readnum);
		f_lseek(&fdsts_recive,16);           //移动文件指针
		f_read(&fdsts_recive,&photo_name[4],4,&readnum);
        f_close(&fdsts_recive);	  							
	}	
	res=f_unlink(photo_name);					 //删除文件
	sendHead(THISADDRESS,CMD_DELETE_PHOTO,0,res,0);					//发送接收照片完成应答
}
/*********************************************************************
*功    能：清空SD卡缓存信息
*入口参数：
*出口参数：
*********************************************************************/
void ClearnSDCache(void)
{
    if(f_open(&fdsts_recive,"Receive.txt",FA_WRITE)==FR_OK)
    {
        f_lseek(&fdsts_recive,0);           //移动文件指针
        f_truncate(&fdsts_recive);			//截断文件
        f_close(&fdsts_recive);
    }
}


/*********************************************************************
*功    能：新建人员信息表
*入口参数：
*出口参数：
*********************************************************************/
void new_member(void)
{
	u8 data_num_arrary[5]={0};
	u32 data_num=0;
	u16 k=0;

	
	if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
	{	 	
		f_lseek(&fdsts_recive,1);           //移动文件指针
        f_read(&fdsts_recive,data_num_arrary,4,&readnum);
        f_close(&fdsts_recive);	  							
	}
	if(f_open(&fdsts,"Member.txt",FA_CREATE_ALWAYS)==FR_OK)
	{
		f_close(&fdsts);
	}
	
	data_num=(data_num_arrary[3]<<24)+(data_num_arrary[2]<<16)+(data_num_arrary[1]<<8)+data_num_arrary[0];
	while(data_num>=1024)
	{
		if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
		{	 	
			f_lseek(&fdsts_recive,k*1024+5);           //移动文件指针
			f_read(&fdsts_recive,data,1024,&readnum);
			f_close(&fdsts_recive);
		}
		if(f_open(&fdsts,"Member.txt",FA_WRITE)==FR_OK)
		{	 	
			f_lseek(&fdsts,k*1024);                          	 	//移动文件指针
			f_write(&fdsts,data,1024,&readnum);	
			f_close(&fdsts);	  							
		}
			
		data_num-=1024;
		k++;
	}
	if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
	{	 	
		f_lseek(&fdsts_recive,k*1024+5);           //移动文件指针
		f_read(&fdsts_recive,data,data_num,&readnum);
		f_close(&fdsts_recive);
	}
	if(f_open(&fdsts,"Member.txt",FA_WRITE)==FR_OK)
	{	 	
		f_lseek(&fdsts,k*1024);                          	 	//移动文件指针
		f_write(&fdsts,data,data_num,&readnum);	
		f_close(&fdsts);	  							
	}
//	GetMaxUserOrder();
	GetLinkInfo();
	SD_Finger_Compare(USART2);		//SD卡内信息与指纹模块内信息同步
	SD_Finger_Compare(USART3);		//SD卡内信息与指纹模块内信息同步
	ResetFingerFlag();			//指纹模块标志位复位
	fastMatchN(USART2);	//开启门外指纹模块
	fastMatchN(USART3);	//开启门内指纹模块	

	ClearnSDCache();
	sendHead(THISADDRESS,CMD_MEMBER,0,0X0B,0);					//发送接收照片完成应答
}

/*********************************************************************
*功    能：查询图片是否存在
*入口参数：
*出口参数：
*********************************************************************/
void check_photo(void)
{
	char name_buff[9]={0};
	char photo_size_buf[9]={0};
	u32 photo_size=0;
	
	if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
	{	 	
		f_lseek(&fdsts_recive,5);           //移动文件指针
        f_read(&fdsts_recive,name_buff,8,&readnum);
		f_read(&fdsts_recive,photo_size_buf,8,&readnum);
        f_close(&fdsts_recive);	  							
	}
	photo_size = my_atoi(photo_size_buf);
	
	res = f_opendir(&dirp,(const TCHAR*)"0:/"); //打开一个目录
    if (res == FR_OK) 
	{	
		while(1)
		{
	        res = f_readdir(&dirp, &fileinfop);                   //读取目录下的一个文件
	        if (res != FR_OK || fileinfop.fname[0] == 0) 
			{
				sendHead(THISADDRESS,CMD_CHECK_PHOTO,0,0,0);					//发送照片不存在应答
				break;  //错误了/到末尾了,退出
			}
	
			fileinfop.fname[8]=0;
			if((strcmp(name_buff,fileinfop.fname))==0)
			{
				if(fileinfop.fsize == photo_size)
				{
					sendHead(THISADDRESS,CMD_CHECK_PHOTO,0,1,0);					//发送照片存在应答
					break;
				}				
			}			
		}
	}
}




/*********************************************************************
*功    能：同步图片
*入口参数：
*出口参数：
*********************************************************************/

void photo_compare(void)
{
	
	char jpg[5]={".jpg"};
	u8 k;
	u16 PhotoNum;
	u16 order_temp;
	u32 Menber_site;
	char name_buff[9]={0};
	u8 check_ok=0;
	char SdUserNumArray[5]={0};	//接收SD卡存储用户数量
	u16 SdUserNum;				//SD卡存储用户数量
	if(f_open(&fdsts,"Member.txt",FA_READ)==FR_OK)	//读SD卡用户总数
	{	 	
		f_lseek(&fdsts,44);  
		f_read(&fdsts,SdUserNumArray,4,&readnum);	
		f_close(&fdsts);	  							
	}
	SdUserNum=my_atoi(SdUserNumArray);	//转换SD存储用户数
	res = f_opendir(&dirp,(const TCHAR*)"0:/"); //打开一个目录
    if (res == FR_OK)
	{	
		while(1)
		{
	        res = f_readdir(&dirp, &fileinfop);                   //读取目录下的一个文件
	        if (res != FR_OK || fileinfop.fname[0] == 0) 
			{
				break;  //错误了/到末尾了,退出
			}
	
			fileinfop.fname[8]=0;
			if((strcmp(jpg,&fileinfop.fname[4]))!=0)
			{
				continue;
			}
			memcpy(name_buff,fileinfop.fname,4);
			PhotoNum=my_atoi(name_buff);
			check_ok=0;	
			
			for(k=0;k<SdUserNum;k++)
			{
				LinkInfo[k].UserNum==PhotoNum;
				check_ok=1;		
				break;
			}
			if(check_ok==0)
			{
				memcpy(name_buff,fileinfop.fname,8);
				f_unlink(name_buff);					 //删除文件				
			}	
			
//			k=0;
//			do
//			{
//				Menber_site=k*425+50;
//				k++;		
//				if(f_open(&fdsts,"Member.txt",FA_READ)==FR_OK)
//				{	 	
//					f_lseek(&fdsts,Menber_site);                         					//移动文件指针
//					f_read(&fdsts,Client.info_arrary,sizeof(Client.info_arrary),&readnum);	//读取人员信息
//					f_close(&fdsts);	  							
//				}	
//				order_temp=(Client.Student_Information[0].order[0]-0x30)*1000;				//计算用户号
//				order_temp+=(Client.Student_Information[0].order[1]-0x30)*100;
//				order_temp+=(Client.Student_Information[0].order[2]-0x30)*10;
//				order_temp+=(Client.Student_Information[0].order[3]-0x30);			
//				Client.Student_Information[0].order[4]=0;
//				if(strcmp(Client.Student_Information[0].order,name_buff) == 0)
//				{
//					check_ok=1;		
//					break;
//				}			
//				
//			}while(order_temp!=SD_NUM_OVER);	//没有遇到结束标志就循环
			if(check_ok==0)
			{
				memcpy(name_buff,fileinfop.fname,8);
				f_unlink(name_buff);					 //删除文件				
			}			
			
		}
	}


}







/*********************************************************************
*功    能：指令选择
*入口参数：
*出口参数：
*********************************************************************/
void switch_CMD(void)
{
    u8 CMD=0;
    if(f_open(&fdsts_recive,"Receive.txt",FA_READ)==FR_OK)
    {	 	
        f_lseek(&fdsts_recive,0);                         //移动文件指针
        f_read(&fdsts_recive,&CMD,1,&readnum);						//读取指令
        f_close(&fdsts_recive);	  							
    }  
//	CMD=CMD_DELETE_PHOTO;
    switch(CMD)														//指令选择
    {
        case CMD_ADD_USER: 			{   adduser();  		 			break;	} //添加用户
        case CMD_DELETE_USER: 	{   deleteuse(); 		 			break; 	}	//删除指定用户
        case CMD_ONLINE: 				{   Sendonline(); 		 		break; 	}	//发送在线联机应答
        case CMD_GET_USER_LIST: {   Uploaduserlist(); 	 	break; 	}	//获取下位机用户列表
        case CMD_GET_ALL_LIST:  {   Uploadalluserlist();	break;	}	//获取下位机用户全部信息
        case CMD_GET_USER_NUM:	{	 	Uploadusernum();	 		break;	}	//获取用户总数
        case CMD_SET_TIME :			{   settime();			 			break; 	}	//设置时间
        case CMD_GTE_I_O : 			{   Upload_access();	 		break; 	}	//上传个人进出门信息
				case CMD_SAVE_PHOTO:		{	 	Save_photo();		 			break;	}	//存储照片
				case CMD_DELETE_PHOTO:	{	 	deletephoto();		 		break;	}	//删除照片
				case CMD_MEMBER:				{	 	new_member();		 			break;	}	//新建人员信息
				case CMD_CHECK_PHOTO:		{	 	check_photo();		 		break;	}	//查寻图片是否存在

	
        default:{break;}
    }
}
















