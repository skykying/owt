/*
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.

 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 *
 * Copyright (C) 2006-2007 - Motorola
 * Copyright (c) 2008-2010, Code Aurora Forum. All rights reserved.
 *
 * Date         Author           Comment
 * -----------  --------------   --------------------------------
 * 2006-Apr-28  Motorola     The kernel module for running the Bluetooth(R)
 *              Sleep-Mode Protocol from the Host side
 * 2006-Sep-08  Motorola         Added workqueue for handling sleep work.
 * 2007-Jan-24  Motorola         Added mbm_handle_ioi() call to ISR.
 *
 */

#include <linux/module.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/notifier.h>
#include <linux/proc_fs.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/platform_device.h>

#include <linux/irq.h>
#include <linux/param.h>
#include <linux/bitops.h>
#include <linux/termios.h>
#include <linux/wakelock.h>
#include <linux/gpio.h>
#include <linux/sunxi-gpio.h>
#include <linux/of_gpio.h>

#include <net/bluetooth/bluetooth.h>
#include <net/bluetooth/hci_core.h>
#include <linux/serial_core.h>
#include "hci_uart.h"

#include "../tty/serial/sunxi-uart.h"


// #define BT_SLEEP_DBG

#ifdef BT_DBG_LPM
#undef BT_DBG_LPM
#endif

#ifdef BT_ERR_LPM
#undef BT_ERR_LPM
#endif

#ifdef BT_SLEEP_DBG
#define BT_DBG_LPM(fmt, arg...) printk(KERN_INFO "[BT_LPM] %s|%d: " fmt "\n",__func__,__LINE__, ## arg)
#else
#define BT_DBG_LPM(fmt, arg...)
#endif

#define BT_ERR_LPM(fmt, arg...) printk(KERN_INFO "[BT_LPM] %s|%d: " fmt "\n", __func__,__LINE__, ## arg)

/*
 * Defines
 */

#define VERSION		"1.2"
#define PROC_DIR	"bluetooth/sleep"

#define DEFAULT_UART_INDEX   1
#define BT_BLUEDROID_SUPPORT 1
static int bluesleep_start(void);
static void bluesleep_stop(void);

struct bluesleep_info {
	unsigned host_wake;
	unsigned ext_wake;
	unsigned host_wake_irq;
	struct wake_lock wake_lock;
	struct uart_port *uport;
	unsigned host_wake_assert:1;
	unsigned ext_wake_assert:1;
};

/* work function */
static void bluesleep_sleep_work(struct work_struct *work);

/* work queue */
DECLARE_DELAYED_WORK(sleep_workqueue, bluesleep_sleep_work);

/* Macros for handling sleep work */
#define bluesleep_rx_busy()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_tx_busy()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_rx_idle()     schedule_delayed_work(&sleep_workqueue, 0)
#define bluesleep_tx_idle()     schedule_delayed_work(&sleep_workqueue, 0)

/* 10 second timeout */
#define TX_TIMER_INTERVAL	10

/* state variable names and bit positions */
#define BT_PROTO	0x01
#define BT_TXDATA	0x02
#define BT_ASLEEP	0x04

#if BT_BLUEDROID_SUPPORT
static bool has_lpm_enabled;
#else
/* global pointer to a single hci device. */
static struct hci_dev *bluesleep_hdev;
#endif

#if BT_BLUEDROID_SUPPORT
static struct platform_device *bluesleep_uart_dev;
#endif
static struct bluesleep_info *bsi;

/* module usage */
static atomic_t open_count = ATOMIC_INIT(1);

/*
 * Local function prototypes
 */

#if !BT_BLUEDROID_SUPPORT
static int bluesleep_hci_event(struct notifier_block *this,
				unsigned long event, void *data);
#endif

/*
 * Global variables
 */

/** Global state flags */
static unsigned long flags;

/** Tasklet to respond to change in hostwake line */
static struct tasklet_struct hostwake_task;

/** Transmission timer */
static struct timer_list tx_timer;

/** Lock for state transitions */
static spinlock_t rw_lock;

