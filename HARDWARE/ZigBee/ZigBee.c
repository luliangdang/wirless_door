#include "stm32f10x.h"
#include "ZigBee.h"
#include "delay.h"
#include "lcd.h"
#include "sdio_sdcard.h"   
#include "ff.h"
#include "my_math.h"
extern FIL fdsts;      	 //存储文件信息
extern UINT readnum;
FIL fdsts_recive; 
u8 receive_ok = 0;				//接收数据包完成标志
u8 receiveNByte_ok = 0;			//接收完成标志
u16 Z_receive_len = 0;			//8字节接收计数器
Z_Serialize_Pack serialize_pack;
u8 receiveSwitch = 0;			//接收开关
u16 numR;
u32 receiveCounter = 0;			//接收到的总字节数

u8 rec_init[8];
u8 Zigbe_INIT=0;				//zigbee初始化前
//STM32的CRC
u32 CRC32(u8 *pBuf,u16 head,u16 tail)
{
        CRC_ResetDR();        	//复位CRC        
        for(; head <= tail; head++)
        {
                CRC->DR = (u32)pBuf[head];
        }
        return (CRC->DR);
}

//初始化IO 串口1 
//bound:波特率
void uart1_init(u32 bound)
{
    //GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
 	USART_DeInit(USART1);  //复位串口1
	 //USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA9
   
    //USART1_RX	  PA.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PA10

   //Usart1 NVIC 配置

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=0 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
   //USART 初始化设置

	USART_InitStructure.USART_BaudRate = bound;//一般设置为9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

    USART_Init(USART1, &USART_InitStructure); //初始化串口
    USART_ClearFlag(USART1, USART_FLAG_TC);
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启中断
    USART_Cmd(USART1, ENABLE);                    //使能串口 

}

//串口1发送一字节数据
//参数：待发送数据
//返回值 无
void sendByte(u8 data)
{      
		USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);//关闭中断
    USART1->DR = data;      
		while((USART1->SR&0X40)==0);//循环发送,直到发送完毕  
		USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启中断
}

//清除接收器
//参数：无
//返回值 无
void cleanReceiveData()
{
	
	u8 i;
	Z_receive_len = 0;
	receive_ok = 0;
	for(i = 0;i < 149;i++)
	{
		serialize_pack.buf[i] = 0;
	}
}

//发送应答
//参数：addr：地址 		cmd：命令		 block：数据块编号 		byte：本块有效字节数 	total：本次数据传输总字节数
//返回值 无	
void sendHead(u8 addr,u8 cmd,u16 block,u8 byte,u32 total)
{
	u32 crcTemp = 0;
	CRC_ResetDR();        					//复位CRC
	
	sendByte((u8)((Z_HEAD&0xff000000)>>24));
	sendByte((u8)((Z_HEAD&0x00ff0000)>>16));
	sendByte((u8)((Z_HEAD&0x0000ff00)>>8));
	sendByte((u8)((Z_HEAD&0x000000ff)>>0));	//发送数据头
	
	sendByte(addr);							//发送地址
	
	sendByte(cmd);							//发送命令
	
	sendByte((u8)((block&0xff00)>>8));
	sendByte((u8)((block&0x00ff)>>0));		//发送当前数据块编号

	sendByte(byte);							//发送当前块有效字节数
	
	sendByte((u8)((total&0xff000000)>>24));
	sendByte((u8)((total&0x00ff0000)>>16));
	sendByte((u8)((total&0x0000ff00)>>8));
	sendByte((u8)((total&0x000000ff)>>0));		//发送总字节数

	crcTemp = CRC_CalcCRC((u32)addr);					//计算CRC
	crcTemp = CRC_CalcCRC((u32)cmd);					//计算CRC
	crcTemp = CRC_CalcCRC((u32)((block&0x00ff)>>0));	//计算CRC
	crcTemp = CRC_CalcCRC((u32)((block&0xff00)>>8));	//计算CRC
	crcTemp = CRC_CalcCRC((u32)byte);					//计算CRC
	crcTemp = CRC_CalcCRC((u32)(total&0x000000ff)>>0);	//计算CRC
	crcTemp = CRC_CalcCRC((u32)(total&0x0000ff00)>>8);	//计算CRC
	crcTemp = CRC_CalcCRC((u32)(total&0x00ff0000)>>16);	//计算CRC
	crcTemp = CRC_CalcCRC((u32)(total&0xff000000)>>24);	//计算CRC

	sendByte((u8)((crcTemp&0xff000000)>>24));
	sendByte((u8)((crcTemp&0x00ff0000)>>16));
	sendByte((u8)((crcTemp&0x0000ff00)>>8));
	sendByte((u8)((crcTemp&0x000000ff)>>0));		//发送CRC
	
	sendByte((u8)((Z_TAIL&0xff000000)>>24));
	sendByte((u8)((Z_TAIL&0x00ff0000)>>16));
	sendByte((u8)((Z_TAIL&0x0000ff00)>>8));
	sendByte((u8)((Z_TAIL&0x000000ff)>>0));			//发送数据尾
}

