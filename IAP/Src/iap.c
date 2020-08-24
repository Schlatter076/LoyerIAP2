#include "iap_config.h"
#include "iap.h"
#include "stmflash.h"
#include "common.h"
#include "usart.h"
#include "systick.h"
#include "usart_4G.h"
#include "md5.h"

pFunction Jump_To_Application;
uint32_t JumpAddress;
uint32_t BlockNbr = 0, UserMemoryMask = 0;
__IO uint32_t FlashProtection = 0;

volatile u8 updating = 0;
volatile u8 initing = 0;

volatile u16 flagCPY = 0;
char reqBuf[128] =
{ 0 };
char spfBuf[128] =
{ 0 };

unsigned char flashBuf[128] =
{ 0 };
u16 flashCnt = 0;
u32 flashDes = ApplicationAddress;
uint32_t RamSource;
u16 flashSize = 0;

u8 LOCAL_SERVER_SWITCH = 0;
/**
 * 组装与上位机通讯时的字符串
 * @buf 填充的字符串
 * @cmd 控制字
 * @ccid 物联网卡号
 * @net 当前使用网络
 * @module 当前使用的模块
 * @ver 当前固件版本号
 * @attr 当前经纬度
 * @num  几口机型
 * @return 无
 */
void communicateWithHost(char buf[], char *cmd, char *ccid, char net,
		char *module, char *ver, char *longitude, char *latitude, char *num)
{
	const char* template = "%s,%s,%s-%c-%s-%s-%s_%s-%s";
	ReadDeviceID();
	sprintf(buf, template, RDeviceID, cmd, ccid, net, module, ver, longitude,
			latitude, num);
}

void catRequestStr(char buf[], char *cmd)
{
	ReadVersion();
	communicateWithHost(buf, cmd, F4G_Fram_Record_Struct.ccid,
			F4G_Fram_Record_Struct.cops, "2", RVersion,
			F4G_Fram_Record_Struct.locations[0],
			F4G_Fram_Record_Struct.locations[1], "06");
}

void catReqStr4UPdate(char buf[], char *cmd, u16 num)
{
	const char* template = "%s,%s,%d";
	ReadDeviceID();
	sprintf(buf, template, RDeviceID, cmd, num);
}
/**
 * 向上位机及服务器发起请求
 * @cmd 请求的控制字
 * @choose 选择开关 高4位控制上位机  低4位控制服务器
 */
void request(char *cmd, u8 choose)
{
	char rBuf[128];
	catRequestStr(rBuf, cmd);
	if (choose & 0x0F)
	{
		request2Server(rBuf);
	}
	if (choose & 0xF0)
	{
		sprintf(spfBuf, "{%s}", rBuf);
		printf("%s\r\n", spfBuf);
	}
}
void request4UPdate(char *cmd, u16 num, u8 choose)
{
	char rBuf[128];
	catReqStr4UPdate(rBuf, cmd, num);
	if (choose & 0x0F)
	{
		request2Server(rBuf);
	}
	if (choose & 0xF0)
	{
		sprintf(spfBuf, "{%s}", rBuf);
		printf("%s\r\n", spfBuf);
	}
}