#if !BT_BLUEDROID_SUPPORT
/** Notifier block for HCI events */
struct notifier_block hci_event_nblock = {
	.notifier_call = bluesleep_hci_event,
};
#endif

struct proc_dir_entry *bluetooth_dir, *sleep_dir;
extern int enable_gpio_wakeup_src(int para);

/*
 * Local functions
 */

/*
 * bt go to sleep will call this function tell uart stop data interactive
 */
static void hsuart_power(int on)
{

	if(bsi->uport == 0){
		BT_DBG_LPM("bsi->uport is null");
	}

	if (bsi->uport->ops == 0){
		BT_DBG_LPM("bsi->uport->ops is null");
	}

	if (bsi->uport->ops->set_mctrl == 0){
		BT_DBG_LPM("bsi->uport->ops->set_mctrl is null");
	}

	if (on) {
		BT_DBG_LPM("set uart power on %d",__LINE__);
		bsi->uport->ops->set_mctrl(bsi->uport, TIOCM_RTS);
	} else {
		BT_DBG_LPM("set uart power off %d",__LINE__);
		bsi->uport->ops->set_mctrl(bsi->uport, 0);
	}
}

/**
 * @return 1 if the Host can go to sleep, 0 otherwise.
 */
static inline int bluesleep_can_sleep(void)
{
	BT_DBG_LPM("bluesleep_can_sleep %d",__LINE__);

	/* check if HOST_WAKE_BT_GPIO and BT_WAKE_HOST_GPIO
	 * are both deasserted
	 */
	return (gpio_get_value(bsi->ext_wake) != bsi->ext_wake_assert) &&
		(gpio_get_value(bsi->host_wake) != bsi->host_wake_assert) &&
		(bsi->uport != NULL);
}

/*
 * after bt wakeup should clean BT_ASLEEP flag and start time.
 */
void bluesleep_sleep_wakeup(void)
{
	BT_DBG_LPM("begin ");
	if (test_bit(BT_ASLEEP, &flags)) {
		BT_DBG_LPM("waking up...");
		/* Start the timer */
		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
		gpio_set_value(bsi->ext_wake, bsi->ext_wake_assert);
		clear_bit(BT_ASLEEP, &flags);
		/*Activating UART */
		hsuart_power(1);
	} else {
		BT_DBG_LPM("working flags is %d ",flags);
	}
	BT_DBG_LPM("end ");
}

/**
 * @brief@  main sleep work handling function which update the flags
 * and activate and deactivate UART ,check FIFO.
 */
static void bluesleep_sleep_work(struct work_struct *work)
{
	BT_DBG_LPM("begin");
	if (bluesleep_can_sleep()) {
		/* already asleep, this is an error case */
		if (test_bit(BT_ASLEEP, &flags)) {
			BT_DBG_LPM("already asleep flags is %d @ %d",flags,__LINE__);
			return;
		}
		if(bsi->uport == 0){
			BT_DBG_LPM("bsi->uport is null ");
			return;
		}
		if (bsi->uport->ops->tx_empty(bsi->uport)) {
			BT_DBG_LPM("going to sleep... flags is %d @ %d",flags,__LINE__);
			set_bit(BT_ASLEEP, &flags);
			/*Deactivating UART */
			hsuart_power(0);
			wake_lock_timeout(&bsi->wake_lock, HZ / 2);
		} else {
			mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
			return;
		}
	} else if ((gpio_get_value(bsi->ext_wake) != bsi->ext_wake_assert) && !test_bit(BT_ASLEEP, &flags)) {
		BT_DBG_LPM("get gpio value %d",__LINE__);
		gpio_set_value(bsi->ext_wake, bsi->ext_wake_assert);
		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL * HZ));
	} else {
		bluesleep_sleep_wakeup();
		BT_DBG_LPM("bluesleep wakeup %d",__LINE__);
	}
	BT_DBG_LPM("end");
}

/**
 * A tasklet function that runs in tasklet context and reads the value
 * of the HOST_WAKE GPIO pin and further defer the work.
 * @param data Not used.
 */
