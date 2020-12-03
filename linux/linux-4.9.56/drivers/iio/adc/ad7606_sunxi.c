/*
 * AD7606 SPI ADC driver
 *
 * Copyright 2011 Analog Devices Inc.
 *
 * Licensed under the GPL-2.
 */

#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/module.h>

#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/iio/buffer.h>


#include <linux/of.h> 
#include <linux/of_address.h> 
#include <linux/of_irq.h> 
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/sunxi-gpio.h>


#include "ad7606.h"

#ifndef _AD7606_SUNXI_FPGA__
#define _AD7606_SUNXI_FPGA__				1
#endif

//============================================================================
static unsigned int sunxi_checkout_gpio(struct device *dev, char* name){
	unsigned int gpio = -1;
	struct gpio_config config;
	struct device_node *np = dev->of_node;

	gpio = of_get_named_gpio_flags(np, name, 0, (enum of_gpio_flags *)&config);
	if(gpio_is_valid(gpio)){
		dev_err(dev,"checkout gpio %s ok\n", name);
		dev_info(dev, "%s gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",
				name,
				config.gpio,
				config.mul_sel,
				config.pull,
				config.drv_level,
				config.data);
	}else{
		dev_err(dev,"checkout gpio %s error\n", name);
	}
	return gpio;
}


static void sunxi_get_stub_db(struct device *dev,struct ad7606_platform_data *pdata){

	dev_err(dev,"to get_stub_gpio \n");
	pdata->stub_db0   	= sunxi_checkout_gpio(dev,"stub-db0");
	pdata->stub_db1 	= sunxi_checkout_gpio(dev,"stub-db1");
	pdata->stub_db2 	= sunxi_checkout_gpio(dev,"stub-db2");
	pdata->stub_db3 	= sunxi_checkout_gpio(dev,"stub-db3");
	pdata->stub_db4 	= sunxi_checkout_gpio(dev,"stub-db4");
	pdata->stub_db5 	= sunxi_checkout_gpio(dev,"stub-db5");
	pdata->stub_db6 	= sunxi_checkout_gpio(dev,"stub-db6");
	pdata->stub_db7 	= sunxi_checkout_gpio(dev,"stub-db7");
	pdata->stub_db8 	= sunxi_checkout_gpio(dev,"stub-db8");
	pdata->stub_db9 	= sunxi_checkout_gpio(dev,"stub-db9");
	pdata->stub_db10 	= sunxi_checkout_gpio(dev,"stub-db10");
	pdata->stub_db11 	= sunxi_checkout_gpio(dev,"stub-db11");
	pdata->stub_db12 	= sunxi_checkout_gpio(dev,"stub-db12");
	pdata->stub_db13 	= sunxi_checkout_gpio(dev,"stub-db13");
	pdata->stub_db14 	= sunxi_checkout_gpio(dev,"stub-db14");
	pdata->stub_db15 	= sunxi_checkout_gpio(dev,"stub-db15");
	dev_err(dev,"end get_stub_gpio \n");
}

static void sunxi_of_parse(struct device *dev,struct ad7606_platform_data *pdata){
	pdata->gpio_frstdata  	= sunxi_checkout_gpio(dev, "gpio-frstdata");

	pdata->gpio_range 		= sunxi_checkout_gpio(dev, "gpio-range");
	pdata->gpio_stby  		= sunxi_checkout_gpio(dev, "gpio-stby");
	pdata->gpio_os0 		= sunxi_checkout_gpio(dev, "gpio-os0");
	pdata->gpio_os1  		= sunxi_checkout_gpio(dev, "gpio-os1");
	pdata->gpio_os2  		= sunxi_checkout_gpio(dev, "gpio-os2");

	pdata->gpio_convst  	= sunxi_checkout_gpio(dev, "gpio-convst");
	pdata->gpio_reset  		= sunxi_checkout_gpio(dev, "gpio-reset");
	pdata->gpio_cs  		= sunxi_checkout_gpio(dev, "gpio-cs");
	pdata->gpio_rd  		= sunxi_checkout_gpio(dev, "gpio-rd");
	pdata->gpio_busy  		= sunxi_checkout_gpio(dev, "gpio-busy");
}


