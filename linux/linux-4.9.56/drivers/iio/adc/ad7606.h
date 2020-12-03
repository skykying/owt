/*
 * AD7606 ADC driver
 *
 * Copyright 2011 Analog Devices Inc.
 *
 * Licensed under the GPL-2.
 */

#ifndef IIO_ADC_AD7606_H_
#define IIO_ADC_AD7606_H_


#define SUNXI_GPIO_DIRECTION_INPUT  0
#define SUNXI_GPIO_DIRECTION_OUTPUT 1

#define SUNXI_GPIO_LEVEL_LOW        0
#define SUNXI_GPIO_LEVEL_HIGHT      1


#ifndef _AD7606_WAIT_TIMEOUT_
#define _AD7606_WAIT_TIMEOUT_		1
#endif


/*
 * TODO: struct ad7606_platform_data needs to go into include/linux/iio
 */


/**
 * struct ad7606_platform_data - platform/board specific information
 * @default_os:		default oversampling value {0, 2, 4, 8, 16, 32, 64}
 * @default_range:	default range +/-{5000, 10000} mVolt
 * @gpio_convst:	number of gpio connected to the CONVST pin
 * @gpio_reset:		gpio connected to the RESET pin, if not used set to -1
 * @gpio_range:		gpio connected to the RANGE pin, if not used set to -1
 * @gpio_os0:		gpio connected to the OS0 pin, if not used set to -1
 * @gpio_os1:		gpio connected to the OS1 pin, if not used set to -1
 * @gpio_os2:		gpio connected to the OS2 pin, if not used set to -1
 * @gpio_frstdata:	gpio connected to the FRSTDAT pin, if not used set to -1
 * @gpio_stby:		gpio connected to the STBY pin, if not used set to -1
 */

struct ad7606_platform_data {
	unsigned int			default_os;
	unsigned int			default_range;

	unsigned int			gpio_stby;
	unsigned int			gpio_range;

	unsigned int			gpio_convst;
	unsigned int			gpio_reset;

	unsigned int			gpio_os0;
	unsigned int			gpio_os1;
	unsigned int			gpio_os2;
	unsigned int			gpio_frstdata;

	unsigned int			gpio_busy;
	unsigned int			gpio_cs;
	unsigned int			gpio_rd;

	unsigned int 			stub_db0;
	unsigned int 			stub_db1;
	unsigned int 			stub_db2;
	unsigned int 			stub_db3;
	unsigned int 			stub_db4;
	unsigned int 			stub_db5;
	unsigned int 			stub_db6;
	unsigned int 			stub_db7;
	unsigned int 			stub_db8;
	unsigned int 			stub_db9;
	unsigned int 			stub_db10;
	unsigned int 			stub_db11;
	unsigned int 			stub_db12;
	unsigned int 			stub_db13;
	unsigned int 			stub_db14;
	unsigned int 			stub_db15;
};

/**
 * struct ad7606_chip_info - chip specific information
 * @name:		identification string for chip
 * @int_vref_mv:	the internal reference voltage
 * @channels:		channel specification
 * @num_channels:	number of channels
 */

struct ad7606_chip_info {
	const char			*name;
	u16				int_vref_mv;
	const struct iio_chan_spec	*channels;
	unsigned int			num_channels;
};

/**
 * struct ad7606_state - driver instance specific data
 */

struct ad7606_state {
	struct device			*dev;
	const struct ad7606_chip_info	*chip_info;
	struct ad7606_platform_data	*pdata;
	struct regulator		*reg;
	struct work_struct		poll_work;
	wait_queue_head_t		wq_data_avail;
	const struct ad7606_bus_ops	*bops;
	unsigned int			range;
	unsigned int			oversampling;
	bool				done;
	void __iomem			*base_address;

	/*
	 * DMA (thus cache coherency maintenance) requires the
	 * transfer buffers to live in their own cache lines.
	 */

	unsigned short			data[8] ____cacheline_aligned;
};

struct ad7606_bus_ops {
	/* more methods added in future? */
	int (*read_block)(struct device *, int, void *);
};

struct iio_dev *ad7606_probe(struct device *dev, int irq,
			      void __iomem *base_address, unsigned int id,
			      const struct ad7606_bus_ops *bops);
int ad7606_remove(struct iio_dev *indio_dev, int irq);
int ad7606_reset(struct ad7606_state *st);

enum ad7606_supported_device_ids {
	ID_AD7606_8,
	ID_AD7606_6,
	ID_AD7606_4
};

int ad7606_register_ring_funcs_and_init(struct iio_dev *indio_dev);
void ad7606_ring_cleanup(struct iio_dev *indio_dev);

#ifdef CONFIG_SUNXI_AD7606_SUPPORT
void sunxi_gpio_of_parse_handle(struct device * dev, struct ad7606_platform_data* pdata);
int sunxi_ad7606_request_gpios(struct ad7606_state *st);
#endif

#ifdef CONFIG_PM_SLEEP
extern const struct dev_pm_ops ad7606_pm_ops;
#define AD7606_PM_OPS (&ad7606_pm_ops)
#else
#define AD7606_PM_OPS NULL
#endif

#endif /* IIO_ADC_AD7606_H_ */
