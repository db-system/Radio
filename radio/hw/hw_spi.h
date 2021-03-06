/**
 * \file
 * <!--
 * This file is part of BeRTOS.
 *
 * Bertos is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU General Public License.  This exception does not however
 * invalidate any other reasons why the executable file might be covered by
 * the GNU General Public License.
 *
 * Copyright 2008 Develer S.r.l. (http://www.develer.com/)
 * All Rights Reserved.
 * -->
 *
 * \brief Hardware macro definition.
 *
 *
 * \author Daniele Basile <asterix@develer.com>
 */

#ifndef HW_SPI_H
#define HW_SPI_H

#include <cfg/macros.h>

#include <io/stm32.h>

#include <drv/gpio_stm32.h>
#include <drv/clock_stm32.h>
#include <drv/timer.h>

#define GPIO_BASE       ((struct stm32_gpio *)GPIOA_BASE)
/**
 * SPI pin definition.
 */
#define CS       BV(4)  //PA4
#define SCK      BV(5)  //PA5
#define MOSI     BV(7)  //PA7
#define MISO     BV(6)  //PA6
#define STROBE   BV(0)  //PA0

/*\}*/

#define SS_ACTIVE()      stm32_gpioPinWrite(GPIO_BASE, CS, 0)
#define SS_INACTIVE()    stm32_gpioPinWrite(GPIO_BASE, CS, 1)


INLINE void  SPI_HW_SCK_ACTIVE(void)
{
	stm32_gpioPinConfig(GPIO_BASE, SCK, GPIO_MODE_OUT_PP, GPIO_SPEED_50MHZ);
	stm32_gpioPinWrite(GPIO_BASE, SCK, 1);
	stm32_gpioPinConfig(GPIO_BASE, SCK | MOSI | MISO, GPIO_MODE_AF_PP, GPIO_SPEED_50MHZ);
}

INLINE void SPI_HW_MOSI_LOW(void)
{
	stm32_gpioPinConfig(GPIO_BASE, MOSI, GPIO_MODE_OUT_PP, GPIO_SPEED_50MHZ);
	stm32_gpioPinWrite(GPIO_BASE, MOSI, 0);
	stm32_gpioPinConfig(GPIO_BASE, SCK | MOSI | MISO, GPIO_MODE_AF_PP, GPIO_SPEED_50MHZ);
}

INLINE bool SPI_HW_IS_MISO_HIGH(void)
{
	stm32_gpioPinConfig(GPIO_BASE, MISO, GPIO_MODE_IN_FLOATING, GPIO_SPEED_50MHZ);
	bool miso = stm32_gpioPinRead(GPIO_BASE, MISO);
	stm32_gpioPinConfig(GPIO_BASE, SCK | MOSI | MISO, GPIO_MODE_AF_PP, GPIO_SPEED_50MHZ);
	return miso;
}

#define SPI_HW_INIT() \
	do { \
		/* Enable clocking on GPIOA */	\
		RCC->APB2ENR |= RCC_APB2_GPIOA;			\
		stm32_gpioPinWrite(GPIO_BASE, CS, 1); \
		stm32_gpioPinConfig(GPIO_BASE, CS, GPIO_MODE_OUT_PP, GPIO_SPEED_50MHZ); \
		stm32_gpioPinConfig(GPIO_BASE, SCK | MOSI | MISO, GPIO_MODE_AF_PP, GPIO_SPEED_50MHZ); \
	} while(0)

#endif /* HW_SPI_H */

