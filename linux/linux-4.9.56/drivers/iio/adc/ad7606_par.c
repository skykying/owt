/*
 * AD7606 Parallel Interface ADC driver
 *
 * Copyright 2011 Analog Devices Inc.
 *
 * Licensed under the GPL-2.
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/io.h>

#include <linux/iio/iio.h>
#include "ad7606.h"

#ifdef CONFIG_SUNXI_AD7606_SUPPORT
#include <linux/of.h> 
#include<linux/of_address.h> 
#include<linux/of_irq.h> 
#include<linux/of_gpio.h>
#endif

static int get_gpio_value_with_check(unsigned int gpio) {
	int val = 0;
	if(gpio_is_valid(gpio)){
		 val = gpio_get_value(gpio);
	}
	return val;
}

#ifdef CONFIG_SUNXI_AD7606_SUPPORT
static int read_gpio_array_value_16(struct device *dev) {
	int value = 0;
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct ad7606_state *st = iio_priv(indio_dev);
	struct ad7606_platform_data *pdata = st->pdata;

	int  db0 	= get_gpio_value_with_check(pdata->stub_db0);
	int  db1 	= get_gpio_value_with_check(pdata->stub_db1);
	int  db2 	= get_gpio_value_with_check(pdata->stub_db2);
	int  db3 	= get_gpio_value_with_check(pdata->stub_db3);
	int  db4 	= get_gpio_value_with_check(pdata->stub_db4);
	int  db5 	= get_gpio_value_with_check(pdata->stub_db5);
	int  db6 	= get_gpio_value_with_check(pdata->stub_db6);
	int  db7 	= get_gpio_value_with_check(pdata->stub_db7);
	int  db8 	= get_gpio_value_with_check(pdata->stub_db8);
	int  db9 	= get_gpio_value_with_check(pdata->stub_db9);
	int  db10 	= get_gpio_value_with_check(pdata->stub_db10);
	int  db11 	= get_gpio_value_with_check(pdata->stub_db11);
	int  db12 	= get_gpio_value_with_check(pdata->stub_db12);
	int  db13 	= get_gpio_value_with_check(pdata->stub_db13);
	int  db14 	= get_gpio_value_with_check(pdata->stub_db14);
	int  db15 	= get_gpio_value_with_check(pdata->stub_db15);

	value = (db0 << 0) | 
			(db1 << 1) |
			(db2 << 2) |
			(db3 << 3) |
			(db4 << 4) |
			(db5 << 5) |
			(db6 << 6) |
			(db7 << 7) |
			(db8 << 8) |
			(db9 << 9) |
			(db10 << 10) |
			(db11 << 11) |
			(db12 << 12) |
			(db13 << 13) |
			(db14 << 14) |
			(db15 << 15) ;

	dev_err(dev,"read_gpio_array_value value=%d",value);
	return value;
}
#endif

static int ad7606_par16_read_block(struct device *dev, int count, void *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct ad7606_state *st = iio_priv(indio_dev);

#ifdef CONFIG_SUNXI_AD7606_SUPPORT
	int i = 0;
	memset(buf,0,count);

	for(i = 0; i< count; i++){
		if(gpio_is_valid(st->pdata->gpio_rd)){
			gpio_set_value(st->pdata->gpio_rd, 0);
			read_gpio_array_value_16(dev);
			gpio_set_value(st->pdata->gpio_rd, 1);
		}
	}

#else
	if(st->base_address != NULL){
		insw((unsigned long)st->base_address, buf, count);
	}
#endif

	return 0;
}

static struct ad7606_bus_ops ad7606_par16_bops = {
	.read_block	= ad7606_par16_read_block,
};

#ifdef CONFIG_SUNXI_AD7606_SUPPORT
static int  read_gpio_array_value_8(struct device *dev) {
	int value = 0;
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct ad7606_state *st = iio_priv(indio_dev);
	struct ad7606_platform_data *pdata = st->pdata;

	int  db0 	= get_gpio_value_with_check(pdata->stub_db0);
	int  db1 	= get_gpio_value_with_check(pdata->stub_db1);
	int  db2 	= get_gpio_value_with_check(pdata->stub_db2);
	int  db3 	= get_gpio_value_with_check(pdata->stub_db3);
	int  db4 	= get_gpio_value_with_check(pdata->stub_db4);
	int  db5 	= get_gpio_value_with_check(pdata->stub_db5);
	int  db6 	= get_gpio_value_with_check(pdata->stub_db6);
	int  db7 	= get_gpio_value_with_check(pdata->stub_db7);


	value = (db0 << 0) | 
			(db1 << 1) |
			(db2 << 2) |
			(db3 << 3) |
			(db4 << 4) |
			(db5 << 5) |
			(db6 << 6) |
			(db7 << 7) ;

	dev_err(dev,"read_gpio_array_value_8 value=%d",value);
	return value;
}
#endif

static int ad7606_par8_read_block(struct device *dev, int count, void *buf)
{
	struct iio_dev *indio_dev = dev_get_drvdata(dev);
	struct ad7606_state *st = iio_priv(indio_dev);

#ifdef CONFIG_SUNXI_AD7606_SUPPORT
	int i = 0;
	memset(buf,0,count);

	for(i = 0; i < count*2; i++){
		if(gpio_is_valid(st->pdata->gpio_rd)){
			gpio_set_value(st->pdata->gpio_rd, 0);
			read_gpio_array_value_8(dev);
			gpio_set_value(st->pdata->gpio_rd, 1);
		}
	}
#else
	if(st->base_address != NULL){
		insb((unsigned long)st->base_address, buf, count * 2);
	}
#endif

	return 0;
}

static struct ad7606_bus_ops ad7606_par8_bops = {
	.read_block	= ad7606_par8_read_block,
};

static int ad7606_par_probe(struct platform_device *pdev)
{
	struct iio_dev *indio_dev;
	void __iomem *addr = NULL;

	int irq = -1;

	int channels;
	int bits;
	enum ad7606_supported_device_ids id;
	struct device *dev = &pdev->dev;

#ifdef CONFIG_SUNXI_AD7606_SUPPORT
	struct device_node *np = dev->of_node;
	struct ad7606_bus_ops *ops;
#else
	resource_size_t remap_size;
	struct resource *res;
#endif

	id = platform_get_device_id(pdev)->driver_data;

#ifdef CONFIG_SUNXI_AD7606_SUPPORT
	if(of_property_read_u32(np, "channels", &channels)){
		dev_err(&pdev->dev,"you must have channels in property \n");
	}
	dev_err(&pdev->dev,"get %d channels of ad7606 from property \n", channels);
	
	if(channels == 4){
		id = ID_AD7606_4;
	}else if(channels == 6){
		id = ID_AD7606_6;
	}else{
		id = ID_AD7606_8;
	}

	if(of_property_read_u32(np, "bits", &bits)){
		dev_err(&pdev->dev,"you must have data bits in property \n");
	}
	dev_err(&pdev->dev,"get %d bits data of ad7606 from property \n", bits);
	if(bits == 16){
		ops = &ad7606_par16_bops;
	}else{
		ops = &ad7606_par8_bops;
	}

	indio_dev = ad7606_probe(&pdev->dev, irq, addr, id, ops);
	if (IS_ERR(indio_dev)){
		return PTR_ERR(indio_dev);
	}
#else
	dev_err(&pdev->dev,"ad7606_par_probe\n");
	irq = platform_get_irq(pdev, 0);
	if (irq < 0) {
		dev_err(&pdev->dev, "no platform irq\n");
		//return -ENODEV;
	}
	dev_err(&pdev->dev,"ad7606 par get resource\n");

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	addr = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(addr)){
		dev_err(&pdev->dev,"ad7606 par get iomem error\n");
		return PTR_ERR(addr);
	}
	remap_size = resource_size(res);


	indio_dev = ad7606_probe(&pdev->dev, irq, addr,
				 id,
				 remap_size > 1 ? &ad7606_par16_bops :
				 &ad7606_par8_bops);

	if (IS_ERR(indio_dev)){
		return PTR_ERR(indio_dev);
	}
#endif

	dev_err(&pdev->dev,"ad7606 par platform_set_drvdata\n");
	platform_set_drvdata(pdev, indio_dev);

	return 0;
}

static int ad7606_par_remove(struct platform_device *pdev)
{
	struct iio_dev *indio_dev = platform_get_drvdata(pdev);

	ad7606_remove(indio_dev, platform_get_irq(pdev, 0));

	return 0;
}

#ifndef CONFIG_SUNXI_AD7606_SUPPORT
static const struct platform_device_id ad7606_driver_ids[] = {
	{
		.name		= "ad7606-8",
		.driver_data	= ID_AD7606_8,
	}, {
		.name		= "ad7606-6",
		.driver_data	= ID_AD7606_6,
	}, {
		.name		= "ad7606-4",
		.driver_data	= ID_AD7606_4,
	},
	{ }
};

MODULE_DEVICE_TABLE(platform, ad7606_driver_ids);
#endif

#ifdef CONFIG_SUNXI_AD7606_SUPPORT
static const struct of_device_id ad7606_of_match[] = {
	{.compatible = "allwinner,sunxi-ad7606-par",},
	{.compatible = "sunxi,sunxi-ad7606-par",},
	{},
};
MODULE_DEVICE_TABLE(of,ad7606_of_match);
#endif


static struct platform_driver ad7606_driver = {
	.probe = ad7606_par_probe,
	.remove	= ad7606_par_remove,
#ifndef CONFIG_SUNXI_AD7606_SUPPORT
	.id_table = ad7606_driver_ids,
#endif
	.driver = {
		.name	 = "ad7606",
		.pm	 = AD7606_PM_OPS,
#ifdef CONFIG_SUNXI_AD7606_SUPPORT
		.of_match_table = of_match_ptr(ad7606_of_match),
#endif
	},
};

module_platform_driver(ad7606_driver);

MODULE_AUTHOR("Michael Hennerich <hennerich@blackfin.uclinux.org>");
MODULE_DESCRIPTION("Analog Devices AD7606 ADC");
MODULE_LICENSE("GPL v2");


/**

DTS {
	
	sunxi_ad7606: sunxi_ad7606@0x01c2087c {
			compatible = "sunxi, sunxi-ad7606","allwinner,sunxi-ad7606";
			reg = <0x0 0x01c2087c 0x0 0x100>
            #gpio-cells = <2>;
			device_type = "adc";
			gpio-stby 	= <&pio PH 7 1 1 1 1>;
			gpio-range 	= <&pio PH 7 1 1 1 1>;
			gpio-convst = <&pio PH 7 1 1 1 1>;
			gpio-reset 	= <&pio PH 7 1 1 1 1>;
			gpio-os0 	= <&pio PH 7 1 1 1 1>;
			gpio-os1 	= <&pio PH 7 1 1 1 1>;
			gpio-os2 	= <&pio PH 7 1 1 1 1>;
			gpio-frstdata = <&pio PH 7 1 1 1 1>;
			gpio-busy 	= <&pio PH 7 1 1 1 1>;
			gpio-cs 	= <&pio PH 7 1 1 1 1>;
			gpio-rd 	= <&pio PH 7 1 1 1 1>;

	}
}

insmod /vendor/modules/ad7606_par.ko
rmmod /vendor/modules/ad7606_par.ko

echo 0 > /sys/devices/iio_sysfs_trigger/add_trigger
cat /sys/bus/iio/devices/trigger0/name> /sys/bus/iio/devices/iio:device0/trigger/current_trigger
echo 1 > /sys/bus/iio/devices/iio:device0/scan_elements/in_voltage4_en
echo 1 > /sys/bus/iio/devices/iio:device0/scan_elements/in_voltage5_en
echo 1 > /sys/bus/iio/devices/iio:device0/scan_elements/in_voltage6_en
echo 1 > /sys/bus/iio/devices/iio:device0/scan_elements/in_voltage7_en
echo 100 > /sys/bus/iio/devices/iio:device0/buffer/length
echo 1 > /sys/bus/iio/devices/iio:device0/buffer/enable
echo 1 > /sys/bus/iio/devices/trigger0/trigger_now

echo 0 > /sys/bus/iio/devices/iio:device0/buffer/enable 
echo "" > /sys/bus/iio/devices/iio:device0/trigger/current_trigger
cat /dev/iio\:device0 | xxd – 

mkdir /config/iio/triggers/hrtimer/trigger0
echo 50 > /sys/bus/iio/devices/trigger0/sampling_frequency

echo 1 > /sys/bus/iio/devices/iio:device0/scan_elements/in_voltage4_en
echo 1 > /sys/bus/iio/devices/iio:device0/scan_elements/in_voltage5_en
echo 1 > /sys/bus/iio/devices/iio:device0/scan_elements/in_voltage6_en
echo 1 > /sys/bus/iio/devices/iio:device0/scan_elements/in_voltage7_en
echo 1 > /sys/bus/iio/devices/iio:device0/buffer/enable
cat /dev/iio:device0 | xxd -


echo 0 > /sys/bus/iio/devices/iio:device0/buffer/enable











1  ===== 5v     =====  XX    ===== 00001
2  ===== 5v     =====  5v    ===== 2
3  ===== 3.3v 	=====  3.3v  ===== 39
4  ===== 3.3v 	=====  3.3v  ===== 40
5  ===== GND 	=====  XX    ===== 00002
6  ===== PD24 	=====  XX    ===== 00003
7  ===== PD12 	=====  DB12  ===== 27
8  ===== PD13 	=====  DB13  ===== 28
9  ===== PD14 	=====  DB14  ===== 29
10 ===== PD15 	=====  DB15  ===== 30
11 ===== PD16 	=====  os1   ===== 3
12 ===== PD17 	=====  os0   ===== 4
13 ===== GND 	=====  GND   ===== 37
14 ===== GND 	=====  GND   ===== 38
15 ===== PD6 	=====  DB6   ===== 21
16 ===== PD7 	=====  DB7   ===== 22
17 ===== PD8 	=====  DB8   ===== 23
18 ===== PD9 	=====  DB9   ===== 24
19 ===== PD10 	=====  DB10  ===== 25
20 ===== PD11 	=====  DB11  ===== 26
21 ===== GND 	=====  xx    ===== 00004
22 ===== GND 	===== xx  	 ===== 00005
23 ===== PD0 	===== DB0    ===== 15
24 ===== PD1 	===== DB1    ===== 16
25 ===== PD2 	===== DB2    ===== 17
26 ===== PD3 	===== DB3    ===== 18
27 ===== PD4 	===== DB4    ===== 19
28 ===== PD5 	===== DB5    ===== 20
29 ===== PD20 	===== rd  	 ===== 7
30 ===== PD21 	===== reset  ===== 8
31 ===== PD18 	===== convrt ===== 5
32 ===== PD19 	===== os2  	 ===== 6
33 ===== PH10 	===== frst   ===== 12
34 ===== PH11 	===== xx  	 ===== 00007
35 ===== PD23 	===== cs  	 ===== 10
36 ===== PD22 	===== XX 	 ===== 00006
37 ===== PH7 	===== busy   ===== 9
38 ===== PH8 	===== xx  	 ===== 00008
39 ===== SCK 	===== xx  	 ===== 00009
40 ===== SDA 	===== xx  	 ===== 00011

*/