void sunxi_gpio_of_parse_handle(struct device* dev, struct ad7606_platform_data* pdata){
	sunxi_of_parse(dev,pdata);
	sunxi_get_stub_db(dev,pdata);
}

//============================================================================

static int sunxi_request_gpio_one(struct device* dev, unsigned int gpio, int direction, int value, char* label) {
	int ret = 0;

	if (gpio_is_valid(gpio)) {
		ret = devm_gpio_request(dev, gpio, label);
		if (ret < 0) {
			dev_err(dev, "can't request %s gpio %d\n",label, gpio);
			return ret;
		}

		if(direction == SUNXI_GPIO_DIRECTION_OUTPUT) {

			if(value == SUNXI_GPIO_LEVEL_HIGHT){
				ret = gpio_direction_output(gpio,1);
			} else {
				ret = gpio_direction_output(gpio,0);
			}
			if (ret < 0) {
				dev_err(dev, "can't request %s output direction gpio %d\n",label, gpio);
				return ret;
			}

		}else{
			ret = gpio_direction_input(gpio);
			if (ret < 0) {
				dev_err(dev, "can't request input direction %s gpio %d\n",label, gpio);
				return ret;
			}
		}
	}

	return ret;
}

 int sunxi_ad7606_request_gpios(struct ad7606_state *st)
{
	struct device * dev = st->dev;
	struct ad7606_platform_data * pdata =  st->pdata;
	int ret = 0;

	dev_dbg(st->dev,"ad7606_request_gpios \n");

	ret = sunxi_request_gpio_one(dev,
								pdata->gpio_convst,
								SUNXI_GPIO_DIRECTION_OUTPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_CONVST");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_CONVST gpio %d\n", st->pdata->gpio_convst);
		return ret;
	}

	ret = sunxi_request_gpio_one(dev,
								pdata->gpio_os0,
								SUNXI_GPIO_DIRECTION_OUTPUT,
								(st->oversampling & 1) ? SUNXI_GPIO_LEVEL_HIGHT : SUNXI_GPIO_LEVEL_LOW,
								"AD7606_OS0");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_OS0 gpio %d\n", pdata->gpio_os0);
		return ret;
	}


	ret = sunxi_request_gpio_one(dev,
								pdata->gpio_os1,
								SUNXI_GPIO_DIRECTION_OUTPUT,
								(st->oversampling & 2) ? SUNXI_GPIO_LEVEL_HIGHT : SUNXI_GPIO_LEVEL_LOW,
								"AD7606_OS1");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_OS1 gpio %d\n", pdata->gpio_os1);
		return ret;
	}

	ret = sunxi_request_gpio_one(dev,
								pdata->gpio_os2,
								SUNXI_GPIO_DIRECTION_OUTPUT,
								(st->oversampling & 2) ? SUNXI_GPIO_LEVEL_HIGHT : SUNXI_GPIO_LEVEL_LOW,
								"AD7606_OS2");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_OS2 gpio %d\n", pdata->gpio_os2);
		return ret;
	}
	
	ret = sunxi_request_gpio_one(dev,
								pdata->gpio_reset,
								SUNXI_GPIO_DIRECTION_OUTPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_RESET");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_RESET gpio %d\n", pdata->gpio_reset);
		return ret;
	}

	ret = sunxi_request_gpio_one(dev,
								pdata->gpio_range,
								SUNXI_GPIO_DIRECTION_OUTPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_RANGE");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_RANGE gpio %d\n", pdata->gpio_range);
		return ret;
	}

	ret = sunxi_request_gpio_one(dev,
								pdata->gpio_stby,
								SUNXI_GPIO_DIRECTION_OUTPUT,
								SUNXI_GPIO_LEVEL_HIGHT,
								"AD7606_STBY");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STBY gpio %d\n", pdata->gpio_stby);
		return ret;
	}

	ret = sunxi_request_gpio_one(dev,
								pdata->gpio_frstdata,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_FRSTDATA");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_FRSTDATA gpio %d\n", pdata->gpio_frstdata);
		return ret;
	}

	ret = sunxi_request_gpio_one(dev,
								pdata->gpio_busy,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_BUSY");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_BUSY gpio %d\n", pdata->gpio_busy);
		return ret;
	}

	ret = sunxi_request_gpio_one(dev,
								pdata->stub_db0,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_STUB_DB0");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STUB_DB0 gpio %d\n", pdata->stub_db0);
		return ret;
	}

	ret = sunxi_request_gpio_one(dev,
								pdata->stub_db1,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_STUB_DB1");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STUB_DB1 gpio %d\n", pdata->stub_db1);
		return ret;
	}
	ret = sunxi_request_gpio_one(dev,
								pdata->stub_db2,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_STUB_DB2");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STUB_DB2 gpio %d\n", pdata->stub_db2);
		return ret;
	}
	ret = sunxi_request_gpio_one(dev,
								pdata->stub_db3,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_STUB_DB3");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STUB_DB3 gpio %d\n", pdata->stub_db3);
		return ret;
	}
	ret = sunxi_request_gpio_one(dev,
								pdata->stub_db4,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_STUB_DB4");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STUB_DB4 gpio %d\n", pdata->stub_db4);
		return ret;
	}
	ret = sunxi_request_gpio_one(dev,
								pdata->stub_db5,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_STUB_DB5");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STUB_DB5 gpio %d\n", pdata->stub_db5);
		return ret;
	}
	ret = sunxi_request_gpio_one(dev,
								pdata->stub_db6,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_STUB_DB6");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STUB_DB6 gpio %d\n", pdata->stub_db6);
		return ret;
	}
	ret = sunxi_request_gpio_one(dev,
								pdata->stub_db7,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_STUB_DB7");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STUB_DB7 gpio %d\n", pdata->stub_db7);
		return ret;
	}
	ret = sunxi_request_gpio_one(dev,
								pdata->stub_db8,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_STUB_DB8");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STUB_DB8 gpio %d\n", pdata->stub_db8);
		return ret;
	}
	ret = sunxi_request_gpio_one(dev,
								pdata->stub_db9,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_STUB_DB9");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STUB_DB9 gpio %d\n", pdata->stub_db9);
		return ret;
	}
	ret = sunxi_request_gpio_one(dev,
								pdata->stub_db10,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_STUB_DB10");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STUB_DB10 gpio %d\n", pdata->stub_db10);
		return ret;
	}
	ret = sunxi_request_gpio_one(dev,
								pdata->stub_db11,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_STUB_DB11");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STUB_DB11 gpio %d\n", pdata->stub_db11);
		return ret;
	}
	ret = sunxi_request_gpio_one(dev,
								pdata->stub_db12,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_STUB_DB12");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STUB_DB12 gpio %d\n", pdata->stub_db12);
		return ret;
	}
	ret = sunxi_request_gpio_one(dev,
								pdata->stub_db13,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_STUB_DB13");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STUB_DB13 gpio %d\n", pdata->stub_db13);
		return ret;
	}
	ret = sunxi_request_gpio_one(dev,
								pdata->stub_db14,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_STUB_DB14");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STUB_DB14 gpio %d\n", pdata->stub_db14);
		return ret;
	}
	ret = sunxi_request_gpio_one(dev,
								pdata->stub_db15,
								SUNXI_GPIO_DIRECTION_INPUT,
								SUNXI_GPIO_LEVEL_LOW,
								"AD7606_STUB_DB15");
	if (ret < 0) {
		dev_err(dev, "can't request AD7606_STUB_DB15 gpio %d\n", pdata->stub_db15);
		return ret;
	}


	return ret;
}
