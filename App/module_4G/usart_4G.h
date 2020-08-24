/*
 * usart_4G.h
 *
 *  Created on: 2020年6月11日
 *      Author: loyer
 */

#ifndef USART_4G_H_
#define USART_4G_H_

#include "stm32f10x.h"
#include "wifi_public.h"
#include "stdio.h"
#include <string.h>
#include <stdbool.h>
#include "systick.h"
#include "base64.h"
#include "stdlib.h"
#include "STMFlash.h"
#include "iap.h"
#include "iap_config.h"
#include "usart.h"

#define TCPServer_IP    "server.dayitc.com"
//#define TCPServer_IP    "119.23.38.148"
#define TCPServer_PORT  "5599"

#define F4G_USART(fmt, ...)	 Wifi_USART_printf (USART2, fmt, ##__VA_ARGS__)
#define PC_USART(fmt, ...)	 printf (fmt, ##__VA_ARGS__)

#define UART2_4G_TX GPIO_Pin_2 //GPIOA 4G模块通信
#define UART2_4G_RX GPIO_Pin_3
#define RST_4G      GPIO_Pin_0 //GPIOC
#define PKEY_4G     GPIO_Pin_1

#define RST_4G_Pin_SetH     GPIO_SetBits(GPIOC,RST_4G)
#define RST_4G_Pin_SetL     GPIO_ResetBits(GPIOC,RST_4G)
#define PKEY_4G_Pin_SetH     GPIO_SetBits(GPIOC,PKEY_4G)
#define PKEY_4G_Pin_SetL     GPIO_ResetBits(GPIOC,PKEY_4G)

#define RX4G_BUF_MAX_LEN 1024		  //最大接收缓存字节数
#define BASE64_BUF_LEN 512
extern struct STRUCT_USART_Fr	  //定义一个全局串口数据帧的处理结构体
{
	char Data_RX_BUF[RX4G_BUF_MAX_LEN];
	char DeData[BASE64_BUF_LEN];
	char EnData[BASE64_BUF_LEN];
	char *base64Str;
	char ServerData[BASE64_BUF_LEN];
	char *Server_Command[2];
	char locations[2][12];
	char ccid[24];
	char cops;
	union
	{
		__IO u16 InfAll;
		struct
		{
			__IO u16 FramLength :15;                               // 14:0
			__IO u16 FramFinishFlag :1;                                // 15
		} InfBit;
	};
} F4G_Fram_Record_Struct;

void USART2_Init(uint32_t rate);
void F4G_Init(u32 bound);
void reset_4G_module(void);
void split(char str[], char *delims);
bool Send_AT_Cmd(char *cmd, char *ack1, char *ack2, u32 time);
u8 F4G_AT_Test(void);
bool connectToTCPServer(char *IP, char * PORT);
void F4G_sendStr(char *str);
void F4G_ExitUnvarnishSend(void);
void getModuleMes(void);
void request2Server(char str[]);

#endif /* USART_4G_H_ */
