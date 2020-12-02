/*
 * AD7606 SPI ADC driver
 *
 * Copyright 2011 Analog Devices Inc.
 *
 * Licensed under the GPL-2.
 */

#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/types.h>
#include <linux/err.h>

#include <linux/iio/iio.h>


#include "ad7606.h"

#ifdef CONFIG_SUNXI_AD7606_SUPPORT
#include <linux/of.h> 
#include <linux/of_address.h> 
#include <linux/of_irq.h> 
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <linux/sunxi-gpio.h>
#endif

#define MAX_SPI_FREQ_HZ		23500000	/* VDRIVE above 4.75 V */

static int ad7606_spi_read_block(struct device *dev,
				 int count, void *buf)
{
	struct spi_device *spi = to_spi_device(dev);
	int i, ret;
	unsigned short *data = buf;
	__be16 *bdata = buf;

	dev_err(dev,"ad7606_spi_read_block count = %d\n", count);
	ret = spi_read(spi, buf, count * 2);
	if (ret < 0) {
		dev_err(&spi->dev, "SPI read error\n");
		return ret;
	}

	for (i = 0; i < count; i++){
		data[i] = be16_to_cpu(bdata[i]);
	}

	return 0;
}

static const struct ad7606_bus_ops ad7606_spi_bops = {
	.read_block	= ad7606_spi_read_block,
};

static int ad7606_spi_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;

	int channels;
	enum ad7606_supported_device_ids id;
	struct device *dev = &spi->dev;
	struct device_node *np = dev->of_node;

	id = spi_get_device_id(spi)->driver_data;

#ifdef CONFIG_SUNXI_AD7606_SUPPORT
	if(of_property_read_u32(np, "channels", &channels)){
		dev_err(&spi->dev,"you must have channels in property \n");
	}
	dev_err(&spi->dev,"get %d channels of ad7606 from property \n", channels);
	
	if(channels == 4){
		id = ID_AD7606_4;
	}else if(channels == 6){
		id = ID_AD7606_6;
	}else{
		id = ID_AD7606_8;
	}
#endif

	indio_dev = ad7606_probe(&spi->dev, spi->irq, NULL, id, &ad7606_spi_bops);
	dev_err(&spi->dev,"ad7606_spi_probe\n");
	if (IS_ERR(indio_dev)){
		return PTR_ERR(indio_dev);
	}

	dev_err(&spi->dev,"spi_set_drvdata\n");
	spi_set_drvdata(spi, indio_dev);

	return 0;
}

static int ad7606_spi_remove(struct spi_device *spi)
{
	struct iio_dev *indio_dev = dev_get_drvdata(&spi->dev);
	dev_err(&spi->dev,"ad7606_spi_remove\n");

	return ad7606_remove(indio_dev, spi->irq);
}

static const struct spi_device_id ad7606_id[] = {
	{"ad7606-8", ID_AD7606_8},
	{"ad7606-6", ID_AD7606_6},
	{"ad7606-4", ID_AD7606_4},
	{}
};
MODULE_DEVICE_TABLE(spi, ad7606_id);

#ifdef CONFIG_SUNXI_AD7606_SUPPORT
static const struct of_device_id ad7606_of_match[] = {
	{.compatible = "allwinner,sunxi-ad7606-fpga",},
	{.compatible = "allwinner,sunxi-ad7606-spi-fpga",},
	{.compatible = "sunxi,sunxi-ad7606-fpga",},
	{.compatible = "sunxi,sunxi-ad7606-spi-fpga",},
	{},
};
MODULE_DEVICE_TABLE(of,ad7606_of_match);
#endif

static struct spi_driver ad7606_driver = {
	.driver = {
		.name = "ad7606",
		.pm = AD7606_PM_OPS,
		.owner = THIS_MODULE,
#ifdef CONFIG_SUNXI_AD7606_SUPPORT
		.of_match_table = of_match_ptr(ad7606_of_match),
#endif
	},
	.probe = ad7606_spi_probe,
	.remove = ad7606_spi_remove,
	.id_table = ad7606_id,
};
module_spi_driver(ad7606_driver);

MODULE_AUTHOR("Michael Hennerich <hennerich@blackfin.uclinux.org>");
MODULE_DESCRIPTION("Analog Devices AD7606 ADC");
MODULE_LICENSE("GPL v2");



/**

insmod /vendor/modules/ad7606_spi.ko
rmmod /vendor/modules/ad7606_spi.ko

DTS {
	
	sunxi_ad7606: sunxi_ad7606@0 {
			compatible = "sunxi, sunxi-ad7606-spi","allwinner,sunxi-ad7606-spi";
			device_type = "adc0";

			gpio-os1 		= <&pio PD 16 1 1 1 1>;
			gpio-os0 		= <&pio PD 17 1 1 1 1>;
			gpio-convst 	= <&pio PD 18 0 1 1 1>;
			gpio-os2 		= <&pio PD 19 1 1 1 1>;
			gpio-rd 		= <&pio PD 20 1 1 1 1>;
			gpio-reset 		= <&pio PD 21 1 1 1 1>;
			gpio-busy 		= <&pio PD 22 0 1 1 1>;
			gpio-cs 		= <&pio PD 23 1 1 1 1>;
			gpio-frstdata   = <&pio PH 7 0 1 1 1>;

	}
}

3 	- os1   	-  D16
4 	- os0 		-  D17
5 	- conv 		-  D18
6 	- os2 		-  D19
7 	- rd 		-  D20
8 	- reset 	-  D21
9 	- busy 		-  D22
10  - cs 		-  D23
12  - first 	-  H7


*/
