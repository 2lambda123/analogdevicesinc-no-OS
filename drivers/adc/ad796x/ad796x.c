/***************************************************************************//**
 *   @file   ad796x.c
 *   @brief  Implementation of AD796X Driver.
 *   @author Axel Haslam (ahaslam@baylibre.com)
********************************************************************************
 * Copyright 2024(c) Analog Devices, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Analog Devices, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *  - The use of this software may or may not infringe the patent rights
 *    of one or more patent holders.  This license does not release you
 *    from the requirement that you obtain separate licenses from these
 *    patent holders to use this software.
 *  - Use of the software either in source or binary form, must be run
 *    on or directly connected to an Analog Devices Inc. component.
 *
 * THIS SOFTWARE IS PROVIDED BY ANALOG DEVICES "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, NON-INFRINGEMENT,
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL ANALOG DEVICES BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, INTELLECTUAL PROPERTY RIGHTS, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/******************************************************************************/
/***************************** Include Files **********************************/
/******************************************************************************/
#include <stdlib.h>
#include <errno.h>
#include "ad796x.h"
#include "no_os_alloc.h"
#include "no_os_print_log.h"
/******************************************************************************/
/************************ Functions Definitions *******************************/
/******************************************************************************/

#define AD796X_BYTES_PER_SAMPLE		4
struct ad796x_mode_gpios {
	uint8_t en0;
	uint8_t en1;
	uint8_t en2;
	uint8_t en3;
};

static struct ad796x_mode_gpios ad796x_modes[] = {
	[AD796X_MODE1_EXT_REF_5P0] = 		{.en3 = 1, .en2 = 0, .en1 = 0, .en0 = 1},
	[AD796X_MODE2_INT_REF_4P0] = 		{.en3 = 1, .en2 = 0, .en1 = 0, .en0 = 1},
	[AD796X_MODE3_EXT_REF_4P0] = 		{.en3 = 1, .en2 = 0, .en1 = 1, .en0 = 0},
	[AD796X_MODE4_SNOOZE] = 		{.en3 = 1, .en2 = 0, .en1 = 1, .en0 = 1},
	[AD796X_MODE5_TEST] = 			{.en3 = 0, .en2 = 1, .en1 = 0, .en0 = 0},
	[AD796X_MODE6_INVALID] = 		{.en3 = 1, .en2 = 1, .en1 = 0, .en0 = 0},
	[AD796X_MODE7_EXT_REF_5P0_9MHZ] = 	{.en3 = 1, .en2 = 1, .en1 = 0, .en0 = 1},
	[AD796X_MODE8_INT_REF_4P0_9MHZ] = 	{.en3 = 1, .en2 = 1, .en1 = 0, .en0 = 1},
	[AD796X_MODE9_EXT_REF_4P0_9MHZ] = 	{.en3 = 1, .en2 = 1, .en1 = 1, .en0 = 0},
	[AD796X_MODE10_SNOOZE2] = 		{.en3 = 1, .en2 = 1, .en1 = 1, .en0 = 1},
};

/**
 * @brief Read data samples from adc
 * @param buf - The buffer to fill.
 * @param samples - number of samples to read.
 * @return 0 in case of success, negative error code otherwise.
 */
int32_t ad400x_read_data(struct ad796x_dev *dev,
			 uint32_t *buf,
			 uint16_t samples)
{
	int ret;

	struct 	axi_dma_transfer read_transfer = {
		.size = samples * AD796X_BYTES_PER_SAMPLE,
		.transfer_done = 0,
		.cyclic = NO,
		.src_addr = 0,
		.dest_addr = (uintptr_t)buf,
	};

	ret = axi_dmac_transfer_start(dev->axi_dmac, &read_transfer);
	if (ret) {
		pr_err("axi_dmac_transfer_start() failed!\n");
		return ret;
	}

	return axi_dmac_transfer_wait_completion(dev->axi_dmac, 3000);
}

/**
 * @brief Remove the device gpio resources.
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
static int ad796x_gpio_remove(struct ad796x_dev *dev)
{
	int ret, i;

	struct no_os_gpio_desc *desc[] = {
		dev->gpio_adc_en3_fmc,
		dev->gpio_adc_en2_fmc,
		dev->gpio_adc_en1_fmc,
		dev->gpio_adc_en0_fmc,
	};

	for (i = 0; i < 4; i++) {
		if (!desc[i])
			continue;

		ret = no_os_gpio_remove(desc[i]);
		if (ret)
			return ret;
	}

	return 0;
}

/**
 * @brief Initialize the device gpios.
 * @param device - The device structure.
 * @param init_param - The structure that contains the device initial
 * 		       parameters.
 * @return 0 in case of success, negative error code otherwise.
 */
