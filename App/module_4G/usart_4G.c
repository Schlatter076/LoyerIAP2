/*
 * usart_4G.c
 *
 *  Created on: 2020��6��11��
 *      Author: loyer
 */
#include "usart_4G.h"

struct STRUCT_USART_Fr F4G_Fram_Record_Struct =
{ 0 };

void USART2_Init(uint32_t bound)
{
	USART_DeInit(USART2);

	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA2  TXD
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;	//PA3  RXD
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;	//��������
	GPIO_Init(GPIOA, &GPIO_InitStructure);	//��ʼ��GPIOA3

	//Usart2 NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	//��ռ���ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//�����ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���

	//USART2 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;	//���ڲ�����
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;	//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;	//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;	//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl =
	USART_HardwareFlowControl_None;	//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
	USART_Init(USART2, &USART_InitStructure); //��ʼ������2

	USART_ITConfig(USART2, USART_IT_RXNE | USART_IT_IDLE, ENABLE); //�������ڽ��ܺ����߿����ж�

	USART_Cmd(USART2, ENABLE);
}

void F4G_Init(u32 bound)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	initing = 1;

	USART2_Init(bound);

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = PKEY_4G | RST_4G;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 	 //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	 //IO���ٶ�Ϊ50MHz
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	if (!localUpdate)
	{
		//����
		do
		{
			PKEY_4G_Pin_SetH;
			delay_ms(1000);
			delay_ms(1000);
			//delay_ms(1000);
			//delay_ms(1000);
			PKEY_4G_Pin_SetL;
			//��λ4Gģ��
			RST_4G_Pin_SetH;
			delay_ms(1100);
			RST_4G_Pin_SetL;
//			delay_ms(500);

			F4G_ExitUnvarnishSend();
			Send_AT_Cmd("AT+CIPCLOSE", "OK", NULL, 500);
			//Send_AT_Cmd("AT+RSTSET", "OK", NULL, 500);
		} while (!F4G_AT_Test());

		//���������Ϣ
		getModuleMes();
		while (!connectToTCPServer(TCPServer_IP, TCPServer_PORT))
			;
		//initing = 0;
	}
}

void USART2_IRQHandler(void)
{
	u8 ucCh;
	if (USART_GetITStatus( USART2, USART_IT_RXNE) != RESET)
	{
		ucCh = USART_ReceiveData( USART2);

		if (F4G_Fram_Record_Struct.InfBit.FramLength < (RX4G_BUF_MAX_LEN - 1))
		{
			F4G_Fram_Record_Struct.Data_RX_BUF[F4G_Fram_Record_Struct.InfBit.FramLength++] =
					ucCh;
		}
		else
		{
			printf("4G Cmd size over.\r\n");
			memset(F4G_Fram_Record_Struct.Data_RX_BUF, 0, RX4G_BUF_MAX_LEN);
			F4G_Fram_Record_Struct.InfBit.FramLength = 0;
		}
		//�յ��������˷��ص�����
		if (ucCh == ']'
				&& (bool) strchr(F4G_Fram_Record_Struct.Data_RX_BUF, '['))
		{
			char *res = F4G_Fram_Record_Struct.Data_RX_BUF;
			while (*res != '[')
			{
				res++;
			}
			res++;

			F4G_Fram_Record_Struct.base64Str = strtok(res, "]");
//			printf("before Decryption=%s\r\n",
//					F4G_Fram_Record_Struct.base64Str);
			base64_decode((const char *) F4G_Fram_Record_Struct.base64Str,
					(unsigned char *) F4G_Fram_Record_Struct.DeData);
			printf("after Decryption=%s\r\n", F4G_Fram_Record_Struct.DeData);
			//split(F4G_Fram_Record_Struct.DeData, ",");
			F4G_Fram_Record_Struct.InfBit.FramLength = 0;
			F4G_Fram_Record_Struct.InfBit.FramFinishFlag = 1;
		}
	}
	USART_ClearFlag(USART2, USART_FLAG_TC);
}
/**
 * ��λ4Gģ��
 */
void reset_4G_module(void)
{
	//��λ4Gģ��
	RST_4G_Pin_SetH;
	//PKEY_4G_Pin_SetL;
	delay_ms(1100);
	RST_4G_Pin_SetL;
	//PKEY_4G_Pin_SetH;
}

/**
 * ��strͨ��delims���зָ�,���õ��ַ��������res��
 * @str ��ת���������ַ���
 * @delims �ָ���
 */
