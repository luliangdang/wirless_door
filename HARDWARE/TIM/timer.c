#include "timer.h"
#include "lcd.h"
#include "wdg.h"


/*********************************************************************
*功    能：定时器6初始化
*入口参数：
*出口参数：
*********************************************************************/
void Tim6_Init(void)
{

    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);	 	//时钟使能
	
	//定时器TIM3初始化			定时5s
	TIM_TimeBaseStructure.TIM_Period = 49999; 				 	//设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =7199; 					//设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;	 	//设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM向上计数模式
	TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure); 			//根据指定的参数初始化TIMx的时间基数单位
 
	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM6_IRQn;  			//TIM6中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;   //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;  		//从优先级0级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 			//IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  							//初始化NVIC寄存器

	TIM_ITConfig(TIM6,TIM_IT_Update,DISABLE ); 					//使能指定的TIM6中断,允许更新中断
	TIM_Cmd(TIM6, DISABLE);  									//使能TIMx					 
//	TIM6->DIER&=~0x0001;		 								//失能指定的TIM6中断,允许更新中断
//	TIM6->CR1&=~0x0001;		 									//失使能TIMx	

}

/*********************************************************************
*功    能：定时器6中断服务程序
*入口参数：
*出口参数：
*********************************************************************/
void TIM6_IRQHandler(void)   									//TIM3中断
{
	if (TIM_GetITStatus(TIM6, TIM_IT_Update) != RESET)  		//检查TIM3更新中断发生与否
		{
		TIM_ClearITPendingBit(TIM6, TIM_IT_Update  ); 			//清除TIMx更新中断标志 
		LCD_LED = 0; 											//关闭背光
		TIM6->DIER&=~0x0001;		 							//失能指定的TIM6中断,允许更新中断
		TIM6->CR1&=~0x0001;										//失使能TIMx	
		}
}




/*********************************************************************
*功    能：定时器7初始化
*入口参数：
*出口参数：
*********************************************************************/
void Tim7_Init(void)
{

    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE);	 	//时钟使能
	
	//定时器TIM3初始化			定时5s
	TIM_TimeBaseStructure.TIM_Period = 49999; 				 	//设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =7199; 					//设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;	 	//设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up; //TIM向上计数模式
	TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure); 			//根据指定的参数初始化TIMx的时间基数单位
 
	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;  			//TIM6中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;   //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;  		//从优先级0级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; 			//IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  							//初始化NVIC寄存器

	TIM_ITConfig(TIM7,TIM_IT_Update,ENABLE ); 					//使能指定的TIM7中断,允许更新中断
	TIM_Cmd(TIM7, DISABLE);  									//使能TIMx					 
}

/*********************************************************************
*功    能：定时器7中断服务程序
*入口参数：
*出口参数：
*********************************************************************/
void TIM7_IRQHandler(void)   									
{
	if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET)  		//检查TIM7更新中断发生与否
	{
		TIM_ClearITPendingBit(TIM7, TIM_IT_Update  ); 			//清除TIMx更新中断标志 
											
		IWDG_Feed();//喂狗
		
		
	}
}