void request4UPdataComplete(u8 choose)
{
	char rBuf[128];
	ReadVersion();
	ReadDeviceID();
	sprintf(rBuf, "%s,%s,%s", (const char *) RDeviceID, REQ_UPDATE_FINSHED,
			RVersion);
	if (choose & 0x0F)
	{
		request2Server(rBuf);
	}
	if (choose & 0xF0)
	{
		sprintf(spfBuf, "{%s}", rBuf);
		printf("%s\r\n", spfBuf);
	}
}
/************************************************************************/
static void FLASH_DisableWriteProtectionPages(void)
{
	uint32_t useroptionbyte = 0, WRPR = 0;
	uint16_t var1 = OB_IWDG_SW, var2 = OB_STOP_NoRST, var3 = OB_STDBY_NoRST;
	FLASH_Status status = FLASH_BUSY;

	WRPR = FLASH_GetWriteProtectionOptionByte();

	if ((WRPR & UserMemoryMask) != UserMemoryMask)
	{
		useroptionbyte = FLASH_GetUserOptionByte();

		UserMemoryMask |= WRPR;

		status = FLASH_EraseOptionBytes();

		if (UserMemoryMask != 0xFFFFFFFF)
		{
			status = FLASH_EnableWriteProtection((uint32_t) ~UserMemoryMask);
		}
		if ((useroptionbyte & 0x07) != 0x07)
		{
			if ((useroptionbyte & 0x01) == 0x0)
			{
				var1 = OB_IWDG_HW;
			}
			if ((useroptionbyte & 0x02) == 0x0)
			{
				var2 = OB_STOP_RST;
			}
			if ((useroptionbyte & 0x04) == 0x0)
			{
				var3 = OB_STDBY_RST;
			}
			FLASH_UserOptionByteConfig(var1, var2, var3);
		}

		if (status == FLASH_COMPLETE)
		{
			printf("Write Protection Close.\r\n");
			NVIC_SystemReset();
		}
		else
		{
			printf("Error: unprotection failed.\r\n");
		}
	}
	else
	{
		printf("Flash not write protected.\r\n");
	}
}

/************************************************************************/
void IAP_WriteFlag(uint16_t flag)
{
	FLASH_Unlock();
	STMFLASH_Write(IAP_FLAG_ADDR, &flag, 1);
	FLASH_Lock();
}
/************************************************************************/
uint16_t IAP_ReadFlag(void)
{
	return STMFLASH_ReadHalfWord(IAP_FLAG_ADDR);
}

/************************************************************************/
void IAP_USART_Init(void)
{
	USART_DeInit(USART1);
	USART1_Init(115200);
}
void IAP_Init(void)
{
	IAP_USART_Init();
	ReadDeviceID();
	//判定是否已经录入了设备编号
	if (*RDeviceID != 'P')
	{
		flagCPY = IAP_ReadFlag();
		IAP_WriteFlag(SET_DeviceID_FLAG_DATA);
	}
}
/************************************************************************/
u8 IAP_RunApp(void)
{
	if (((*(__IO uint32_t*) ApplicationAddress) & 0x2FFE0000) == 0x20000000)
	{
		printf("\r\nRun to app.\r\n");
		RCC_DeInit(); //关闭外设
		__disable_irq();
		TIM_Cmd(TIM4, DISABLE); //关闭定时器4
		USART_Cmd(USART1, DISABLE);
		USART_Cmd(USART2, DISABLE);
		JumpAddress = *(__IO uint32_t*) (ApplicationAddress + 4);
		Jump_To_Application = (pFunction) JumpAddress;
		__set_MSP(*(__IO uint32_t*) ApplicationAddress);
		Jump_To_Application();
		return 0;
	}
	else
	{
		printf("\r\nRun to app error.\r\n");
		return -1;
	}
}
/************************************************************************/
void IAP_Main_Menu(void)
{
	BlockNbr = (ApplicationAddress - 0x08000000) >> 12;

#if defined (STM32F10X_MD) || defined (STM32F10X_MD_VL)
	UserMemoryMask = ((uint32_t)~((1 << BlockNbr) - 1));
#else /* USE_STM3210E_EVAL */
	if (BlockNbr < 62)
	{
		UserMemoryMask = ((uint32_t) ~((1 << BlockNbr) - 1));
	}
	else
	{
		UserMemoryMask = ((uint32_t) 0x80000000);
	}
#endif 

	if ((FLASH_GetWriteProtectionOptionByte() & UserMemoryMask)
			!= UserMemoryMask)
	{
		FlashProtection = 1;
	}
	else
	{
		FlashProtection = 0;
	}
	printf("Now enter the main menu!\r\n");
	while (1)
	{
		if (FlashProtection != 0) //There is write protected
		{
			printf("diswp\r\n");
		}
		request(REQ_REGISTER, 0xFF);
		delay_ms(500);
		if (F4G_Fram_Record_Struct.InfBit.FramFinishFlag)
		{
			F4G_Fram_Record_Struct.InfBit.FramFinishFlag = 0;
			LOCAL_SERVER_SWITCH = 0x0F;
			split(F4G_Fram_Record_Struct.DeData, ",");
			if (check_response(F4G_Fram_Record_Struct.Server_Command[1]))
				return;
		}
		else if (USART1_Record_Struct.InfBit.FramFinishFlag)
		{
			USART1_Record_Struct.InfBit.FramFinishFlag = 0; //清除标志位
			LOCAL_SERVER_SWITCH = 0xF0;
			UT_split(USART1_Record_Struct.DATA, ",");
			if (check_response(USART1_Record_Struct.Server_Command[1]))
				return;
		}
	}
}
/************************************************************************/
/**
 * 校验响应的字符串
 */