static void bluesleep_hostwake_task(unsigned long data)
{
	BT_DBG_LPM("hostwake line change");
	spin_lock(&rw_lock);

	if (gpio_get_value(bsi->host_wake) == bsi->host_wake_assert){
		bluesleep_rx_busy();
		BT_DBG_LPM("bluesleep rx busy %d",__LINE__);
	} else {
		bluesleep_rx_idle();
		BT_DBG_LPM("bluesleep rx idle %d",__LINE__);
	}

	spin_unlock(&rw_lock);
}

/**
 * Handles proper timer action when outgoing data is delivered to the
 * HCI line discipline. Sets BT_TXDATA.
 */
static void bluesleep_outgoing_data(void)
{
	unsigned long irq_flags;

	BT_DBG_LPM("begin");

	spin_lock_irqsave(&rw_lock, irq_flags);

	BT_DBG_LPM("before flags is %d ",flags);
	/* log data passing by */
	set_bit(BT_TXDATA, &flags);
	BT_DBG_LPM("after flags is %d ",flags);

	/* if the tx side is sleeping... */
	if (gpio_get_value(bsi->ext_wake) != bsi->ext_wake_assert) {
		BT_DBG_LPM("tx was sleeping, wakeup it");
		bluesleep_sleep_wakeup();
	}

	spin_unlock_irqrestore(&rw_lock, irq_flags);
	BT_DBG_LPM("end");
}

#if BT_BLUEDROID_SUPPORT
static struct uart_port *bluesleep_get_uart_port(void)
{
	struct uart_port *uport = NULL;

	BT_DBG_LPM("begin ");
	if (bluesleep_uart_dev) {
		BT_DBG_LPM("step 1 ");
		uport = platform_get_drvdata(bluesleep_uart_dev);
		if (uport) {
			BT_DBG_LPM(
			"%s get uart_port from blusleep_uart_dev: %s, port irq: %d",
			__func__, bluesleep_uart_dev->name, uport->irq);
		}else{
			BT_DBG_LPM("get platform_device driver data failed ");
		}
	}
	return uport;
}

static int bluesleep_lpm_proc_show(struct seq_file *m, void *v)
{
	seq_printf(m, "lpm enable: %d\n", has_lpm_enabled);
	return 0;
}

static int bluesleep_lpm_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, bluesleep_lpm_proc_show, NULL);
}

static ssize_t bluesleep_write_proc_lpm(struct file *file,
				const char __user *buffer,
				size_t count, loff_t *pos)
{
	char b;

	if (count < 1){
		BT_DBG_LPM(" count is %d ", count);
		return -EINVAL;
	}

	if (copy_from_user(&b, buffer, 1)){
		BT_DBG_LPM(" copy from user space error ");
		return -EFAULT;
	}

	BT_DBG_LPM(" copy from user space count is %d ", count);
	BT_DBG_LPM(" copy from user space b is %c ", b);

	if (b == '0') {
		BT_DBG_LPM(" bluesleep stop ");

		/* HCI_DEV_UNREG */
		bluesleep_stop();
		has_lpm_enabled = false;
		bsi->uport = NULL;
	} else {
		/* HCI_DEV_REG */
		if (!has_lpm_enabled) {

			has_lpm_enabled = true;
			if (bluesleep_uart_dev) {
				bsi->uport = bluesleep_get_uart_port();
			}

			/* if bluetooth started, start bluesleep*/
			BT_DBG_LPM("bluesleep_start ");
			bluesleep_start();
		}
	}

	return count;
}

static int bluesleep_btwrite_proc_show(struct seq_file *m, void *v)
{
	seq_puts(m, "it's not support\n");
	return 0;
}

static int bluesleep_btwrite_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, bluesleep_btwrite_proc_show, NULL);
}

