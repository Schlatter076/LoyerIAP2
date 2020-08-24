/*
 * tim4.c
 *
 *  Created on: 2020年7月20日
 *      Author: loyer
 */
#include "tim4.h"

volatile u8 updateCnt = 0;
volatile u8 initCnt = 0;


void Init_led(void)
{
	GPIO_InitTypeDef GPIO_InitStructure; //定义结构体变量

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;  //选择你要设置的IO口
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	 //设置推挽输出模式
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	  //设置传输速率
	GPIO_Init(GPIOA, &GPIO_InitStructure); /* 初始化GPIO */

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
	//关闭JTAG，保留SWD，释放PB3 PB4 PA15
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);

	GPIO_ResetBits(GPIOA, GPIO_Pin_15);
}


/*******************************************************************************
 * 函 数 名         : TIM4_Init
 * 函数功能		   : TIM4初始化函数
 * 输    入         : per:重装载值
 psc:分频系数
 * 输    出         : 无
 *******************************************************************************/
void TIM4_Init(u16 per, u16 psc)
{
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	Init_led();
	TIM_DeInit(TIM4);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); //使能TIM4时钟

	TIM_TimeBaseInitStructure.TIM_Period = per;   //自动装载值
	TIM_TimeBaseInitStructure.TIM_Prescaler = psc; //分频系数
	TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up; //设置向上计数模式
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);

	TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE); //开启定时器中断
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);

	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn; //定时器中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 4; //抢占优先级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 4;		//子优先级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM4, ENABLE); //使能定时器
}

/*******************************************************************************
 * 函 数 名         : TIM4_IRQHandler
 * 函数功能		   : TIM4中断函数
 * 输    入         : 无
 * 输    出         : 无
 *******************************************************************************/
void TIM4_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update))
	{
		if (updating)
		{
			updateCnt++;
			if (updateCnt == 20)
			{
				updateCnt = 0;
				LED_Status ^= 1;
			}
		}
		if (initing)
		{
			initCnt++;
			if (initCnt == 50)
			{
				initCnt = 0;
				LED_Status ^= 1;
			}
		}
	}
	TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
}