u8 check_response(char *str)
{
	if (memcmp(str, RES_REGISTER_AND_UPDATE, 2) == 0)
	{
		IAP_WriteFlag(UPDATE_FLAG_DATA);
		return 1;
	}
	else if (memcmp(str, RES_UPLOAD, 2) == 0)
	{
		IAP_WriteFlag(UPLOAD_FLAG_DATA);
		return 1;
	}
	else if (memcmp(str, RES_ERASE, 2) == 0)
	{
		IAP_WriteFlag(ERASE_FLAG_DATA);
		return 1;
	}
	else if (memcmp(str, RES_REBOOT, 2) == 0)
	{
		IAP_WriteFlag(INIT_FLAG_DATA);
		NVIC_SystemReset();
	}
	else if (memcmp(str, RES_REGISTER_SUCCESS, 2) == 0)
	{
		if (LOCAL_SERVER_SWITCH == 0x0F)
		{
			printf("server params=%s\r\n", F4G_Fram_Record_Struct.ServerData);
			WriteAPPServer(F4G_Fram_Record_Struct.ServerData);
			F4G_ExitUnvarnishSend();
			Send_AT_Cmd("AT+CIPCLOSE", "OK", NULL, 500);
			Send_AT_Cmd("AT+RSTSET", "OK", NULL, 500);
		}
		else
		{
			printf("local params=%s\r\n", USART1_Record_Struct.ServerData);
			WriteAPPServer(USART1_Record_Struct.ServerData);
		}
		IAP_WriteFlag(APPRUN_FLAG_DATA);
		return 1;
	}
	else if (memcmp(str, RES_REGISTER_ERROR, 2) == 0)
	{
		IAP_WriteFlag(INIT_FLAG_DATA);
		return 1;
	}
	else if (memcmp(str, RES_DIS_WP, 2) == 0)
	{
		FLASH_DisableWriteProtectionPages();
	}
	else if (memcmp(str, RES_SET_ID, 2) == 0)
	{
		IAP_WriteFlag(SET_DeviceID_FLAG_DATA);
		request(REQ_SET_ID, 0xF0);
		return 1;
	}
	else if (memcmp(str, RES_GET_ID, 2) == 0)
	{
		IAP_WriteFlag(GET_DeviceID_FLAG_DATA);
		return 1;
	}
	else
	{
		printf(" Invalid CMD !\r\n");
	}
	return 0;
}