//发送一块数据块
//参数：data：待发送数据 addr：地址 cmd：命令 block：数据块编号 byte：本块有效字节数     total：本次数据传输总字节数
//返回值 无
void sendBlock(u8 *data,u8 addr,u8 cmd,u16 block,u8 byte,u32 total)
{
	u8 i = 0;
	u32 crcTemp = 0;
	CRC_ResetDR();        					//复位CRC
	
	sendByte((u8)((Z_HEAD&0xff000000)>>24));
	sendByte((u8)((Z_HEAD&0x00ff0000)>>16));
	sendByte((u8)((Z_HEAD&0x0000ff00)>>8));
	sendByte((u8)((Z_HEAD&0x000000ff)>>0));	//发送数据头
	
	sendByte(addr);							//发送地址
	
	sendByte(cmd);							//发送命令
	
	sendByte((u8)((block&0xff00)>>8));
	sendByte((u8)((block&0x00ff)>>0));		//发送当前数据块编号

	sendByte(byte);							//发送当前块有效字节数
	
	sendByte((u8)((total&0xff000000)>>24));
	sendByte((u8)((total&0x00ff0000)>>16));
	sendByte((u8)((total&0x0000ff00)>>8));
	sendByte((u8)((total&0x000000ff)>>0));	//发送总字节数

	crcTemp = CRC_CalcCRC((u32)addr);					//计算CRC
	crcTemp = CRC_CalcCRC((u32)cmd);					//计算CRC
	crcTemp = CRC_CalcCRC((u32)((block&0x00ff)>>0));	//计算CRC
	crcTemp = CRC_CalcCRC((u32)((block&0xff00)>>8));	//计算CRC
	crcTemp = CRC_CalcCRC((u32)byte);					//计算CRC
	crcTemp = CRC_CalcCRC((u32)(total&0x000000ff)>>0);	//计算CRC
	crcTemp = CRC_CalcCRC((u32)(total&0x0000ff00)>>8);	//计算CRC
	crcTemp = CRC_CalcCRC((u32)(total&0x00ff0000)>>16);	//计算CRC
	crcTemp = CRC_CalcCRC((u32)(total&0xff000000)>>24);	//计算CRC
	for(i = 0;i < 128;i++)
	{
		sendByte(data[i]);						//发送数据
		crcTemp = CRC_CalcCRC((u32)data[i]);	//计算CRC
	}
	
	sendByte((u8)((crcTemp&0xff000000)>>24));
	sendByte((u8)((crcTemp&0x00ff0000)>>16));
	sendByte((u8)((crcTemp&0x0000ff00)>>8));
	sendByte((u8)((crcTemp&0x000000ff)>>0));	//发送CRC
	
	sendByte((u8)((Z_TAIL&0xff000000)>>24));
	sendByte((u8)((Z_TAIL&0x00ff0000)>>16));
	sendByte((u8)((Z_TAIL&0x0000ff00)>>8));
	sendByte((u8)((Z_TAIL&0x000000ff)>>0));		//发送数据尾
}

