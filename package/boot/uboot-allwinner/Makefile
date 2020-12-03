#
# Copyright (C) 2013-2016 OpenWrt.org
# Copyright (C) 2017 Yousong Zhou
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_VERSION:=2019.01

PKG_HASH:=50bd7e5a466ab828914d080d5f6a432345b500e8fba1ad3b7b61e95e60d51c22

PKG_MAINTAINER:=Zoltan HERPAI <wigyori@uid0.hu>

include $(INCLUDE_DIR)/u-boot.mk
include $(INCLUDE_DIR)/package.mk

ifneq ($(strip $(CONFIG_EXTERNAL_UBOOT_TREE)),"")
	define Build/Prepare
		if [ -d $(PKG_BUILD_DIR) ]; then \
			rm -fR $(PKG_BUILD_DIR); \
		fi
		mkdir -p $(PKG_BUILD_DIR)
		rsync -a $(CONFIG_EXTERNAL_UBOOT_TREE)/ $(PKG_BUILD_DIR)
		
		if [ -d $(PKG_BUILD_DIR)/../bootloader ]; then \
                        rm -fR $(PKG_BUILD_DIR)/../bootloader; \
                fi
		mv $(PKG_BUILD_DIR)/bootloader $(PKG_BUILD_DIR)/..

		if [ -d $(PKG_BUILD_DIR)/../gcc-linaro ]; then \
                        rm -fR $(PKG_BUILD_DIR)/../gcc-linaro; \
                fi
		mv $(PKG_BUILD_DIR)/gcc-linaro $(PKG_BUILD_DIR)/..

		if [ -d $(PKG_BUILD_DIR)/../armv8_toolchain ]; then \
                        rm -fR $(PKG_BUILD_DIR)/../armv8_toolchain; \
                fi
		mv $(PKG_BUILD_DIR)/armv8_toolchain $(PKG_BUILD_DIR)/..

		mv $(PKG_BUILD_DIR)/u-boot-2014.07/* $(PKG_BUILD_DIR)
		if [ -d $(PKG_BUILD_DIR)/u-boot-2014.07 ]; then \
                        rm -fR $(PKG_BUILD_DIR)/u-boot-2014.07; \
                fi
        endef
endif

define U-Boot/Default
  SECTION:=boot
  CATEGORY:=Boot Loaders
  TITLE:=U BOOT for Allwinner
  BUILD_TARGET:=allwinner
  UBOOT_SUNXI_IMAGE_BOOT0_NAND:=boot0_nand_sun50iw1p1.bin
  UBOOT_SUNXI_IMAGE_BOOT0_MMC:=boot0_sdcard_sun50iw1p1.bin
  UBOOT_SUNXI_IMAGE_FES:=fes1_sun50iw1p1.bin
  UBOOT_SUNXI_IMAGE_SBOOT:=sboot_sun50iw1p1.bin
  UBOOT_SUNXI_IMAGE:=u-boot-sun50iw1p1.bin
  UENV:=default
endef

SUB_UBOOT_VERSION:=u-boot-2014.07

define U-Boot/sun50iw1p1
  BUILD_SUBTARGET:=cortexa53
  NAME:=tulip  A64
  BUILD_DEVICES:=sun50i-a64-tulip-plus
  UBOOT_SUNXI_IMAGE:=u-boot-sun50iw1p1.bin
  DEPENDS:=+PACKAGE_u-boot-sun50iw1p1:arm-trusted-firmware-sunxi
  UENV:=a64
endef

UBOOT_TARGETS := \
  sun50iw1p1 

UBOOT_MAKE_FLAGS += \
	BL31=$(STAGING_DIR_IMAGE)/bl31.bin

define Build/Compile
	$(MAKE) -C $(PKG_BUILD_DIR) distclean && \
	$(MAKE) -C $(PKG_BUILD_DIR) sun50iw1p1_config && \
	$(MAKE) -C $(PKG_BUILD_DIR) && \
	$(MAKE) -C $(PKG_BUILD_DIR) boot0 && \
	$(MAKE) -C $(PKG_BUILD_DIR) spl && \
	$(MAKE) -C $(PKG_BUILD_DIR) fes && \
	$(MAKE) -C $(PKG_BUILD_DIR) sboot
endef

define Build/InstallDev
	$(INSTALL_DIR) $(STAGING_DIR_IMAGE)
	$(CP) $(PKG_BUILD_DIR)/bin/$(UBOOT_SUNXI_IMAGE_BOOT0_NAND) $(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-nand.bin
	$(CP) $(PKG_BUILD_DIR)/bin/$(UBOOT_SUNXI_IMAGE_BOOT0_MMC) $(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-mmc.bin
	$(CP) $(PKG_BUILD_DIR)/bin/$(UBOOT_SUNXI_IMAGE_FES) $(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-fes.bin
	$(CP) $(PKG_BUILD_DIR)/bin/$(UBOOT_SUNXI_IMAGE_SBOOT) $(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-sboot.bin
	$(CP) $(PKG_BUILD_DIR)/bin/$(UBOOT_SUNXI_IMAGE) $(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-u-boot-with-spl.bin
	mkimage -C none -A arm -T script -d uEnv-$(UENV).txt \
		$(STAGING_DIR_IMAGE)/$(BUILD_DEVICES)-boot.scr
endef

define Package/U-Boot/install/default
endef

$(eval $(call BuildPackage/U-Boot))