int8_t IAP_Update(void)
{
	char *result;
	char md5[32];
	long appBytesSize = 0L;
	u16 frameSize = 0;
	u16 len = 0;
	u16 size = 0;
	uint32_t j;
	char m[32] =
	{ 0 };
	u8 reqCnt = 0;
	char *temData = NULL;
	char *BinStr = NULL;
	u16 step = 0;

	printf("Update begin.\r\n");
	updating = 1;
	while (1)
	{
		request(REQ_VERSION, LOCAL_SERVER_SWITCH); //请求固件版本信息
		delay_ms(500);
		if (F4G_Fram_Record_Struct.InfBit.FramFinishFlag)
		{
			F4G_Fram_Record_Struct.InfBit.FramFinishFlag = 0;
			split(F4G_Fram_Record_Struct.DeData, ",");
			//获取版本信息
			if (memcmp(F4G_Fram_Record_Struct.Server_Command[1],
			REQ_CURRENT_VERSION, 2) == 0)
			{
				result = strtok(F4G_Fram_Record_Struct.ServerData, "-");
				if (result != NULL)
				{
					WVersion = result;
					writeVersion();
					ReadVersion();
				}
				result = strtok(NULL, "-");
				appBytesSize = result == NULL ? 0 : atol(result);
				result = strtok(NULL, "-");
				frameSize = result == NULL ? 0 : atoi(result);
				result = strtok(NULL, "-");
				if (result != NULL)
				{
					memcpy(md5, result, 32);
				}
				//判定数据是否接收正确
				if (WVersion != NULL
						&& appBytesSize != 0&& frameSize != 0 && md5 != NULL)
				{
					printf("ver=%s,appSize=%ld,frameSize=%d,md5=%s\r\n",
							WVersion, appBytesSize, frameSize, md5);
					break;
				}
				else
				{
					request(REQ_ERROR_REPORT, LOCAL_SERVER_SWITCH);
					updating = 0;
					return 0;
				}
			}
		}
		else if (USART1_Record_Struct.InfBit.FramFinishFlag)
		{
			USART1_Record_Struct.InfBit.FramFinishFlag = 0;
			UT_split(USART1_Record_Struct.DATA, ",");
			//获取版本信息
			if (memcmp(USART1_Record_Struct.Server_Command[1],
			REQ_CURRENT_VERSION, 2) == 0)
			{
				result = strtok(USART1_Record_Struct.ServerData, "-");
				if (result != NULL)
				{
					WVersion = result;
					writeVersion();
					ReadVersion();
				}
				result = strtok(NULL, "-");
				appBytesSize = result == NULL ? 0 : atol(result);
				result = strtok(NULL, "-");
				frameSize = result == NULL ? 0 : atoi(result);
				result = strtok(NULL, "-");
				if (result != NULL)
				{
					memcpy(md5, result, 32);
				}
				//判定数据是否接收正确
				if (WVersion != NULL
						&& appBytesSize != 0&& frameSize != 0 && md5 != NULL)
				{
					printf("ver=%s,appSize=%ld,frameSize=%d,md5=%s\r\n",
							WVersion, appBytesSize, frameSize, md5);
					break;
				}
				else
				{
					request(REQ_ERROR_REPORT, LOCAL_SERVER_SWITCH);
					updating = 0;
					return 0;
				}
			}
		}
	}

	len = (appBytesSize % frameSize == 0) ?
			appBytesSize / frameSize : (appBytesSize / frameSize + 1);

	IAP_Erase();

	for (size = 0; size < len; size++)
	{
//		do
//		{
//			if (reqCnt > 50)
//			{
//				request(REQ_ERROR_REPORT, LOCAL_SERVER_SWITCH);
//				updating = 0;
//				return 0;
//			}
//			request4UPdate(REQ_REQUEST_FRAME, size, LOCAL_SERVER_SWITCH);
//			delay_ms(200);
//			reqCnt++;
//		}
		request4UPdate(REQ_REQUEST_FRAME, size, LOCAL_SERVER_SWITCH);
		//等待服务端数据返回
		while (F4G_Fram_Record_Struct.InfBit.FramFinishFlag == 0
				&& USART1_Record_Struct.InfBit.FramFinishFlag == 0);
		reqCnt = 0;
		if (F4G_Fram_Record_Struct.InfBit.FramFinishFlag)
		{
			F4G_Fram_Record_Struct.InfBit.FramFinishFlag = 0;
			split(F4G_Fram_Record_Struct.DeData, ",");
			//获取版本信息
			if (memcmp(F4G_Fram_Record_Struct.Server_Command[1],
			RES_RESPONSE_FRAME, 2) == 0)
			{
				step = atoi(strtok(F4G_Fram_Record_Struct.ServerData, "-"));
				if (step == size)
				{
					temData = F4G_Fram_Record_Struct.ServerData;
					while (*temData != '(')
					{
						temData++;
					}
					temData++;
//					printf("datas=%s\r\n", temData);
					//取出二进制文件
					BinStr = strtok(temData, ",");
					flashBuf[0] = atoi((const char *) BinStr);
					printf("\r\nframe%d=(%02X,", size, flashBuf[0]);
					for (flashCnt = 1; flashCnt < frameSize; flashCnt++)
					{
						BinStr = strtok(NULL, ",");
						flashBuf[flashCnt] = atoi((const char *) BinStr);
						printf("%02X,", flashBuf[flashCnt]);
					}
					printf(")\r\n");
					RamSource = (uint32_t) flashBuf;
					if (size < len - 1)
					{
						for (j = 0;
								(j < frameSize)
										&& (flashDes
												< ApplicationAddress
														+ appBytesSize); j += 4)
						{
							/* Program the data received into STM32F10x Flash */
							FLASH_Unlock();
							FLASH_ProgramWord(flashDes,
									*(uint32_t *) RamSource);
							FLASH_Lock();
							if (*(uint32_t *) flashDes
									!= *(uint32_t *) RamSource)
							{
								printf("write to flash error,addr=%08X\r\n",
										flashDes);
								delay_ms(1000);
								delay_ms(1000);
								delay_ms(1000);
								updating = 0;
								return 0;
							}
							flashDes += 4;
							RamSource += 4;
						}
					}
				}
			}
		}
		else if (USART1_Record_Struct.InfBit.FramFinishFlag)
		{
			USART1_Record_Struct.InfBit.FramFinishFlag = 0;
			UT_split(USART1_Record_Struct.DATA, ",");
			//获取版本信息
			if (memcmp(USART1_Record_Struct.Server_Command[1],
			RES_RESPONSE_FRAME, 2) == 0)
			{
				step = atoi(strtok(USART1_Record_Struct.ServerData, "-"));
				if (step == size)
				{
					temData = USART1_Record_Struct.ServerData;
					while (*temData != '(')
					{
						temData++;
					}
					temData++;
//					printf("datas=%s\r\n", temData);
					//取出二进制文件
					BinStr = strtok(temData, ",");
					flashBuf[0] = atoi((const char *) BinStr);
					printf("\r\nframe%d=(%02X,", size, flashBuf[0]);
					for (flashCnt = 1; flashCnt < frameSize; flashCnt++)
					{
						BinStr = strtok(NULL, ",");
						flashBuf[flashCnt] = atoi((const char *) BinStr);
						printf("%02X,", flashBuf[flashCnt]);
					}
					printf(")\r\n");
					RamSource = (uint32_t) flashBuf;
//					if (size < len - 1)
//					{
					for (j = 0;
							(j < frameSize)
									&& (flashDes
											< ApplicationAddress + appBytesSize);
							j += 4)
					{
						/* Program the data received into STM32F10x Flash */
						FLASH_Unlock();
						FLASH_ProgramWord(flashDes, *(uint32_t *) RamSource);
						FLASH_Lock();
						if (*(uint32_t *) flashDes != *(uint32_t *) RamSource)
						{
							printf("write to flash error,addr=%08X\r\n",
									flashDes);
							delay_ms(1000);
							delay_ms(1000);
							delay_ms(1000);
							updating = 0;
							return 0;
						}
						flashDes += 4;
						RamSource += 4;
					}
//					}
				}
			}
		}
	}
//	printf("last frame=(");
//	for (flashCnt = 0; flashCnt < frameSize; flashCnt++)
//	{
//		printf("%02X,", flashBuf[flashCnt]);
//	}
//	printf(")\r\n");

	//进行MD5校验
	getMD5Str(m, flashBuf, 128);
//	if (strncmp((const char *) m, (const char *) md5, 31) == 0)
//	{
	printf("All datas has been recieved,md5=%s!\r\n", m);

//		RamSource = (uint32_t) flashBuf;
//		for (j = 0;
//				(j < frameSize)
//						&& (flashDes < ApplicationAddress + appBytesSize); j +=
//						4)
//		{
//			/* Program the data received into STM32F10x Flash */
//			FLASH_Unlock();
//			FLASH_ProgramWord(flashDes, *(uint32_t *) RamSource);
//			FLASH_Lock();
//			if (*(uint32_t *) flashDes != *(uint32_t *) RamSource)
//			{
//				printf("write to flash error,addr=%ld\r\n", flashDes);
//				return 0;
//			}
//			flashDes += 4;
//			RamSource += 4;
//		}
	u8 cnt = 0;
	while (cnt < 10)
	{
		cnt++;
		request4UPdataComplete(LOCAL_SERVER_SWITCH);
		delay_ms(500);
		if (F4G_Fram_Record_Struct.InfBit.FramFinishFlag)
		{
			F4G_Fram_Record_Struct.InfBit.FramFinishFlag = 0;
			split(F4G_Fram_Record_Struct.DeData, ",");
			//获取版本信息
			if (memcmp(F4G_Fram_Record_Struct.Server_Command[1],
			RES_UPDATE_FINEHED_REPONSE, 2) == 0)
			{
				printf("update data checked success!\r\n");
				break;
			}
		}
		else if (USART1_Record_Struct.InfBit.FramFinishFlag)
		{
			USART1_Record_Struct.InfBit.FramFinishFlag = 0;
			UT_split(USART1_Record_Struct.DATA, ",");
			//获取版本信息
			if (memcmp(USART1_Record_Struct.Server_Command[1],
			RES_UPDATE_FINEHED_REPONSE, 2) == 0)
			{
				printf("update data checked success!\r\n");
				break;
			}
		}
	}
//	}
//	else
//	{
//		printf("md5 checked fail,md5=%s!\r\n", m);
//		//校验失败
//		return 0;
//	}
	updating = 0;
	return 1;
}

