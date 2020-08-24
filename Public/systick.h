/*
 * systick.h
 *
 *  Created on: 2019Äê8ÔÂ13ÈÕ
 *      Author: hw076
 */

#ifndef SYSTICK_H_
#define SYSTICK_H_

#include "stm32f10x.h"

void SysTick_Init(uint8_t SYSCLK);
void delay_ms(uint16_t nms);
void delay_us(uint32_t nus);

#endif /* SYSTICK_H_ */