static ssize_t bluesleep_write_proc_btwrite(struct file *file,
				const char __user *buffer,
				size_t count, loff_t *pos)
{
	char b;

	if (count < 1){
		BT_DBG_LPM(" count is %d", count);
		return -EINVAL;
	}

	if (copy_from_user(&b, buffer, 1)){
		BT_DBG_LPM("copy value from user space");
		return -EFAULT;
	}

	BT_DBG_LPM("user space data is %c",b);
	BT_DBG_LPM("user space count is %d",count);

	/* HCI_DEV_WRITE */
	if (b != '0'){
		bluesleep_outgoing_data();
	}

	return count;
}

static const struct file_operations lpm_fops = {
	.owner		= THIS_MODULE,
	.open		= bluesleep_lpm_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= bluesleep_write_proc_lpm,
};
static const struct file_operations btwrite_fops = {
	.owner		= THIS_MODULE,
	.open		= bluesleep_btwrite_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
	.write		= bluesleep_write_proc_btwrite,
};
#else
/**
 * Handles HCI device events.
 * @param this Not used.
 * @param event The event that occurred.
 * @param data The HCI device associated with the event.
 * @return <code>NOTIFY_DONE</code>.
 */
static int bluesleep_hci_event(struct notifier_block *this,
				unsigned long event, void *data)
{
	struct hci_dev *hdev = (struct hci_dev *) data;
	struct hci_uart *hu;
	struct uart_state *state;

	if (!hdev)
		return NOTIFY_DONE;

	switch (event) {
	case HCI_DEV_REG:
		if (!bluesleep_hdev) {
			bluesleep_hdev = hdev;
			hu  = (struct hci_uart *) hdev->driver_data;
			state = (struct uart_state *) hu->tty->driver_data;
			bsi->uport = state->uart_port;
		}
		break;
	case HCI_DEV_UNREG:
		bluesleep_hdev = NULL;
		bsi->uport = NULL;
		break;
	case HCI_DEV_WRITE:
		bluesleep_outgoing_data();
		break;
	}

	return NOTIFY_DONE;
}
#endif

/**
 * Handles transmission timer expiration.
 * @param data Not used.
 */
static void bluesleep_tx_timer_expire(unsigned long data)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&rw_lock, irq_flags);

	BT_DBG_LPM("Tx timer expired");

	/* were we silent during the last timeout */
	if (!test_bit(BT_TXDATA, &flags)) {
		BT_DBG_LPM("Tx has been idle");
		gpio_set_value(bsi->ext_wake, !bsi->ext_wake_assert);
		bluesleep_tx_idle();
	} else {
		BT_DBG_LPM("Tx data during last period");
		mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL*HZ));
	}

	/* clear the incoming data flag */
	clear_bit(BT_TXDATA, &flags);

	spin_unlock_irqrestore(&rw_lock, irq_flags);
}

/**
 * Schedules a tasklet to run when receiving an interrupt on the
 * <code>HOST_WAKE</code> GPIO pin.
 * @param irq Not used.
 * @param dev_id Not used.
 */
static irqreturn_t bluesleep_hostwake_isr(int irq, void *dev_id)
{
	BT_DBG_LPM(" ISR  ");
	/* schedule a tasklet to handle the change in the host wake line */
	tasklet_schedule(&hostwake_task);
	wake_lock(&bsi->wake_lock);
	return IRQ_HANDLED;
}

/**
 * Starts the Sleep-Mode Protocol on the Host.
 * @return On success, 0. On error, -1, and <code>errno</code> is set
 * appropriately.
 */
