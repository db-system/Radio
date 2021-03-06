/**
 * \file
 * <!--
 * This program is free software; you can redistribute it and/or modify
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
 * Copyright 2013 Daniele Basile <asterix@develer.com>
 *
 * -->
 *
 * \brief Measure.
 *
 * \author Daniele Basile <asterix24@gmail.com>
 *
 */

#include "measure.h"

#include "hw/hw_adc.h"
#include "hw/hw_measure.h"

// Define logging setting (for cfg/log.h module).
#define LOG_LEVEL   2
#define LOG_FORMAT  0
#include <cfg/log.h>
#include <cfg/macros.h>

#include <io/stm32.h>

#include <cpu/types.h>

#include <drv/rtc.h>
#include <drv/timer.h>
#include <drv/i2c.h>
#include <drv/mpl3115a2.h>
#include <drv/rtc.h>
#include <drv/bkp_stm32.h>

#include <algo/rotating_hash.h>
#include <algo/table.h>

#include <string.h>

# if 1
static const Table t[] = {
	{ 100   ,  13721 },
	{ 200   ,  10975 },
	{ 300   ,  9485 },
	{ 400   ,  8465 },
	{ 500   ,  7689 },
	{ 600   ,  7060 },
	{ 700   ,  6529 },
	{ 800   ,  6068 },
	{ 900   ,  5658 },
	{ 1000  ,  5288 },
	{ 1100  ,  4949 },
	{ 1200  ,  4635 },
	{ 1300  ,  4341 },
	{ 1400  ,  4063 },
	{ 1500  ,  3798 },
	{ 1600  ,  3545 },
	{ 1700  ,  3301 },
	{ 1800  ,  3064 },
	{ 1900  ,  2833 },
	{ 2000  ,  2607 },
	{ 2100  ,  2385 },
	{ 2200  ,  2164 },
	{ 2300  ,  1946 },
	{ 2400  ,  1727 },
	{ 2500  ,  1507 },
	{ 2600  ,  1285 },
	{ 2700  ,  1060 },
	{ 2800  ,  830 },
	{ 2900  ,  594 },
	{ 3000  ,  349 },
	{ 3100  ,  94 },
	{ 3200  ,  -175 },
	{ 3300  ,  -461 },
	{ 3400  ,  -770 },
	{ 3500  ,  -1110 },
	{ 3600  ,  -1492 },
	{ 3700  ,  -1935 },
	{ 3800  ,  -2475 },
	{ 3900  ,  -3189 },
	{ 4000  ,  -4320 },
};
#else
static const Table t[] = {
	{ 100   ,  13721 },
	{ 300   ,  9485 },
	{ 500   ,  7689 },
	{ 700   ,  6529 },
	{ 900   ,  5658 },
	{ 1000  ,  5288 },
	{ 1100  ,  4949 },
	{ 1300  ,  4341 },
	{ 1400  ,  4063 },
	{ 1500  ,  3798 },
	{ 1600  ,  3545 },
	{ 1700  ,  3301 },
	{ 1800  ,  3064 },
	{ 1900  ,  2833 },
	{ 2000  ,  2607 },
	{ 2100  ,  2385 },
	{ 2200  ,  2164 },
	{ 2300  ,  1946 },
	{ 2400  ,  1727 },
	{ 2500  ,  1507 },
	{ 2600  ,  1285 },
	{ 2700  ,  1060 },
	{ 2800  ,  830 },
	{ 2900  ,  594 },
	{ 3000  ,  349 },
	{ 3100  ,  94 },
	{ 3200  ,  -175 },
	{ 3300  ,  -461 },
	{ 3400  ,  -770 },
	{ 3500  ,  -1110 },
	{ 3700  ,  -1935 },
	{ 3900  ,  -3189 },
	{ 4000  ,  -4320 },
};
#endif

static I2c i2c;

int measure_ntc(int ch, uint8_t *data, size_t len)
{
	ASSERT(len >= sizeof(int16_t));
	uint16_t d = adc_read(ch);

	int v = table_linearInterpolation(t, countof(t), d);
	memcpy(data, &v, sizeof(int16_t));

	return 0;
}

int measure_pressure(uint8_t *data, size_t len)
{
	(void)len;
	int32_t press;
	if (mlp3115a2_readPressure(&i2c, &press) < 0)
		return -1;

	memcpy(data, &press, sizeof(int32_t));
	return 0;
}

int measure_pressureTemp(uint8_t *data, size_t len)
{
	(void)len;
	int16_t temp = 0;
	if (mlp3115a2_readTemp(&i2c, &temp) < 0)
		return -1;

	memcpy(data, &temp, sizeof(int16_t));
	return 0;
}


void measure_enable(int cfg)
{
	MEASURE_ON();

	if (cfg & MEAS_ADC)
		adc_init();

	if (cfg & MEAS_I2C)
	{
		i2c_init(&i2c, I2C2, CONFIG_I2C_FREQ);
		mpl3115a2_init(&i2c);
	}
}

int measure_updateRot(uint8_t *data, size_t len)
{
	// Save the rot of current encoded data.
	uint16_t rot;
	rotating_init(&rot);
	rotating_update(data, len, &rot);
	LOG_INFO("Update rot in bkp[%d]\n", rot);
	bkp_write(&rot, sizeof(uint16_t));

	return 0;
}

int measure_isDataChage(uint8_t *data, size_t len)
{
	uint16_t rot;
	uint16_t prev_rot;

	bkp_read(&prev_rot, sizeof(uint16_t));

	rotating_init(&rot);
	rotating_update(data, len, &rot);
	LOG_INFO("Prev[%d] Now[%d]\n", prev_rot, rot);

	if (rot == prev_rot)
		return 0;

	bkp_write(&rot, sizeof(uint16_t));
	return 1;
}

void measure_init(void)
{
	MEASURE_INIT();
}


