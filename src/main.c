/* Includes */
#include <stddef.h>
#include "stm32f10x.h"
#include "SysTick.h"
#include "stdio.h"
#include "usart.h"
#include "usart_4G.h"
#include "STMFlash.h"
#include "common.h"
#include "iap.h"
#include "tim4.h"

void HSI_SetSysClock(uint32_t pllmul)
{
	__IO uint32_t HSIStartUpStatus = 0;

	// 把 RCC 外设初始化成复位状态，这句是必须的
	RCC_DeInit();

	//使能 HSI
	RCC_HSICmd(ENABLE);

	// 等待 HSI 就绪
	HSIStartUpStatus = RCC->CR & RCC_CR_HSIRDY;

	// 只有 HSI 就绪之后则继续往下执行
	if (HSIStartUpStatus == RCC_CR_HSIRDY)
	{
		//-------------------------------------------------------------//

		// 使能 FLASH 预存取缓冲区
		FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);

		// SYSCLK 周期与闪存访问时间的比例设置，这里统一设置成 2
		// 设置成 2 的时候，SYSCLK 低于 48M 也可以工作，如果设置成 0 或者 1 的时候，
		// 如果配置的 SYSCLK 超出了范围的话，则会进入硬件错误，程序就死了
		// 0：0 < SYSCLK <= 24M
		// 1：24< SYSCLK <= 48M
		// 2：48< SYSCLK <= 72M
		FLASH_SetLatency(FLASH_Latency_2);
		//------------------------------------------------------------//

		// AHB 预分频因子设置为 1 分频，HCLK = SYSCLK
		RCC_HCLKConfig(RCC_SYSCLK_Div1);

		// APB2 预分频因子设置为 1 分频，PCLK2 = HCLK
		RCC_PCLK2Config(RCC_HCLK_Div1);

		// APB1 预分频因子设置为 1 分频，PCLK1 = HCLK/2
		RCC_PCLK1Config(RCC_HCLK_Div2);

		//-----------设置各种频率主要就是在这里设置-------------------//
		// 设置 PLL 时钟来源为 HSE，设置 PLL 倍频因子
		// PLLCLK = 4MHz * pllmul
		RCC_PLLConfig(RCC_PLLSource_HSI_Div2, pllmul);
		//-- -----------------------------------------------------//

		// 开启 PLL
		RCC_PLLCmd(ENABLE);

		// 等待 PLL 稳定
		while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
		{
		}

		// 当 PLL 稳定之后，把 PLL 时钟切换为系统时钟 SYSCLK
		RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

		// 读取时钟切换状态位，确保 PLLCLK 被选为系统时钟
		while (RCC_GetSYSCLKSource() != 0x08)
		{
		}
	}
	else
	{
		// 如果 HSI 开启失败，那么程序就会来到这里，用户可在这里添加出错的代码处理
		// 当 HSE 开启失败或者故障的时候，单片机会自动把 HSI 设置为系统时钟，
		// HSI 是内部的高速时钟，8MHZ
		while (1)
		{
		}
	}
}

/**
 **===========================================================================
 **
 **  Abstract: main program
 **
 **===========================================================================
 */
int main(void)
{
//	SystemInit();
	HSI_SetSysClock(RCC_PLLMul_9);
	SysTick_Init(36);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组S
	IAP_WriteFlag(INIT_FLAG_DATA);
	IAP_Init();
	printf("Now In IAP\r\n");
	TIM4_Init(10, 36000 - 1); //10ms

	//检查是否需要本地更新
	delay_ms(1000);
	printf("{localUpdate?}\r\n");
	delay_ms(1000);
	delay_ms(1000);

	F4G_Init(115200);

	/* Infinite loop */
	while (1)
	{
		switch (IAP_ReadFlag())
		{
		case APPRUN_FLAG_DATA:  //jump to app
			if (IAP_RunApp())
				IAP_WriteFlag(UPDATE_FLAG_DATA);
			break;
		case INIT_FLAG_DATA:  //initialze state (blank mcu)
			IAP_Main_Menu();
			break;
		case UPDATE_FLAG_DATA:  // download app state
			if (IAP_Update())
			{
				//IAP_WriteFlag(APPRUN_FLAG_DATA);
				NVIC_SystemReset();
			}
			else
				IAP_WriteFlag(INIT_FLAG_DATA);
			break;
		case UPLOAD_FLAG_DATA:  // upload app state
			IAP_Upload();
			IAP_WriteFlag(INIT_FLAG_DATA);
			break;
		case ERASE_FLAG_DATA:		// erase app state
			IAP_Erase();
			IAP_WriteFlag(INIT_FLAG_DATA);
			break;
		case SET_DeviceID_FLAG_DATA:		// set DeviceID
			Set_DeviceID();
			break;
		case GET_DeviceID_FLAG_DATA:		// read DeviceID
			Get_DeviceID();
			IAP_WriteFlag(INIT_FLAG_DATA);
			break;
		default:
			break;
		}
	}
}