static int bluesleep_start(void)
{
	int retval;
	unsigned long irq_flags;

	spin_lock_irqsave(&rw_lock, irq_flags);

	BT_DBG_LPM("begin");

	if (test_bit(BT_PROTO, &flags)) {
		spin_unlock_irqrestore(&rw_lock, irq_flags);
		return 0;
	}

	spin_unlock_irqrestore(&rw_lock, irq_flags);

	if (!atomic_dec_and_test(&open_count)) {
		atomic_inc(&open_count);
		return -EBUSY;
	}

	BT_DBG_LPM("timer start");
	/* start the timer */
	mod_timer(&tx_timer, jiffies + (TX_TIMER_INTERVAL*HZ));

	/* assert BT_WAKE */
	gpio_set_value(bsi->ext_wake, bsi->ext_wake_assert);
	BT_DBG_LPM("gpio set value  %d",__LINE__);

	retval = request_irq(bsi->host_wake_irq, bluesleep_hostwake_isr,
				IRQF_TRIGGER_FALLING |
				IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND,
				"bluetooth hostwake", NULL);
	if (retval  < 0) {
		BT_ERR_LPM("Couldn't acquire BT_HOST_WAKE IRQ");
		goto fail;
	}

	retval = enable_irq_wake(bsi->host_wake_irq);
	if (retval < 0) {
		BT_ERR_LPM("Couldn't enable BT_HOST_WAKE as wakeup interrupt");
		free_irq(bsi->host_wake_irq, NULL);
		goto fail;
	}

	set_bit(BT_PROTO, &flags);

	BT_DBG_LPM("set flag bit %d",flags);

	wake_lock(&bsi->wake_lock);

	return 0;
fail:
	del_timer(&tx_timer);
	atomic_inc(&open_count);

	return retval;
}

/**
 * Stops the Sleep-Mode Protocol on the Host.
 */
static void bluesleep_stop(void)
{
	unsigned long irq_flags;

	spin_lock_irqsave(&rw_lock, irq_flags);

	if (!test_bit(BT_PROTO, &flags)) {
		spin_unlock_irqrestore(&rw_lock, irq_flags);
		return;
	}

	/* assert BT_WAKE */
	BT_DBG_LPM("gpio get value %d",__LINE__);
	gpio_set_value(bsi->ext_wake, bsi->ext_wake_assert);

	BT_DBG_LPM("delete timer %d",__LINE__);
	del_timer(&tx_timer);

	BT_DBG_LPM("clear flag bits %d",__LINE__);
	clear_bit(BT_PROTO, &flags);

	if (test_bit(BT_ASLEEP, &flags)) {
		clear_bit(BT_ASLEEP, &flags);
		hsuart_power(1);
		BT_DBG_LPM("set uart power on %d",__LINE__);
	}

	atomic_inc(&open_count);

	spin_unlock_irqrestore(&rw_lock, irq_flags);
	if (disable_irq_wake(bsi->host_wake_irq))
		BT_ERR_LPM("Couldn't disable hostwake IRQ wakeup mode\n");
	free_irq(bsi->host_wake_irq, NULL);
	wake_lock_timeout(&bsi->wake_lock, HZ / 2);
}
#if 0
/**
 * Read the <code>BT_WAKE</code> GPIO pin value via the proc interface.
 * When this function returns, <code>page</code> will contain a 1 if the
 * pin is high, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start Not used.
 * @param offset Not used.
 * @param count Not used.
 * @param eof Whether or not there is more data to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluepower_read_proc_btwake(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	*eof = 1;
	return sprintf(page, "btwake:%u\n",
		(gpio_get_value(bsi->ext_wake) == bsi->ext_wake_assert));
}

/**
 * Write the <code>BT_WAKE</code> GPIO pin value via the proc interface.
 * @param file Not used.
 * @param buffer The buffer to read from.
 * @param count The number of bytes to be written.
 * @param data Not used.
 * @return On success, the number of bytes written. On error, -1, and
 * <code>errno</code> is set appropriately.
 */
static int bluepower_write_proc_btwake(struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	char *buf;

	if (count < 1)
		return -EINVAL;

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if (copy_from_user(buf, buffer, count)) {
		kfree(buf);
		return -EFAULT;
	}

	if (buf[0] == '0') {
		gpio_set_value(bsi->ext_wake, !bsi->ext_wake_assert);
	} else if (buf[0] == '1') {
		gpio_set_value(bsi->ext_wake, bsi->ext_wake_assert);
	} else {
		kfree(buf);
		return -EINVAL;
	}

	kfree(buf);
	return count;
}

