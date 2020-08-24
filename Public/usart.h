/*
 * usart.h
 *
 *  Created on: 2019年10月19日
 *      Author: Loyer
 */

#ifndef USART_H_
#define USART_H_

#include "bit_band.h"
#include "STMFlash.h"

#define RX_BUF_MAX_LEN 1024		  //最大接收缓存字节数
extern struct USART_Fram	  //定义一个全局串口数据帧的处理结构体
{
	char RX_BUF[RX_BUF_MAX_LEN];
	char *Server_Command[2];
	char DATA[512];
	char ServerData[512];
	union
	{
		__IO u16 InfAll;
		struct
		{
			__IO u16 FramLength :15;                               // 14:0
			__IO u16 FramFinishFlag :1;                                // 15
		} InfBit;
	};
} USART1_Record_Struct;

extern volatile u8 localUpdate;

void USART1_Init(u32 bound);
void UT_split(char str[], char *delims);

#endif /* USART_H_ */