//接收数据块
//参数：无
//返回值 无
void zigBeeReceiveData()
{
	static u32 oldBlock = 0;
	if(serialize_pack.pack.ADDRESS == THISADDRESS)	//通过地址判断是否接收数据
	{
		if((serialize_pack.pack.BLOCK == oldBlock + 1)||(serialize_pack.pack.BLOCK == 0))
		{
			oldBlock = serialize_pack.pack.BLOCK;
			receiveCounter += serialize_pack.pack.BYTE; //将接收到的字节数加到字节计数器中
			//命令serialize_pack.pack.CMD;
			//总字节数serialize_pack.pack.TOTAL;
			//serialize_pack.pack.DATA					//将接收的数据存入内存卡
			
			if(serialize_pack.pack.BLOCK==0)
			{
				receiveCounter = serialize_pack.pack.BYTE;
				if(f_open(&fdsts_recive,"Receive.txt",FA_WRITE|FA_CREATE_ALWAYS)==FR_OK)
				{			
					f_lseek(&fdsts_recive,0);                         				 //移动文件指针
					f_write(&fdsts_recive,&serialize_pack.pack.CMD,1,&readnum);		 //命令
					f_write(&fdsts_recive,&serialize_pack.pack.BYTE+1,4,&readnum);	 //数据包总字节数
					f_close(&fdsts_recive);	 
				}
				else
				{
					return;
				}
			}            
			if(f_open(&fdsts_recive,"Receive.txt",FA_WRITE)==FR_OK) 
			{	
				f_lseek(&fdsts_recive,serialize_pack.pack.BLOCK*128+5);                         	 //移动文件指针
				f_write(&fdsts_recive,serialize_pack.pack.DATA,serialize_pack.pack.BYTE,&readnum);	 
				f_close(&fdsts_recive);	  							
			} 
			else
			{
				return;
			}
			if(receiveCounter == serialize_pack.pack.TOTAL)		//数据包接收完成
			{
				receiveCounter=0;
				//cleanReceiveData();
				receiveNByte_ok = 1;				//数据包接收完成标志位
			}
			sendHead(serialize_pack.pack.ADDRESS,
					serialize_pack.pack.CMD,
					serialize_pack.pack.BLOCK,
					serialize_pack.pack.BYTE,
					serialize_pack.pack.TOTAL);		//发送应答
			cleanReceiveData();						//清除接收缓存和标志位
		}
	}   
	receive_ok = 0;								//读取完成之后将标志位复位，以便于下次接收数据
}

