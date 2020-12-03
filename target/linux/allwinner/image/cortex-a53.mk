#
# Copyright (C) 2013-2016 OpenWrt.org
# Copyright (C) 2016 Yousong Zhou
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#
ifeq ($(SUBTARGET),cortexa53)

define Device/sun50i-a64-tulip-plus
  DEVICE_TITLE:=tulip Plus A64
  SUPPORTED_DEVICES:=tulip,tulip-plus
  ALLWINNER_DTS:=allwinner/sun50i-a64-pine64-plus
  ALLWINNER_DIR:=allwinner
  DTC_DEP_FILE:=.sun50i-a64-pine64-plus.dtb.d.dtc.tmp
  DTC_SRC_FILE:=.sun50i-a64-pine64-plus.dtb.dts.tmp
  FEX_FILE:=sun50i-a64-tulip-plus-4.9.fex
  KERNEL_NAME := Image
  KERNEL := kernel-bin
endef

TARGET_DEVICES += sun50i-a64-tulip-plus

define Device/sun50i-a64-tulip-baseboard
  DEVICE_TITLE:=tulip base board
  SUPPORTED_DEVICES:=tulip,baseboard
  ALLWINNER_DTS:=allwinner/sun50i-a64-pine64-plus
  ALLWINNER_DIR:=allwinner
  DTC_DEP_FILE:=.sun50i-a64-pine64-plus.dtb.d.dtc.tmp
  DTC_SRC_FILE:=.sun50i-a64-pine64-plus.dtb.dts.tmp
  FEX_FILE:=sun50i-a64-tulip-baseboard-4.9.fex
  KERNEL_NAME := Image
  KERNEL := kernel-bin
endef

TARGET_DEVICES += sun50i-a64-tulip-baseboard

endif