/**
 * Read the <code>BT_HOST_WAKE</code> GPIO pin value via the proc interface.
 * When this function returns, <code>page</code> will contain a 1 if the pin
 * is high, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start Not used.
 * @param offset Not used.
 * @param count Not used.
 * @param eof Whether or not there is more data to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluepower_read_proc_hostwake(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	*eof = 1;
	return sprintf(page, "hostwake: %u\n",
		(gpio_get_value(bsi->host_wake) == bsi->host_wake_assert));
}


/**
 * Read the low-power status of the Host via the proc interface.
 * When this function returns, <code>page</code> contains a 1 if the Host
 * is asleep, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start Not used.
 * @param offset Not used.
 * @param count Not used.
 * @param eof Whether or not there is more data to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluesleep_read_proc_asleep(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	unsigned int asleep;

	asleep = test_bit(BT_ASLEEP, &flags) ? 1 : 0;
	*eof = 1;
	return sprintf(page, "asleep: %u\n", asleep);
}

/**
 * Read the low-power protocol being used by the Host via the proc interface.
 * When this function returns, <code>page</code> will contain a 1 if the Host
 * is using the Sleep Mode Protocol, 0 otherwise.
 * @param page Buffer for writing data.
 * @param start Not used.
 * @param offset Not used.
 * @param count Not used.
 * @param eof Whether or not there is more data to be read.
 * @param data Not used.
 * @return The number of bytes written.
 */
static int bluesleep_read_proc_proto(char *page, char **start, off_t offset,
					int count, int *eof, void *data)
{
	unsigned int proto;

	proto = test_bit(BT_PROTO, &flags) ? 1 : 0;
	*eof = 1;
	return sprintf(page, "proto: %u\n", proto);
}

/**
 * Modify the low-power protocol used by the Host via the proc interface.
 * @param file Not used.
 * @param buffer The buffer to read from.
 * @param count The number of bytes to be written.
 * @param data Not used.
 * @return On success, the number of bytes written. On error, -1, and
 * <code>errno</code> is set appropriately.
 */
static int bluesleep_write_proc_proto(struct file *file, const char *buffer,
					unsigned long count, void *data)
{
	char proto;

	if (count < 1)
		return -EINVAL;

	if (copy_from_user(&proto, buffer, 1))
		return -EFAULT;

	if (proto == '0')
		bluesleep_stop();
	else
		bluesleep_start();

	/* claim that we wrote everything */
	return count;
}
#endif