void split(char str[], char *delims)
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
	memcpy(F4G_Fram_Record_Struct.ServerData, result, BASE64_BUF_LEN);
	//printf("comd2=%s\r\n", F4G_Fram_Record_Struct.ServerData);
	result = strtok(str, delims);
	F4G_Fram_Record_Struct.Server_Command[0] = result;
	result = strtok( NULL, delims);
	F4G_Fram_Record_Struct.Server_Command[1] = result;
}

/**
 * ��F4Gģ�鷢��ATָ��
 * @cmd�������͵�ָ��
 * @ack1 @ack2���ڴ�����Ӧ��ΪNULL������Ӧ������Ϊ���߼���ϵ
 * @time���ȴ���Ӧ��ʱ��
 * @return 1�����ͳɹ� 0��ʧ��
 */
bool Send_AT_Cmd(char *cmd, char *ack1, char *ack2, u32 time)
{
	F4G_Fram_Record_Struct.InfBit.FramLength = 0; //���¿�ʼ�������ݰ�
	F4G_USART("%s\r\n", cmd);
	if (ack1 == 0 && ack2 == 0)	 //����Ҫ��������
	{
		return true;
	}
	delay_ms(time);	  //��ʱtimeʱ��

	F4G_Fram_Record_Struct.Data_RX_BUF[F4G_Fram_Record_Struct.InfBit.FramLength] =
			'\0';

	PC_USART("%s", F4G_Fram_Record_Struct.Data_RX_BUF);

	if (ack1 != 0 && ack2 != 0)
	{
		return ((bool) strstr(F4G_Fram_Record_Struct.Data_RX_BUF, ack1)
				|| (bool) strstr(F4G_Fram_Record_Struct.Data_RX_BUF, ack2));
	}
	else if (ack1 != 0)
		return ((bool) strstr(F4G_Fram_Record_Struct.Data_RX_BUF, ack1));

	else
		return ((bool) strstr(F4G_Fram_Record_Struct.Data_RX_BUF, ack2));

}
/**
 * ����4Gģ���Ƿ�����(֧��ATָ��)
 */
u8 F4G_AT_Test(void)
{
	char count = 0;
	while (count < 5)
	{
		Send_AT_Cmd("AT", "OK", NULL, 500);
		++count;
	}
	if (Send_AT_Cmd("AT", "OK", NULL, 500))
	{
		printf("test 4G module success!\r\n");
		return 1;
	}
	printf("test 4G module fail!\r\n");
	return 0;
}
/**
 * ���ӵ�������
 * @IP ������ip��ַ
 * @PORT ���������ŵĶ˿�
 * @return 1--���ӳɹ�  0--����ʧ��
 */
bool connectToTCPServer(char *IP, char * PORT)
{
	char gCmd[100] =
	{ 0 };
	//1.����ģʽΪTCP͸��ģʽ
	Send_AT_Cmd("AT+CIPMODE=1", "OK", NULL, 500);
	sprintf(gCmd, "AT+CIPSTART=\"TCP\",\"%s\",%s", IP, PORT);
	return Send_AT_Cmd(gCmd, "OK", NULL, 1800);
}
/**
 * ͸��ģʽ��4Gģ�鷢���ַ���
 */
void F4G_sendStr(char *str)
{
	u8 i = 0;
	char cmd[128] = "{(";
	strcat(cmd, str);
	strcat(cmd, "}");
//	sprintf(cmd, "{(%s}", str);
	while (cmd[i] != '}')
	{
		USART_SendData(USART2, cmd[i]);
		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
			;
		i++;
	}
	USART_SendData(USART2, cmd[i]);
	while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET)
		;
//	F4G_USART("%s", cmd);
}

/**
 * 4Gģ���˳�͸��ģʽ
 */
void F4G_ExitUnvarnishSend(void)
{
	delay_ms(1000);
	F4G_USART("+++");
	delay_ms(500);
}
/***********************���¿�ʼΪ�������ͨ��ҵ����벿��*************************************/
/**
 * ��ȡģ��������Ϣ
 */