//交换数组数据
//参数: num待转换数组 head:开始位置 tail:结束位置
void exchangeOrder(u8 *num,u8 head,u8 tail)
{
	u8 temp = 0,i = 0,time = ((tail+1)-head)/2;
	for(;time > 0;tail--)
	{
		temp = num[head+i];
		num[head+i] = num[tail];
		num[tail] = temp;
		i++;
		time--;
	}
}
//发送数据
//参数：data：待发送的数据 cmd：命令：addr：地址    byte： 总字节数     outTime(ms)：超时时间
//返回值 0：成功 1：超时 2:超重发次数 3:通信错误
u8 sendNByte(u8 *data,u8 cmd,u8 addr,u32 byte,u16 outTime)
{
	u8 reSendTimes;					//重发次数
	u8 ret = 0;						//返回值
	u16 outTimeX = 0,				//重装超时时间用
		block = 0;					//数据块计数器
	u16 quotient = byte/128;		//需要发送的数据块数
	u8	remainder = byte%128;		//最后一块中有效的字节数
	if(remainder != 0){quotient++;}
	receiveSwitch = 1;				//发送数据时不接收数据
	for(;quotient > 0;quotient--)
	{
		for(reSendTimes = ReSendTimes;reSendTimes > 0;reSendTimes--)
		{
			cleanReceiveData();											//初始化接收数据
			if((quotient != 1)||(remainder == 0))
			{
				sendBlock(data+block*128,addr,cmd,block,128,byte);		//发送包头	
			}else
			{
				sendBlock(data+block*128,addr,cmd,block,remainder,byte);//发送包头	
			}
			outTimeX = outTime;
			while((Z_receive_len < 20) && (outTimeX != 0))				//等待应答数据
			{
				outTimeX--;
				delay_ms(1);
			}
			exchangeOrder(serialize_pack.buf,6,7);
			exchangeOrder(serialize_pack.buf,9,12);
			exchangeOrder(serialize_pack.buf,13,16);
			exchangeOrder(serialize_pack.buf,17,20);	//采用高位在前发送，所以需要将高低位交换
			if(outTimeX == 0){continue;}				//如果超时，重新发送
			if((serialize_pack.packHead.HEAD != Z_HEAD)
				||(serialize_pack.packHead.ADDRESS != addr)
				||(serialize_pack.packHead.CMD != cmd)
				||(serialize_pack.packHead.BLOCK != block)
				||(serialize_pack.packHead.TOTAL != byte))	//判断应答是否正确
			{continue;}		//错误则重发
			else
			{break;}		//正确就发送下一块数据
		}
		block++;
		if(outTimeX == 0){ret = 1;break;}						//返回超时
		if(reSendTimes == 0){ret = 2;break;}				
		if(serialize_pack.pack.ADDRESS != addr){ret = 3;break;}	//地址应该匹配
	}
	receiveSwitch = 0;		//发送完数据打开接收
	return ret;
}

//串口1中断服务函数，处理接收到的数据
u8 rec_num=0;
void USART1_IRQHandler(void)                	//串口1中断服务程序
{
	Zigbe_INIT=1;
	if(Zigbe_INIT==1)
	{
		if((USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) && (receive_ok != 1))  //接收中断
		{
			serialize_pack.buf[Z_receive_len++] = USART_ReceiveData(USART1);//读取接收到的数据
			if(Z_receive_len == 1)	//查找包头
			{
				if(serialize_pack.buf[0] != (u8)((Z_HEAD&0xff000000)>>24))
				{cleanReceiveData();}
			}
			if(Z_receive_len == 4)	//校验包头
			{
				exchangeOrder(serialize_pack.buf,0,3);
				if(serialize_pack.pack.HEAD != Z_HEAD)
				{cleanReceiveData();}	//如果包头不对，重新接收
			}
			if(Z_receive_len == 149)
			{
				exchangeOrder(serialize_pack.buf,6,7);
				exchangeOrder(serialize_pack.buf,9,12);
				exchangeOrder(serialize_pack.buf,141,144);
				exchangeOrder(serialize_pack.buf,145,148);	//采用高位在前发送，所以需要将高低位交换
				CRC_ResetDR();        						//复位CRC
				if((CRC32(serialize_pack.buf,4,140) != serialize_pack.pack.CRC_DATA)
					|| (serialize_pack.pack.TAIL != Z_TAIL))	//进行CRC校验以及校验包尾
				{
					cleanReceiveData();	//如果CRC校验错误以及包尾错误，重新接收
				}else
				{
					Z_receive_len = 0;
					receive_ok = 1;		//接收完成，置位标志位
				}
			}
			if((receive_ok == 1) && (receiveSwitch == 0))
			{
				zigBeeReceiveData();	//接收校验数据并产生应答
			}
		} 
	}
	else
	{		
		if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)   //接收中断
		{
			rec_init[rec_num]= USART_ReceiveData(USART1);//读取接收到的数据
			rec_num++;
		}
	}
//	USART_ClearFlag(USART1, USART_FLAG_TC);
	USART_ClearFlag(USART1, USART_FLAG_RXNE);
} 