static int __init bluesleep_probe(struct platform_device *pdev)
{
	struct device_node *np = pdev->dev.of_node;
	struct device *dev = &pdev->dev;
	struct gpio_config config;
	int ret, uart_index;
	u32 val;

	bsi = devm_kzalloc(&pdev->dev, sizeof(struct bluesleep_info),
			GFP_KERNEL);
	if (!bsi)
		return -ENOMEM;

	bsi->host_wake = of_get_named_gpio_flags(np, "bt_hostwake",
			0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(bsi->host_wake)) {
		BT_ERR_LPM("get gpio bt_hostwake failed\n");
		return -EINVAL;
	}

	BT_DBG_LPM(
	"bt_hostwake gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",
			config.gpio,
			config.mul_sel,
			config.pull,
			config.drv_level,
			config.data);

	ret = devm_gpio_request(dev, bsi->host_wake, "bt_hostwake");
	if (ret < 0) {
		BT_ERR_LPM("can't request bt_hostwake gpio %d\n",
			bsi->host_wake);
		return ret;
	}
	ret = gpio_direction_input(bsi->host_wake);
	if (ret < 0) {
		BT_ERR_LPM("can't request input direction bt_wake gpio %d\n",
			bsi->host_wake);
		return ret;
	}

	ret = enable_gpio_wakeup_src(bsi->host_wake);
	if (ret < 0) {
		BT_ERR_LPM("can't enable wakeup src for bt_hostwake %d\n",
			bsi->host_wake);
		return ret;
	}

	bsi->ext_wake = of_get_named_gpio_flags(np, "bt_wake",
			0, (enum of_gpio_flags *)&config);
	if (!gpio_is_valid(bsi->ext_wake)) {
		BT_ERR_LPM("get gpio bt_wake failed\n");
		return -EINVAL;
	}

	BT_DBG_LPM(
	"bt_wake gpio=%d  mul-sel=%d  pull=%d  drv_level=%d  data=%d\n",
			config.gpio,
			config.mul_sel,
			config.pull,
			config.drv_level,
			config.data);

	ret = devm_gpio_request(dev, bsi->ext_wake, "bt_wake");
	if (ret < 0) {
		BT_ERR_LPM("can't request bt_wake gpio %d\n",
			bsi->ext_wake);
		return ret;
	}

	/* 1.set bt_wake as output and the level is assert, assert bt wake */
	ret = gpio_direction_output(bsi->ext_wake, bsi->ext_wake_assert);
	if (ret < 0) {
		BT_ERR_LPM("can't request output direction bt_wake gpio %d\n",
			bsi->ext_wake);
		return ret;
	}

	/* set ext_wake_assert and host_wake_assert */
	bsi->ext_wake_assert = bsi->host_wake_assert = config.data;

	/* 2.get bt_host_wake gpio irq */
	bsi->host_wake_irq = gpio_to_irq(bsi->host_wake);
	if (IS_ERR_VALUE(bsi->host_wake_irq)) {
		BT_ERR_LPM("map gpio [%d] to virq failed, errno = %d\n",
				bsi->host_wake, bsi->host_wake_irq);
		ret = -ENODEV;
		return ret;
	}

	uart_index = DEFAULT_UART_INDEX;
	if (!of_property_read_u32(np, "uart_index", &val)) {
		switch (val) {
		case 0:
		case 1:
		case 2:
			uart_index = val;
			break;
		default:
			BT_ERR_LPM("unsupported uart_index (%u)\n", val);
		}
	}
	BT_DBG_LPM("uart_index (%u)\n", uart_index);

	bluesleep_uart_dev = sw_uart_get_pdev(uart_index);
	if(bluesleep_uart_dev != 0){
		BT_DBG_LPM("bluesleep_uart_dev is initialized");
	}else{
		BT_DBG_LPM("bluesleep uart dev is null");
	}

	wake_lock_init(&bsi->wake_lock, WAKE_LOCK_SUSPEND, "bluesleep");
	return 0;
}

static int bluesleep_remove(struct platform_device *pdev)
{
	/* assert bt wake */
	gpio_set_value(bsi->ext_wake, bsi->ext_wake_assert);
	if (test_bit(BT_PROTO, &flags)) {
		if (disable_irq_wake(bsi->host_wake_irq)){
			BT_ERR_LPM("Couldn't disable hostwake IRQ wakeup mode\n");
		}

		free_irq(bsi->host_wake_irq, NULL);
		del_timer(&tx_timer);
		if (test_bit(BT_ASLEEP, &flags)){
			hsuart_power(1);
		}
	}

	wake_lock_destroy(&bsi->wake_lock);
	return 0;
}

static const struct of_device_id sunxi_btlpm_ids[] = {
	{ .compatible = "allwinner,sunxi-btlpm" },
	{ /* Sentinel */ }
};

static struct platform_driver bluesleep_driver = {
	.remove	= bluesleep_remove,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "sunxi-btlpm",
		.of_match_table	= sunxi_btlpm_ids,
	},
};

/**
 * Initializes the module.
 * @return On success, 0. On error, -1, and <code>errno</code> is set
 * appropriately.
 */