void getModuleMes(void)
{
	char *result = NULL;
	u8 inx = 0;
//	Send_AT_Cmd("AT+SAPBR=0,1", "OK", NULL, 500); //ȥ����
//
//	while (!Send_AT_Cmd("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"", "OK", NULL, 500))
//		;
//	while (!Send_AT_Cmd("AT+SAPBR=3,1,\"APN\",\"\"", "OK", NULL, 500))
//		;
//	while (!Send_AT_Cmd("AT+SAPBR=1,1", "OK", NULL, 500))
//		;
////	while (!Send_AT_Cmd("AT+SAPBR=2,1", "OK", NULL, 500))
////		;
//	//AT + CIPGSMLOC = 1, 1
//	//��ȡ��γ�Ⱥ�ʱ��
	while (!Send_AT_Cmd("AT+CIPGSMLOC=1,1", "OK", NULL, 500))
		;
//	result = strtok(F4G_Fram_Record_Struct.Data_RX_BUF, ",");
//	result = strtok( NULL, ",");
//	result = strtok( NULL, ",");
//	strcpy(F4G_Fram_Record_Struct.locations[0], result);
//	printf("longitude is \"%s\"\n", F4G_Fram_Record_Struct.locations[0]);
//	result = strtok( NULL, ",");
//	strcpy(F4G_Fram_Record_Struct.locations[1], result);
//	printf("latitude is \"%s\"\n", F4G_Fram_Record_Struct.locations[1]);

	strcpy(F4G_Fram_Record_Struct.locations[0], "0");
	strcpy(F4G_Fram_Record_Struct.locations[1], "0");

	//��ȡ����������
	while (!Send_AT_Cmd("AT+CCID", "OK", NULL, 500))
		;
	result = F4G_Fram_Record_Struct.Data_RX_BUF;
	inx = 0;
	while (!(*result <= '9' && *result >= '0'))
	{
		result++;
	}
	//��ֵΪ��ĸ������ʱ
	while ((*result <= '9' && *result >= '0')
			|| (*result <= 'Z' && *result >= 'A')
			|| (*result <= 'z' && *result >= 'a'))
	{
		F4G_Fram_Record_Struct.ccid[inx++] = *result;
		result++;
	}
	printf("CCID=%s\r\n", F4G_Fram_Record_Struct.ccid);

	//��ȡģ��������Ϣ
	while (!Send_AT_Cmd("AT+COPS=0,1", "OK", NULL, 1000))
		;
	while (!Send_AT_Cmd("AT+COPS?", "OK", NULL, 500))
		;
	if ((bool) strstr(F4G_Fram_Record_Struct.Data_RX_BUF, "CMCC"))
	{
		F4G_Fram_Record_Struct.cops = '3';
	}
	else if ((bool) strstr(F4G_Fram_Record_Struct.Data_RX_BUF, "UNICOM"))
	{
		F4G_Fram_Record_Struct.cops = '6';
	}
	else
	{
		F4G_Fram_Record_Struct.cops = '9';
	}
	printf("COPS is \"%c\"\n", F4G_Fram_Record_Struct.cops);
	//��ʱ����ͨAPN����
	if (F4G_Fram_Record_Struct.cops == '3')
	{
		while (!Send_AT_Cmd("AT+CPNETAPN=1,cmiot,\"\",\"\",0", "OK",
		NULL, 1800))
			;
	}
	else if (F4G_Fram_Record_Struct.cops == '6')
	{
		while (!Send_AT_Cmd("AT+CPNETAPN=1,UNIM2M.NJM2MAPN,\"\",\"\",0", "OK",
		NULL, 1800))
			;
	}
	else if (F4G_Fram_Record_Struct.cops == '9')
	{
		while (!Send_AT_Cmd("AT+CPNETAPN=1,CTNET,\"\",\"\",0", "OK",
		NULL, 1800))
			;
	}
	delay_ms(1000); //ǿ�Ƶȴ�1S
}
/**
 * ���������������
 */
void request2Server(char str[])
{
	//	const unsigned char *enc = RegBuf;
//	const unsigned char f4g[128] =
//			"PM610001,00,89860416141871905416-3-2-sixBT20200713-_-06";
//	char buf[128];
//	base64_encode(f4g, buf);
	base64_encode((const unsigned char *) str, F4G_Fram_Record_Struct.EnData);
//	base64_encode("PM000204,00,89860403101873285015-1-2-imid20190909-103.25883484_30.56869316-12-11111", F4G_Fram_Record_Struct.EnData);
	//printf("encode=>%s\r\n", F4G_Fram_Record_Struct.EnData);
	F4G_sendStr(F4G_Fram_Record_Struct.EnData);
}