/*********************************************************************
*功    能：设定模块PIN ID为特定值XX XX
*入口参数：PIN ID	0001-FF00
*出口参数：PIN ID
*********************************************************************/
u16 SET_PIN_ID(u8 high,u8 low)
{
	u8 sum;
	sum=(0XFC+0X02+0X91+0X01+high+low)&0XFF;
	sendByte(0XFC);
	sendByte(0X02);
	sendByte(0X91);
	sendByte(0X01);
	sendByte(high);
	sendByte(low);
	sendByte(sum);
	delay_ms(100);
	return (rec_init [0]<<8)+rec_init [1];	
}


/*********************************************************************
*功    能：读取模块PIN ID
*入口参数：
*出口参数：PIN ID
*********************************************************************/
u16 READ_PIN_ID(void)
{
	u8 sum;
	sum=(0XFC+0X00+0X91+0X03+0XA3+0XB3)&0XFF;
	sendByte(0XFC);
	sendByte(0X00);
	sendByte(0X91);
	sendByte(0x01);
	sendByte(0X03);
	sendByte(0XB3);
	sendByte(sum);
	delay_ms(100);
	return (rec_init[0]<<8)+rec_init[1];	
}


/*********************************************************************
*功    能：设置模块串口波特率
*入口参数：
*出口参数：PIN IDbound
*********************************************************************/
void SET_BOUND(u8 bound)
{
	u8 sum;
	sum=(0XFC+0X01+0X91+0X06+bound+0XF6)&0XFF;
	sendByte(0XFC);
	sendByte(0X01);
	sendByte(0X91);
	sendByte(0X06);
	sendByte(bound);
	sendByte(0XF6);
	sendByte(sum);
	delay_ms(100);
		
}

/*********************************************************************
*功    能：设置模块为主模式
*入口参数：
*出口参数：
*********************************************************************/
void SET_COORDINATOR(void)
{
	u8 sum;
	sum=(0XFC+0X00+0X91+0X09+0XA9+0XC9)&0XFF;
	sendByte(0XFC);
	sendByte(0X00);
	sendByte(0X91);
	sendByte(0X09);
	sendByte(0XA9);
	sendByte(0XC9);
	sendByte(sum);
	delay_ms(100);
		
}


/*********************************************************************
*功    能：设置模块为从模式
*入口参数：
*出口参数：
*********************************************************************/
void SET_ROUTER(void)
{
	u8 sum;
	sum=(0XFC+0X00+0X91+0X0A+0XBA+0XDA)&0XFF;
	sendByte(0XFC);
	sendByte(0X00);
	sendByte(0X91);
	sendByte(0X0A);
	sendByte(0XBA);
	sendByte(0XDA);
	sendByte(sum);
	delay_ms(100);
		
}


/*********************************************************************
*功    能：读取节点类型
*入口参数：
*出口参数：0 主模式；1从模式
*********************************************************************/
u8 READ_NODE(void)
{
	u8 sum;
	sum=(0XFC+0X00+0X91+0X0B+0XCB+0XEB)&0XFF;
	sendByte(0XFC);
	sendByte(0X00);
	sendByte(0X91);
	sendByte(0X0B);
	sendByte(0XCB);
	sendByte(0XEB);
	sendByte(sum);
	delay_ms(100);
	if(rec_init[5]==0X69)		//主模式
	{
		return 0;
	}
	else if(rec_init[5]==0X72)	//从模式
	{
		return 1;
	}
}


/*********************************************************************
*功    能：设置模块频道
*入口参数：
*出口参数：
*********************************************************************/
void SET_CHANNEL(u8 Channel)
{
	u8 sum;
	sum=(0XFC+0X01+0X91+0X0C+Channel+0X1A)&0XFF;
	sendByte(0XFC);
	sendByte(0X01);
	sendByte(0X91);
	sendByte(0X0C);
	sendByte(Channel);
	sendByte(0X1A);
	sendByte(sum);
	delay_ms(100);
		
}