/************************************************************************/
int8_t IAP_Upload(void)
{
	printf("Not currently supported.\r\n");
	return 0;
}

/************************************************************************/
int8_t IAP_Erase(void)
{
	uint8_t erase_cont[3] =
	{ 0 };
	Int2Str(erase_cont, FLASH_IMAGE_SIZE / PAGE_SIZE);
	printf("Total erase_cont=%s\r\n", erase_cont);
	if (EraseSomePages(FLASH_IMAGE_SIZE, 1))
		return 0;
	else
		return -1;
}
u8 Set_DeviceID(void)
{
	while (USART1_Record_Struct.InfBit.FramFinishFlag == 0)
	{
		printf("{DeviceID}\r\n");
		delay_ms(500);
	}
	char *cmdCP = USART1_Record_Struct.RX_BUF;
	while (*cmdCP != '[')
		cmdCP++;
	cmdCP++;
	WDeviceID = strtok((char *) cmdCP, "]");
	printf("Now set DeviceID=%s.\r\n", WDeviceID);
	WriteDeviceID();
	ReadDeviceID();
	USART1_Record_Struct.InfBit.FramFinishFlag = 0;
	IAP_WriteFlag(INIT_FLAG_DATA);
	if (flagCPY != 0)
	{
		IAP_WriteFlag(flagCPY);
		flagCPY = 0;
	}
	return 0;
}
void Get_DeviceID(void)
{
	ReadDeviceID();
	printf("Current DeviceID=%s.\r\n", RDeviceID);
}