static int __init bluesleep_init(void)
{
	int retval;
	struct proc_dir_entry *ent;

	BT_DBG_LPM("BlueSleep Mode Driver Ver %s", VERSION);

	retval = platform_driver_probe(&bluesleep_driver, bluesleep_probe);
	if (retval)
		return retval;

#if !BT_BLUEDROID_SUPPORT
	bluesleep_hdev = NULL;
#endif

	bluetooth_dir = proc_mkdir("bluetooth", NULL);
	if (bluetooth_dir == NULL) {
		BT_ERR_LPM("Unable to create /proc/bluetooth directory");
		return -ENOMEM;
	}

	sleep_dir = proc_mkdir("sleep", bluetooth_dir);
	if (sleep_dir == NULL) {
		BT_ERR_LPM("Unable to create /proc/%s directory", PROC_DIR);
		return -ENOMEM;
	}
#if 0
	/* Creating read/write "btwake" entry */
	ent = create_proc_entry("btwake", 0, sleep_dir);
	if (ent == NULL) {
		BT_ERR_LPM("Unable to create /proc/%s/btwake entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}
	ent->read_proc = bluepower_read_proc_btwake;
	ent->write_proc = bluepower_write_proc_btwake;

	/* read only proc entries */
	if (create_proc_read_entry("hostwake", 0, sleep_dir,
				bluepower_read_proc_hostwake, NULL) == NULL) {
		BT_ERR_LPM("Unable to create /proc/%s/hostwake entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}

	/* read/write proc entries */
	ent = create_proc_entry("proto", 0666, sleep_dir);
	if (ent == NULL) {
		BT_ERR_LPM("Unable to create /proc/%s/proto entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}
	ent->read_proc = bluesleep_read_proc_proto;
	ent->write_proc = bluesleep_write_proc_proto;

	/* read only proc entries */
	if (create_proc_read_entry("asleep", 0,
			sleep_dir, bluesleep_read_proc_asleep, NULL) == NULL) {
		BT_ERR_LPM("Unable to create /proc/%s/asleep entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}
#endif
#if BT_BLUEDROID_SUPPORT
	/* read/write proc entries */
	ent = proc_create("lpm", 0660, sleep_dir, &lpm_fops);
	if (ent == NULL) {
		BT_ERR_LPM("Unable to create /proc/%s/lpm entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}

	ent = proc_create("btwrite", 0660, sleep_dir, &btwrite_fops);
	if (ent == NULL) {
		BT_ERR_LPM("Unable to create /proc/%s/btwrite entry", PROC_DIR);
		retval = -ENOMEM;
		goto fail;
	}
#endif

	flags = 0; /* clear all status bits */

	/* Initialize spinlock. */
	spin_lock_init(&rw_lock);

	/* Initialize timer */
	init_timer(&tx_timer);
	tx_timer.function = bluesleep_tx_timer_expire;
	tx_timer.data = 0;

	/* initialize host wake tasklet */
	tasklet_init(&hostwake_task, bluesleep_hostwake_task, 0);

#if !BT_BLUEDROID_SUPPORT
	hci_register_notifier(&hci_event_nblock);
#endif

	return 0;

fail:
	BT_DBG_LPM(" register falied ");

#if BT_BLUEDROID_SUPPORT
	remove_proc_entry("btwrite", sleep_dir);
	remove_proc_entry("lpm", sleep_dir);
#endif
#if 0
	remove_proc_entry("asleep", sleep_dir);
	remove_proc_entry("proto", sleep_dir);
	remove_proc_entry("hostwake", sleep_dir);
	remove_proc_entry("btwake", sleep_dir);
#endif
	remove_proc_entry("sleep", bluetooth_dir);
	remove_proc_entry("bluetooth", 0);
	return retval;
}

/**
 * Cleans up the module.
 */
static void __exit bluesleep_exit(void)
{
#if !BT_BLUEDROID_SUPPORT
	hci_unregister_notifier(&hci_event_nblock);
#endif
	platform_driver_unregister(&bluesleep_driver);

#if BT_BLUEDROID_SUPPORT
	remove_proc_entry("btwrite", sleep_dir);
	remove_proc_entry("lpm", sleep_dir);
#endif
#if 0
	remove_proc_entry("asleep", sleep_dir);
	remove_proc_entry("proto", sleep_dir);
	remove_proc_entry("hostwake", sleep_dir);
	remove_proc_entry("btwake", sleep_dir);
#endif
	remove_proc_entry("sleep", bluetooth_dir);
	remove_proc_entry("bluetooth", 0);
}

module_init(bluesleep_init);
module_exit(bluesleep_exit);

MODULE_DESCRIPTION("Bluetooth Sleep Mode Driver ver %s " VERSION);
#ifdef MODULE_LICENSE
MODULE_LICENSE("GPL");
#endif
