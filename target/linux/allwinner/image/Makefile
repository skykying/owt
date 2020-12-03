#
# Copyright (C) 2013-2016 OpenWrt.org
# Copyright (C) 2016 Yousong Zhou
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
# 
# LINUX_DIR
include $(TOPDIR)/rules.mk

# DTS_DIR provider
include $(INCLUDE_DIR)/image.mk

# DTC, DTC_FLAGS  provider
include $(INCLUDE_DIR)/u-boot.mk

FAT32_BLOCK_SIZE=1024
FAT32_BLOCKS=$(shell echo $$(($(CONFIG_SUNXI_SD_BOOT_PARTSIZE)*1024*1024/$(FAT32_BLOCK_SIZE))))

KERNEL_LOADADDR:=0x40008000

define Build/allwinner-sdcard
	rm -f $@.boot
	mkfs.fat $@.boot -C $(FAT32_BLOCKS)

	mcopy -i $@.boot $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-boot.scr ::boot.scr
	mcopy -i $@.boot $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-sdcard.dtb ::dtb 
	mcopy -i $@.boot $(IMAGE_KERNEL) ::uImage
	./sdcard.sh $@ \
		$@.boot \
		$(IMAGE_ROOTFS) \
		$(CONFIG_SUNXI_SD_BOOT_PARTSIZE) \
		$(CONFIG_TARGET_ROOTFS_PARTSIZE) \
		$(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-u-boot-with-spl.bin
	rm -f $@.boot
endef


define Build/merge-dtb
	rm -Rf $@.dtb.dir
	mkdir -p $@.dtb.dir

	$(CP) $(DTS_DIR)/$(ALLWINNER_DIR)/$(DTC_DEP_FILE) $@.dtb.dir
	$(CP) $(DTS_DIR)/$(ALLWINNER_DIR)/$(DTC_SRC_FILE) $@.dtb.dir
	$(CP) ./fex/$(FEX_FILE) $@.dtb.dir

	rm -f $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-sdcard.dtb
	rm -f $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-sdcard.dts

	$(DTC) $(DTC_FLAGS) -O dtb -o $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-sdcard.dtb	\
		-b 0			\
		-i $@.dtb.dir	\
		-F $@.dtb.dir/$(FEX_FILE)	\
		-d $@.dtb.dir/$(DTC_DEP_FILE) \
		$@.dtb.dir/$(DTC_SRC_FILE)

	$(DTC) $(DTC_FLAGS)  -I dtb -O dts -o $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-sdcard.dts $(STAGING_DIR_IMAGE)/$(DEVICE_NAME)-sdcard.dtb

	rm -Rf $@.dtb.dir
endef

define Build/allwinner-img
	if [ -d $(STAGING_DIR_IMAGE)/sign ]; then \
		rm -rf $(STAGING_DIR_IMAGE)/sign; \
	fi 
	mkdir -p $(STAGING_DIR_IMAGE)/sign;

	if [ ! -d $(STAGING_DIR_IMAGE)/sign/.tools.dir ]; then \
		mkdir -p $(STAGING_DIR_IMAGE)/sign/.tools.dir; \
	fi

	$(CP) ./common/ $(STAGING_DIR_IMAGE)/sign/.tools.dir
	$(CP) ./tools/ $(STAGING_DIR_IMAGE)/sign/.tools.dir 
	$(CP) ./fex/ $(STAGING_DIR_IMAGE)/sign/.tools.dir 
	$(CP) ./resource/ $(STAGING_DIR_IMAGE)/sign/.tools.dir

	$(CP) $(IMAGE_KERNEL) $(STAGING_DIR_IMAGE)/Image

	export FORCE_UNSAFE_CONFIGURE=1
	$(MAKE) -C $(CONFIG_EXTERNAL_RAMDISK_TREE) $(CONFIG_EXTERNAL_RAMDISK_DEFCONFIG)
	$(MAKE) -C $(CONFIG_EXTERNAL_RAMDISK_TREE)

	$(CP) $(DTS_DIR)/$(ALLWINNER_DIR)/$(DTC_DEP_FILE) $(STAGING_DIR_IMAGE)/sign/$(DEVICE_NAME).dtc.dep
	$(CP) $(DTS_DIR)/$(ALLWINNER_DIR)/$(DTC_SRC_FILE) $(STAGING_DIR_IMAGE)/sign/$(DEVICE_NAME).dtc.src

	$(CP) $(IMAGE_ROOTFS) $(STAGING_DIR_IMAGE)/_rootfs.ext4
	
	./sign_image.sh \
		$(STAGING_DIR_IMAGE)/sign/.tools.dir \
		$(STAGING_DIR_IMAGE)/sign \
		$(STAGING_DIR_IMAGE) \
		$(DEVICE_NAME) \
		$(DTC) \
		$(DTC_FLAGS) \
		$(LINUX_DIR) \
		$(TARGET_CROSS) \
		$(CONFIG_EXTERNAL_RAMDISK_TREE) \
		$(CONFIG_EXTERNAL_RAMDISK_DEFCONFIG) \
		$@
endef

# why \x00\x00\x00\x00 for zImage-initramfs
define Device/Default
  PROFILES := Default
  DEVICE_VARS := ALLWINNER_DTS ALLWINNER_UBOOT ALLWINNER_DIR DTC_DEP_FILE DTC_SRC_FILE FEX_FILE
  KERNEL_NAME := Image
  KERNEL := kernel-bin
  IMAGES := allwinner.img.gz
  IMAGE/allwinner.img.gz := allwinner-img | gzip
endef

# why \x00\x00\x00\x00 for zImage-initramfs
define Device/allwinner-bakeup
  PROFILES := Default
  DEVICE_VARS := ALLWINNER_DTS ALLWINNER_UBOOT ALLWINNER_DIR DTC_DEP_FILE DTC_SRC_FILE FEX_FILE
  KERNEL_NAME := zImage
  KERNEL := kernel-bin | uImage none
  IMAGES := sdcard.img.gz allwinner.img.gz
  IMAGE/allwinner.img.gz := allwinner-img | gzip
  IMAGE/sdcard.img.gz := merge-dtb | allwinner-sdcard | append-metadata | gzip
endef

include cortex-a53.mk

$(eval $(call BuildImage))
