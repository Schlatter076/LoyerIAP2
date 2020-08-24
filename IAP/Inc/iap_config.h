#ifndef __IAP_CONFIG_H__
#define __IAP_CONFIG_H__
/* Define if use bkp save flag  -------------------------------*/
#define USE_BKP_SAVE_FLAG     0

/* Define the APP start address -------------------------------*/
#define ApplicationAddress    0x8010000

/* Output printer switch --------------------------------------*/

/* Bootloader command -----------------------------------------*/
#define BootLoader_Version    "sixBT20200713"
//#define CMD_UPDATE_STR        "update"
//#define CMD_UPLOAD_STR        "upload"
//#define CMD_ERASE_STR		  "erase"
//#define CMD_MENU_STR          "menu"
//#define CMD_RUNAPP_STR        "runapp"
//#define CMD_ERROR_STR         "error"
//#define CMD_DISWP_STR         "diswp"//禁止写保护
//#define CMD_setID_STR         "setID"
//#define CMD_getID_STR         "getID"

/**
 * 1.1 主动上行 上电注册
 */
#define REQ_REGISTER  "00"
/**
 * 1.2 从动下行 注册失败
 */
#define RES_REGISTER_ERROR  "01"
/**
 * 1.3 从动下行 要求更新固件
 */
#define RES_REGISTER_AND_UPDATE  "02"
/**
 * 1.4 从动下行 注册成功
 */
#define RES_REGISTER_SUCCESS  "03"
// ************************** 2.2 固件更新 ****************************//
/**
 * 2.1 主动上行 请求固件版本信息
 */
#define REQ_VERSION  "04"
/**
 * 2.2 从动下行 版本信息
 */
#define REQ_CURRENT_VERSION  "05"
/**
 * 2.3 主动上行 请求第N帧固件
 */
#define REQ_REQUEST_FRAME  "06"
/**
 * 2.4 从动下行 第N帧固件内容
 */
#define RES_RESPONSE_FRAME  "07"
/**
 * 2.5 主动上行 完成固件更新
 */
#define REQ_UPDATE_FINSHED  "08"
/**
 * 2.6 从动下行 响应收到设备完成更新
 */
#define RES_UPDATE_FINEHED_REPONSE  "09"
// ************************** 2.3 上报应用服务器连接情况(预留) ****************************//
// ************************** 2.4 上报异常 ****************************//
/**
 * 4.1 主动上行 上报异常
 */
#define REQ_ERROR_REPORT  "E0"
/**
 * 4.2 从动下行 响应收到异常上报
 */
#define RES_ERROR_REPORT_RECEIVED  "E1"

#define RES_UPLOAD  "F1"
#define RES_ERASE   "F2"
#define RES_REBOOT  "F3"
#define RES_DIS_WP  "F4"
#define RES_GET_ID  "F5"
#define RES_SET_ID  "F6"
#define REQ_SET_ID  "F7"

/* IAP command------------------------------------------------ */
#define IAP_FLAG_ADDR   0x0803F800

//#define IAP_FLASH_FLAG_ADDR   0x8002800

#define INIT_FLAG_DATA      0xFFFF   //默认标志的数据(空片子的情况)
#define UPDATE_FLAG_DATA        0xEEEE   //下载标志的数据
#define UPLOAD_FLAG_DATA        0xDDDD   //上传标志的数据
#define ERASE_FLAG_DATA         0xCCCC   //擦除标志的数据
#define APPRUN_FLAG_DATA        0x5A5A   //APP不需要做任何处理，直接运行状态
#define SET_DeviceID_FLAG_DATA  0xEDED  //写设备标号
#define GET_DeviceID_FLAG_DATA  0xDEDE  //读设备标号

/* Define the Flsah area size ---------------------------------*/
#define PAGE_SIZE                         (0x800)    /* 2 Kbytes */
#define FLASH_SIZE                        (0x40000)  /* 256 KBytes */
/* Compute the FLASH upload image size --------------------------*/
#define FLASH_IMAGE_SIZE      (uint32_t) (FLASH_SIZE - (ApplicationAddress - 0x08000000))

/* The maximum length of the command string -------------------*/
#define CMD_STRING_SIZE       128

#endif
