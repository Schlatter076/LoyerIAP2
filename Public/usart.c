#include "usart.h"
#include "stdio.h"
#include "usart_4G.h"
#include "string.h"
#include "base64.h"

struct USART_Fram USART1_Record_Struct =
{ 0 };

volatile u8 localUpdate = 0;

//*/
/*******************************************************************************
 * 函 数 名         : USART1_Init
 * 函数功能		   : USART1初始化函数
 * 输    入         : bound:波特率
 * 输    出         : 无
 *******************************************************************************/
void USART1_Init(u32 bound)
{
	//GPIO端口设置
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/*  配置GPIO的模式和IO口 */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //TX			   //串口输出PA9
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	    //复用推挽输出
	GPIO_Init(GPIOA, &GPIO_InitStructure); /* 初始化串口输入IO */
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;	    //RX			 //串口输入PA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;		  //模拟输入
	GPIO_Init(GPIOA, &GPIO_InitStructure); /* 初始化GPIO */

	//USART1 初始化设置
	USART_InitStructure.USART_BaudRate = bound;		  //波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;		 //字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;		  //一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;		  //无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl =
	USART_HardwareFlowControl_None;		  //无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	USART_Init(USART1, &USART_InitStructure); //初始化串口1

	USART_Cmd(USART1, ENABLE);  //使能串口1

	USART_ClearFlag(USART1, USART_FLAG_TC);

	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);  //开启相关中断

	//Usart1 NVIC 配置
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;  //串口1中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;  //抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、

}
/*******************************************************************************
 * 函 数 名         : USART1_IRQHandler
 * 函数功能		   : USART1中断函数
 * 输    入         : 无
 * 输    出         : 无
 *******************************************************************************/
void USART1_IRQHandler(void)                	//串口1中断服务程序
{
	u8 r;
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //接收中断
	{
		r = USART_ReceiveData(USART1);  //(USART1->DR);	//读取接收到的数据
		if (USART1_Record_Struct.InfBit.FramLength >= RX_BUF_MAX_LEN)
		{
			printf("Cmd size over.\r\n");
			memset(USART1_Record_Struct.RX_BUF, 0, RX_BUF_MAX_LEN);
			USART1_Record_Struct.InfBit.FramLength = 0;
		}
		if (USART1_Record_Struct.InfBit.FramFinishFlag == 0) //未接收满
		{
			USART1_Record_Struct.RX_BUF[USART1_Record_Struct.InfBit.FramLength++] =
					r;
		}
		if (r == '$')
		{
			localUpdate = 1;
			USART1_Record_Struct.InfBit.FramLength = 0;
		}
		if (r == ']' && (bool) strchr(USART1_Record_Struct.RX_BUF, '['))
		{
			char *res = USART1_Record_Struct.RX_BUF;
			while (*res != '[')
			{
				res++;
			}
			res++;

			memcpy(USART1_Record_Struct.DATA, strtok(res, "]"), 512);

			USART1_Record_Struct.InfBit.FramLength = 0;
			USART1_Record_Struct.InfBit.FramFinishFlag = 1;
		}
	}
	USART_ClearFlag(USART1, USART_FLAG_TC);
}
/**
 * 将str通过delims进行分割,所得的字符串填充在res中
 * @str 待转换的数据字符串
 * @delims 分隔符
 */
void UT_split(char str[], char *delims)
{
	char *result = str;
	u8 inx = 0;
	while (inx < 2)
	{
		result++;
		if (*result == ',')
		{
			++inx;
		}
	}
	result++;
	memcpy(USART1_Record_Struct.ServerData, result, 512);
	//printf("comd2=%s\r\n", F4G_Fram_Record_Struct.ServerData);
	result = strtok(str, delims);
	USART1_Record_Struct.Server_Command[0] = result;
	result = strtok( NULL, delims);
	USART1_Record_Struct.Server_Command[1] = result;
}
