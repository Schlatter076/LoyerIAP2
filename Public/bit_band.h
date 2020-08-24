/*
 * bit_band.h
 *
 *  Created on: 2019年8月13日
 *      Author: hw076
 */

#ifndef BIT_BAND_H_
#define BIT_BAND_H_

#include "stm32f10x.h"

//IO口操作宏定义
#define BITBAND(addr, bitnum) ((addr & 0xF0000000)+0x2000000+((addr &0xFFFFF)<<5)+(bitnum<<2))
#define MEM_ADDR(addr)  *((volatile unsigned long  *)(addr))
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum))
//IO口地址映射
#define GPIOA_ODR_Addr    (GPIOA_BASE+12) //0x4001080C
#define GPIOB_ODR_Addr    (GPIOB_BASE+12) //0x40010C0C
#define GPIOC_ODR_Addr    (GPIOC_BASE+12) //0x4001100C
#define GPIOD_ODR_Addr    (GPIOD_BASE+12) //0x4001140C
#define GPIOE_ODR_Addr    (GPIOE_BASE+12) //0x4001180C
#define GPIOF_ODR_Addr    (GPIOF_BASE+12) //0x40011A0C
#define GPIOG_ODR_Addr    (GPIOG_BASE+12) //0x40011E0C

#define GPIOA_IDR_Addr    (GPIOA_BASE+8) //0x40010808
#define GPIOB_IDR_Addr    (GPIOB_BASE+8) //0x40010C08
#define GPIOC_IDR_Addr    (GPIOC_BASE+8) //0x40011008
#define GPIOD_IDR_Addr    (GPIOD_BASE+8) //0x40011408
#define GPIOE_IDR_Addr    (GPIOE_BASE+8) //0x40011808
#define GPIOF_IDR_Addr    (GPIOF_BASE+8) //0x40011A08
#define GPIOG_IDR_Addr    (GPIOG_BASE+8) //0x40011E08

//IO口操作,只对单一的IO口!
//确保n的值小于16!
#define PA_out(n)   BIT_ADDR(GPIOA_ODR_Addr,n)  //输出
#define PA_in(n)    BIT_ADDR(GPIOA_IDR_Addr,n)  //输入

#define PB_out(n)   BIT_ADDR(GPIOB_ODR_Addr,n)  //输出
#define PB_in(n)    BIT_ADDR(GPIOB_IDR_Addr,n)  //输入

#define PC_out(n)   BIT_ADDR(GPIOC_ODR_Addr,n)  //输出
#define PC_in(n)    BIT_ADDR(GPIOC_IDR_Addr,n)  //输入

#define PD_out(n)   BIT_ADDR(GPIOD_ODR_Addr,n)  //输出
#define PD_in(n)    BIT_ADDR(GPIOD_IDR_Addr,n)  //输入

#define PE_out(n)   BIT_ADDR(GPIOE_ODR_Addr,n)  //输出
#define PE_in(n)    BIT_ADDR(GPIOE_IDR_Addr,n)  //输入

#define PF_out(n)   BIT_ADDR(GPIOF_ODR_Addr,n)  //输出
#define PFin(n)    BIT_ADDR(GPIOF_IDR_Addr,n)  //输入

#define PG_out(n)   BIT_ADDR(GPIOG_ODR_Addr,n)  //输出
#define PG_in(n)    BIT_ADDR(GPIOG_IDR_Addr,n)  //输入

#endif /* BIT_BAND_H_ */