/*********************************************************************
*功    能：读取模块频道
*入口参数：
*出口参数：频道
*********************************************************************/
u8 READ_CHANNEL(void)
{
	u8 sum;
	sum=(0XFC+0X00+0X91+0X0D+0X34+0X2B)&0XFF;
	sendByte(0XFC);
	sendByte(0X00);
	sendByte(0X91);
	sendByte(0X0D);
	sendByte(0X34);
	sendByte(0X2B);
	sendByte(sum);
	delay_ms(100);
	return rec_init[5];	
}



/*********************************************************************
*功    能：数据传输模式
*入口参数：模式
*出口参数：
*********************************************************************/
void DATA_TRANS_MODE(u8 mode)
{
	u8 sum;
	sum=(0XFC+0X01+0X91+0X64+0X58+mode)&0XFF;
	sendByte(0XFC);
	sendByte(0X01);
	sendByte(0X91);
	sendByte(0X64);
	sendByte(0X58);
	sendByte(mode);
	sendByte(sum);
	delay_ms(100);
}
	


/*********************************************************************
*功    能：模块重启
*入口参数：
*出口参数：
*********************************************************************/
void MODULE_SOFTWARE_RESTARRT(void)
{
	u8 sum;
	sum=(0XFC+0X00+0X91+0X87+0X6A+0X35)&0XFF;
	sendByte(0XFC);
	sendByte(0X00);
	sendByte(0X91);
	sendByte(0X87);
	sendByte(0X6A);
	sendByte(0X35);
	sendByte(sum);
	delay_ms(1000);
	delay_ms(1000);
}


/*********************************************************************
*功    能：zigbee波特率设置
*入口参数：
*出口参数：
*********************************************************************/
void SetBound(void)
{
	u8 i;
	u32 Bound=0;
	u32 LastBound=0;
	for(i=1;i<=2;i++)
	{
		uart1_init(115200/i);     //Zigbee初始化
		delay_ms(10);
		SET_BOUND(0X01);			//波特率9600
		if(rec_init[2]==9)
		{
			uart1_init(9600);     //Zigbee初始化
			return;
		}	
	}
	LastBound=76800;
	for(i=0;i<5;i++)
	{
		Bound=LastBound/2;
		uart1_init(Bound);     //Zigbee初始化
		LastBound=Bound;
		
		
		delay_ms(10);
		SET_BOUND(0X01);			//波特率9600
		if(rec_init[2]==9)
		{
			uart1_init(9600);     //Zigbee初始化
			return;
		}	
	}	

}

/*********************************************************************
*功    能：zigbee初始化
*入口参数：
*出口参数：
*********************************************************************/
void Zigbee_Init(void)
{	
	u8 i;

	char PIN_ID[5]={0};
	char CHANNEL[3]={0};
	u8 PIN_ID_H=0;
	u8 PIN_ID_L=0;
	u8 ChannelNum;
	FRESULT res;
	if(f_open(&fdsts,"config.txt",FA_READ)==FR_OK)
	{
		f_lseek(&fdsts,7);                          	 	//移动文件指针
		f_read(&fdsts,PIN_ID,4,&readnum);
		f_lseek(&fdsts,21);                          	 	//移动文件指针
		f_read(&fdsts,CHANNEL,2,&readnum);
		f_close(&fdsts);
	}
	PIN_ID_H=(PIN_ID[0]-0x30)*16+PIN_ID[1]-0x30;
	PIN_ID_L=(PIN_ID[2]-0x30)*16+PIN_ID[3]-0x30;
	ChannelNum=my_atoi(CHANNEL);
	SetBound();		//zigbee波特率设置
	rec_num=0;
	SET_ROUTER();				//从模式
	rec_num=0;
	SET_PIN_ID(PIN_ID_H,PIN_ID_L);		//PIN ID 12 34
	rec_num=0;
	SET_CHANNEL(ChannelNum);			//频道12
	rec_num=0;
	MODULE_SOFTWARE_RESTARRT();	//模块重启
	Zigbe_INIT=1;			//zigbee初始化后
	
}