static int ad796x_gpio_init(struct ad796x_dev *dev,
			    struct ad796x_init_param *init_param)
{
	int ret, i;

	struct no_os_gpio_init_param *params[] = {
		init_param->gpio_adc_en3_fmc_ip,
		init_param->gpio_adc_en2_fmc_ip,
		init_param->gpio_adc_en1_fmc_ip,
		init_param->gpio_adc_en0_fmc_ip,
	};

	struct no_os_gpio_desc *desc[] = {
		dev->gpio_adc_en3_fmc,
		dev->gpio_adc_en2_fmc,
		dev->gpio_adc_en1_fmc,
		dev->gpio_adc_en0_fmc,
	};

	uint8_t value[] = {
		ad796x_modes[init_param->mode].en3,
		ad796x_modes[init_param->mode].en2,
		ad796x_modes[init_param->mode].en1,
		ad796x_modes[init_param->mode].en0,
	};

	for (i = 0; i < 4; i++) {
		ret = no_os_gpio_get_optional(&desc[i], params[i]);
		if (ret)
			goto error;

		if (!desc[i])
			continue;

		ret = no_os_gpio_direction_output(desc[i], value[i]);
		if (ret)
			goto error;
	}

	return 0;
error:
	ad796x_gpio_remove(dev);
	return ret;
}

/**
 * @brief Initialize the device.
 * @param device - The device structure.
 * @param init_param - The structure that contains the device initial
 * 		       parameters.
 * @return 0 in case of success, negative error code otherwise.
 */
int ad796x_init(struct ad796x_dev **device,
		struct ad796x_init_param *init_param)
{
	struct ad796x_dev *dev;
	int ret;

	dev = (struct ad796x_dev *)no_os_calloc(1, sizeof(*dev));
	if (!dev)
		return -ENOMEM;

	ret = ad796x_gpio_init(dev, init_param);
	if (ret)
		goto error_gpio;

	ret = axi_clkgen_init(&dev->clkgen, init_param->clkgen_ip);
	if (ret) {
		pr_err("error: axi_clkgen_init error: %d\n", ret);
		goto err_clkgen;
	}

	ret = axi_clkgen_set_rate(dev->clkgen, 125000000);
	if (ret) {
		pr_err("error: axi_clkgen_set_rate error: %d\n", ret);
		goto err_set_rate;
	}

	ret = no_os_pwm_init(&dev->axi_pwm_0, init_param->axi_pwm_0_ip);
	if (ret) {
		pr_err("error: no_os_pwm_init error: %d\n", ret);
		goto err_pwm_0;
	}

	ret = no_os_pwm_init(&dev->axi_pwm_1, init_param->axi_pwm_1_ip);
	if (ret) {
		pr_err("error: no_os_pwm_init error: %d\n", ret);
		goto err_pwm_1;
	}

	ret = axi_adc_init(&dev->ad796x_core, init_param->ad796x_core_ip);
	if (ret) {
		pr_err("axi_adc_init error: %d\n", ret);
		goto err_adc;
	}

	ret = axi_dmac_init(&dev->axi_dmac, init_param->dmac_ip);
	if (ret) {
		pr_err("axi_dmac_init error: %d\n", ret);
		goto err_dmac;
	}

	*device = dev;

	return 0;

err_dmac:
	axi_adc_remove(dev->ad796x_core);
err_adc:
	no_os_pwm_remove(dev->axi_pwm_1);
err_pwm_1:
	no_os_pwm_remove(dev->axi_pwm_0);
err_pwm_0:
err_set_rate:
	axi_clkgen_remove(dev->clkgen);
err_clkgen:
	ad796x_gpio_remove(dev);
error_gpio:
	no_os_free(dev);
	return ret;
}

/**
 * @brief Remove the device and release resources.
 * @param dev - The device structure.
 * @return 0 in case of success, negative error code otherwise.
 */
int ad796x_remove(struct ad796x_dev *dev)
{
	int ret;

	ret = axi_adc_remove(dev->ad796x_core);
	if (ret)
		return ret;

	ret = no_os_pwm_remove(dev->axi_pwm_1);
	if (ret)
		return ret;

	ret = no_os_pwm_remove(dev->axi_pwm_0);
	if (ret)
		return ret;

	ret = axi_clkgen_remove(dev->clkgen);
	if (ret)
		return ret;

	ret = ad796x_gpio_remove(dev);
	if (ret)
		return ret;

	ret = ad796x_gpio_remove(dev);
	if (ret)
		return ret;

	no_os_free(dev);

	return ret;
}

