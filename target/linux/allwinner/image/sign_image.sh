#!/usr/bin/env bash
# 
# (c) Copyright 2020
# Allwinner Technology Co., Ltd. <www.allwinnertech.com>
# James Deng <csjamesdeng@allwinnertech.com>
# 
############################ Notice #####################################
# a. Some config files priority is as follows:
#    - xxx_${platform}.{cfg|fex} > xxx.{cfg|fex}
#    - ${chip}/${board}/*.{cfg|fex} > ${chip}/default/*.{cfg|fex}
#    - ${chip}/default/*.cfg > common/imagecfg/*.cfg
#    - ${chip}/default/*.fex > common/partition/*.fex
#  e.g. sun8iw7p1/configs/perf/image_linux.cfg > sun8iw7p1/configs/default/image_linux.cfg
#       > common/imagecfg/image_linux.cfg > sun8iw7p1/configs/perf/image.cfg
#       > sun8iw7p1/configs/default/image.cfg > common/imagecfg/image.cfg
#
# b. Support Nor storages rule:
#    - Need to create sys_partition_nor.fex or sys_partition_nor_${platform}.fex
#    - Add "{filename = "full_img.fex",     maintype = "12345678", \
#      subtype = "FULLIMG_00000000",}" to image[_${platform}].cfg
#
# c. Switch uart port
#    - Need to add your chip configs into ${RESOURCE_DIR}/tools/card_debug_pin
#    - Call pack with 'debug' parameters

set -x
[ $# -eq 11 ] || {
    echo "SYNTAX: $0 <resource dir> <signture dir> <image dir> <dtb name> <target image>"
    exit 1
}

RESOURCE_DIR="$1"
SIG_DIR="$2"
IMAGE_DIR="$3"
DEVICE_NAME="$4"

DTC_COMPILER="$5"
DTC_FLAGS="$6"

LINUX_DIR="$7"
TARGET_CROSS="$8"

RAMDISK_SRC_DIR="$9"
RAMDISK_CONFIG="${10}"

TARGET_IMAGE_NAME="${11}"

function pack_error()
{
	echo -e "\033[47;31mERROR: $*\033[0m"
}

function pack_bar()
{
	echo "######################################################################"
	echo -e "\033[47;30mPACK: $*\033[0m"
	echo "######################################################################"
}


# define option, format:
#   'long option' 'default value' 'help message' 'short option'
# DEFINE_string 'chip' '' 'chip to build, e.g. sun7i' 'c'
# DEFINE_string 'platform' '' 'platform to build, e.g. linux, android, camdroid' 'p'
# DEFINE_string 'board' '' 'board to build, e.g. evb' 'b'
# DEFINE_string 'kernel' '' 'kernel to build, e.g. linux-3.4, linux-3.10' 'k'
# DEFINE_string 'debug_mode' 'uart0' 'config debug mode, e.g. uart0, card0' 'd'
# DEFINE_string 'signture' 'none' 'pack boot signture to do secure boot' 's'
# DEFINE_string 'secure' 'none' 'pack secure boot with -v arg' 'v'
# DEFINE_string 'mode' 'normal' 'pack dump firmware' 'm'
# DEFINE_string 'function' 'android' 'pack private firmware' 'f'
# DEFINE_string 'vsp' '' 'pack firmware for vsp' 't'

# # parse the command-line
# FLAGS "$@" || exit $?
# eval set -- "${FLAGS_ARGV}"

PACK_CHIP="sun50iw1p1"
PACK_PLATFORM="linux"
PACK_BOARD="owt"
PACK_KERN="4.9"
PACK_DEBUG="uart0"
PACK_SIG="none"
PACK_SECURE="none"
PACK_MODE="none"
PACK_FUNC="none"
PACK_VSP="none"



export ARCH=arm64
export PATH=${RESOURCE_DIR}/tools/mod_update:$PATH
export PATH=${RESOURCE_DIR}/tools/openssl:$PATH
export PATH=${RESOURCE_DIR}/tools/eDragonEx:$PATH
export PATH=${RESOURCE_DIR}/tools/fsbuild200:$PATH
export PATH=${RESOURCE_DIR}/tools/android:$PATH

tools_file_list=(
${RESOURCE_DIR}/common/tools/split_xxxx.fex
${RESOURCE_DIR}/resource/tools/split_xxxx.fex
${RESOURCE_DIR}/common/tools/usbtool_test.fex
${RESOURCE_DIR}/common/tools/usbtool_crash.fex
${RESOURCE_DIR}/resource/tools/usbtool_test.fex
${RESOURCE_DIR}/common/tools/cardscript.fex
${RESOURCE_DIR}/common/tools/cardscript_secure.fex
${RESOURCE_DIR}/resource/tools/cardscript.fex
${RESOURCE_DIR}/resource/tools/cardscript_secure.fex
${RESOURCE_DIR}/common/tools/cardtool.fex
${RESOURCE_DIR}/resource/tools/cardtool.fex
${RESOURCE_DIR}/common/tools/usbtool.fex
${RESOURCE_DIR}/resource/tools/usbtool.fex
${RESOURCE_DIR}/common/tools/aultls32.fex
${RESOURCE_DIR}/resource/tools/aultls32.fex
${RESOURCE_DIR}/common/tools/aultools.fex
${RESOURCE_DIR}/resource/tools/aultools.fex
)

configs_file_list=(
${RESOURCE_DIR}/common/toc/toc1.fex
${RESOURCE_DIR}/common/toc/toc0.fex
${RESOURCE_DIR}/common/toc/boot_package.fex
${RESOURCE_DIR}/common/hdcp/esm.fex
${RESOURCE_DIR}/common/dtb/sunxi.fex
${RESOURCE_DIR}/common/imagecfg/*.cfg
${RESOURCE_DIR}/common/partition/sys_partition_dump.fex
${RESOURCE_DIR}/common/partition/sys_partition_private.fex
${RESOURCE_DIR}/common/version/version_base.mk
${RESOURCE_DIR}/resource/configs/default/*
${RESOURCE_DIR}/resource/configs/owt/*.fex
${RESOURCE_DIR}/resource/configs/owt/*.cfg
${RESOURCE_DIR}/resource/configs/default/version_base.mk
)

boot_resource_list=(
${RESOURCE_DIR}/resource/boot-resource/boot-resource:${SIG_DIR}/
${RESOURCE_DIR}/resource/boot-resource/boot-resource.ini:${SIG_DIR}/
${RESOURCE_DIR}/resource/configs/owt/bootlogo.bmp:${SIG_DIR}/boot-resource/
${RESOURCE_DIR}/resource/configs/owt/bootlogo.bmp:${SIG_DIR}/bootlogo.bmp
${RESOURCE_DIR}/resource/boot-resource/boot-resource/bat/bempty.bmp:${SIG_DIR}/bempty.bmp
${RESOURCE_DIR}/resource/boot-resource/boot-resource/bat/battery_charge.bmp:${SIG_DIR}/battery_charge.bmp
)

boot_file_list=(
${IMAGE_DIR}/${DEVICE_NAME}-nand.bin:${SIG_DIR}/boot0_nand.fex
${IMAGE_DIR}/${DEVICE_NAME}-mmc.bin:${SIG_DIR}/boot0_sdcard.fex
${IMAGE_DIR}/${DEVICE_NAME}-fes.bin:${SIG_DIR}/fes1.fex
${IMAGE_DIR}/${DEVICE_NAME}-u-boot-with-spl.bin:${SIG_DIR}/u-boot.fex
${RESOURCE_DIR}/resource/bin/bl31.bin:${SIG_DIR}/monitor.fex
${RESOURCE_DIR}/resource/bin/scp.bin:${SIG_DIR}/scp.fex
${RESOURCE_DIR}/resource/bin/arisc.fex:${SIG_DIR}/arisc.fex
${RESOURCE_DIR}/resource/bin/optee_sun50iw1p1.bin:${SIG_DIR}/optee.fex
)

# ${IMAGE_DIR}/bl31.bin:${SIG_DIR}/monitor.fex

a64_boot_file_secure=(
${RESOURCE_DIR}/resource/bin/optee_${PACK_CHIP}.bin:${SIG_DIR}/optee.fex
${IMAGE_DIR}/${DEVICE_NAME}-sboot.bin:${SIG_DIR}/sboot.bin
)


function cpio_rootfs()
{
    if [ -d $1 ] ; then
    	(cd $1; find . | fakeroot cpio -o -Hnewc | gzip > ../"$2")
    else
    	echo "skel not exist"
    	exit 1
    fi
}

function un_cpio_rootfs()
{
    if [ -f "$1" ] ; then
    	rm -rf $2 && mkdir $2
    	gzip -dc $1 | (cd $2; fakeroot cpio -i)
    else
    	echo "$1 not exist"
    	exit 1
    fi
}

function ramdisk_install(){
	cd ${RAMDISK_SRC_DIR}/

	if [ -f "output/images/rootfs.cpio.gz" ]; then
		cp -vf output/images/rootfs.cpio.gz ${IMAGE_DIR}/rootfs.cpio.gz
	else
		echo "build ramdisk with 'source ramdisk.sh' in ${RAMDISK_SRC_DIR}"
		exit 1
	fi
}

function uart_switch()
{
	rm -rf ${SIG_DIR}/awk_debug_card0
	touch ${SIG_DIR}/awk_debug_card0
	TX=`awk  '$0~"'$PACK_CHIP'"{print $2}' ${RESOURCE_DIR}/tools/card_debug_pin`
	RX=`awk  '$0~"'$PACK_CHIP'"{print $3}' ${RESOURCE_DIR}/tools/card_debug_pin`
	PORT=`awk  '$0~"'$PACK_CHIP'"{print $4}' ${RESOURCE_DIR}/tools/card_debug_pin`
	MS=`awk  '$0~"'$PACK_CHIP'"{print $5}' ${RESOURCE_DIR}/tools/card_debug_pin`
	CK=`awk  '$0~"'$PACK_CHIP'"{print $6}' ${RESOURCE_DIR}/tools/card_debug_pin`
	DO=`awk  '$0~"'$PACK_CHIP'"{print $7}' ${RESOURCE_DIR}/tools/card_debug_pin`
	DI=`awk  '$0~"'$PACK_CHIP'"{print $8}' ${RESOURCE_DIR}/tools/card_debug_pin`

	BOOT_UART_ST=`awk  '$0~"'$PACK_CHIP'"{print $2}' ${RESOURCE_DIR}/tools/card_debug_string`
	BOOT_PORT_ST=`awk  '$0~"'$PACK_CHIP'"{print $3}' ${RESOURCE_DIR}/tools/card_debug_string`
	BOOT_TX_ST=`awk  '$0~"'$PACK_CHIP'"{print $4}' ${RESOURCE_DIR}/tools/card_debug_string`
	BOOT_RX_ST=`awk  '$0~"'$PACK_CHIP'"{print $5}' ${RESOURCE_DIR}/tools/card_debug_string`
	UART0_ST=`awk  '$0~"'$PACK_CHIP'"{print $6}' ${RESOURCE_DIR}/tools/card_debug_string`
	UART0_USED_ST=`awk  '$0~"'$PACK_CHIP'"{print $7}' ${RESOURCE_DIR}/tools/card_debug_string`
	UART0_PORT_ST=`awk  '$0~"'$PACK_CHIP'"{print $8}' ${RESOURCE_DIR}/tools/card_debug_string`
	UART0_TX_ST=`awk  '$0~"'$PACK_CHIP'"{print $9}' ${RESOURCE_DIR}/tools/card_debug_string`
	UART0_RX_ST=`awk  '$0~"'$PACK_CHIP'"{print $10}' ${RESOURCE_DIR}/tools/card_debug_string`
	UART1_ST=`awk  '$0~"'$PACK_CHIP'"{print $11}' ${RESOURCE_DIR}/tools/card_debug_string`
	JTAG_ST=`awk  '$0~"'$PACK_CHIP'"{print $12}' ${RESOURCE_DIR}/tools/card_debug_string`
	MS_ST=`awk  '$0~"'$PACK_CHIP'"{print $13}' ${RESOURCE_DIR}/tools/card_debug_string`
	CK_ST=`awk  '$0~"'$PACK_CHIP'"{print $14}' ${RESOURCE_DIR}/tools/card_debug_string`
	DO_ST=`awk  '$0~"'$PACK_CHIP'"{print $15}' ${RESOURCE_DIR}/tools/card_debug_string`
	DI_ST=`awk  '$0~"'$PACK_CHIP'"{print $16}' ${RESOURCE_DIR}/tools/card_debug_string`
	MMC0_ST=`awk  '$0~"'$PACK_CHIP'"{print $17}' ${RESOURCE_DIR}/tools/card_debug_string`
	MMC0_USED_ST=`awk  '$0~"'$PACK_CHIP'"{print $18}' ${RESOURCE_DIR}/tools/card_debug_string`

	echo '$0!~";" && $0~"'$BOOT_TX_ST'"{if(C)$0="'$BOOT_TX_ST' = '$TX'"} \' >> ${SIG_DIR}/awk_debug_card0
	echo '$0!~";" && $0~"'$BOOT_RX_ST'"{if(C)$0="'$BOOT_RX_ST' = '$RX'"} \' >> ${SIG_DIR}/awk_debug_card0
	echo '$0!~";" && $0~"'$BOOT_PORT_ST'"{if(C)$0="'$BOOT_PORT_ST' = '$PORT'"} \' >> ${SIG_DIR}/awk_debug_card0
	if [ "`grep "auto_print_used" "${SIG_DIR}/sys_config.fex" | grep "1"`" ]; then
		echo '$0!~";" && $0~"'$MMC0_USED_ST'"{if(A)$0="'$MMC0_USED_ST' = 1";A=0} \' >> ${SIG_DIR}/awk_debug_card0
	else
		echo '$0!~";" && $0~"'$MMC0_USED_ST'"{if(A)$0="'$MMC0_USED_ST' = 0";A=0} \' >> ${SIG_DIR}/awk_debug_card0
	fi
	echo '$0!~";" && $0~"\\['$MMC0_ST'\\]"{A=1}  \' >> ${SIG_DIR}/awk_debug_card0
	echo '$0!~";" && $0~"'$UART0_TX_ST'"{if(B)$0="'$UART0_TX_ST' = '$TX'"} \' >> ${SIG_DIR}/awk_debug_card0
	echo '$0!~";" && $0~"'$UART0_RX_ST'"{if(B)$0="'$UART0_RX_ST' = '$RX'"} \' >> ${SIG_DIR}/awk_debug_card0
	echo '$0!~";" && $0~"\\['$UART0_ST'\\]"{B=1} \' >> ${SIG_DIR}/awk_debug_card0
	echo '$0!~";" && $0~"'$UART0_USED_ST'"{if(B)$0="'$UART0_USED_ST' = 1"}  \' >> ${SIG_DIR}/awk_debug_card0
	echo '/^'$UART0_PORT_ST'/{next} \' >> ${SIG_DIR}/awk_debug_card0
	echo '$0!~";" && $0~"\\['$UART1_ST'\\]"{B=0} \' >> ${SIG_DIR}/awk_debug_card0
	echo '$0!~";" && $0~"\\['$BOOT_UART_ST'\\]"{C=1} \' >> ${SIG_DIR}/awk_debug_card0
	echo '$0!~";" && $0~"\\['$JTAG_ST'\\]"{C=0} \' >> ${SIG_DIR}/awk_debug_card0
	echo '$0!~";" && $0~"'$MS_ST'"{$0="'$MS_ST' = '$MS'"} \' >> ${SIG_DIR}/awk_debug_card0
	echo '$0!~";" && $0~"'$CK_ST'"{$0="'$CK_ST' = '$CK'"} \' >> ${SIG_DIR}/awk_debug_card0
	echo '$0!~";" && $0~"'$DO_ST'"{$0="'$DO_ST' = '$DO'"} \' >> ${SIG_DIR}/awk_debug_card0
	echo '$0!~";" && $0~"'$DI_ST'"{$0="'$DI_ST' = '$DI'"} \' >> ${SIG_DIR}/awk_debug_card0
	echo '1' >> ${SIG_DIR}/awk_debug_card0

	if [ "`grep "auto_print_used" "${SIG_DIR}/sys_config.fex" | grep "1"`" ]; then
		sed -i -e '/^uart0_rx/a\pinctrl-1=' ${SIG_DIR}/sys_config.fex
		sed -i -e '/^uart0_rx/a\pinctrl-0=' ${SIG_DIR}/sys_config.fex
	fi

	awk -f ${SIG_DIR}/awk_debug_card0 ${SIG_DIR}/sys_config.fex > ${SIG_DIR}/sys_config_debug.fex
	rm -f ${SIG_DIR}/sys_config.fex
	mv ${SIG_DIR}/sys_config_debug.fex ${SIG_DIR}/sys_config.fex
	echo "uart -> card0"
}


function do_prepare()
{
	pack_bar "do_prepare"

	cd ${SIG_DIR}

	printf "copying tools file\n"
	for file in ${tools_file_list[@]} ; do
		if [ -f $file ]; then
			cp -f $file ${SIG_DIR}/ 2> /dev/null
		fi
	done

	printf "copying configs file\n"
	for file in ${configs_file_list[@]} ; do
		if [ -f $file ]; then
			cp -f $file ${SIG_DIR}/ 2> /dev/null
		fi
	done

	if [ -f ${RESOURCE_DIR}/fex/${DEVICE_NAME}-4.9.fex ]; then
		rm -f sys_config.fex
		cp ${RESOURCE_DIR}/fex/${DEVICE_NAME}-4.9.fex sys_config.fex
	fi

	# If platform config files exist, we will cover the default files
	# For example, mv ${SIG_DIR}/image_linux.cfg ${SIG_DIR}/image.cfg
	find . -type f -a \( -name "*.fex" -o -name "*.cfg" \) -print | \
		sed "s#\(.*\)_${PACK_PLATFORM}\(\..*\)#mv -fv & \1\2#e"

	if [ "x${PACK_MODE}" = "xdump" ] ; then
		cp -vf sys_partition_dump.fex sys_partition.fex
		cp -vf usbtool_test.fex usbtool.fex
	elif [ "x${PACK_FUNC}" = "xprvt" ] ; then
		cp -vf sys_partition_private.fex sys_partition.fex
	fi

	printf "copying boot resource\n"
	for file in ${boot_resource_list[@]} ; do
		cp -rf `echo $file | awk -F: '{print $1}'` \
			`echo $file | awk -F: '{print $2}'` 2>/dev/null
	done

	if [ ! -f bootlogo.bmp ]; then
		cp ${RESOURCE_DIR}/resource/boot-resource/bootlogo.bmp bootlogo.bmp
	fi

	if [ -f bootlogo.bmp.lzma ]; then
		rm -rf bootlogo.bmp.lzma
	fi
	/usr/bin/lzma -k bootlogo.bmp

	if [ -f bempty.bmp.lzma ]; then
		rm -rf bempty.bmp.lzma
	fi
	/usr/bin/lzma -k bempty.bmp

	if [ -f battery_charge.bmp.lzma ]; then
		rm -rf battery_charge.bmp.lzma
	fi
	/usr/bin/lzma -k battery_charge.bmp

	printf "copying boot file\n"
	for file in ${boot_file_list[@]} ; do
		cp -f `echo $file | awk -F: '{print $1}'` \
			`echo $file | awk -F: '{print $2}'` 2>/dev/null
	done

	if [ "x${PACK_SECURE}" = "xsecure" -o "x${PACK_SIG}" = "xsecure" -o  "x${PACK_SIG}" = "xprev_refurbish" ] ; then
			printf "copying arm64 secure boot file\n"
			for file in ${a64_boot_file_secure[@]} ; do
				cp -f `echo $file | awk -F: '{print $1}'` \
					`echo $file | awk -F: '{print $2}'`
			done
	fi
	
	if [ "x${PACK_MODE}" = "xcrashdump" ] ; then
		cp -vf ${SIG_DIR}/sys_partition_dump.fex sys_partition.fex
		cp -vf ${SIG_DIR}/image_crashdump.cfg image.cfg
		cp -vf ${RESOURCE_DIR}/resource/bin/u-boot-crashdump-${PACK_CHIP}.bin u-boot.fex
	fi

	
	if [ "x${PACK_SECURE}" = "xsecure"  -o "x${PACK_SIG}" = "xsecure" ] ; then
		printf "add burn_secure_mode in target in sys config\n"
		sed -i -e '/^\[target\]/a\burn_secure_mode=1' sys_config.fex
		sed -i -e '/^\[platform\]/a\secure_without_OS=0' sys_config.fex
	elif [ "x${PACK_SIG}" = "xprev_refurbish" ] ; then
		printf "add burn_secure_mode in target in sys config\n"
		sed -i -e '/^\[target\]/a\burn_secure_mode=1' sys_config.fex
		sed -i -e '/^\[platform\]/a\secure_without_OS=1' sys_config.fex
	else
		sed -i '/^burn_secure_mod/d' sys_config.fex
		sed -i '/^secure_without_OS/d' sys_config.fex
	fi

	if [ "x${PACK_VSP}" = "xvsp" ] ; then
		printf "change usb_port_type to device...\n"
		printf "disable usb_serial_unique...\n"
		printf "change usb_serial_number to ${PACK_PLATFORM}_android...\n"
		sed -i 's/^usb_port_type.*/usb_port_type = 0/g' sys_config.fex 
		sed -i 's/^usb_serial_unique.*/usb_serial_unique = 0/g' sys_config.fex
		sed -i 's/^usb_serial_number.*/usb_serial_number = "'"${PACK_CHIP}"'_android"/g' sys_config.fex
	fi

	# Here, we can switch uart to card or normal
	if [ "x${PACK_DEBUG}" = "xcard0" -a "x${PACK_MODE}" != "xdump" \
		-a "x${PACK_FUNC}" != "xprvt" ] ; then \
		uart_switch
	else
		sed -i -e '/^auto_print_used/s\1\0\' sys_config.fex
	fi

	sed -i 's/\\boot-resource/\/boot-resource/g' boot-resource.ini
	sed -i 's/\\\\/\//g' image.cfg
	sed -i 's/^imagename/;imagename/g' image.cfg

	IMG_NAME="${PACK_CHIP}_${PACK_PLATFORM}_${PACK_BOARD}_${PACK_DEBUG}"

	if [ "x${PACK_SIG}" != "xnone" ]; then
		IMG_NAME="${IMG_NAME}_${PACK_SIG}"
	fi

	if [ "x${PACK_MODE}" = "xdump" -o "x${PACK_MODE}" = "xota_test" -o "x${PACK_MODE}" = "xcrashdump" ] ; then
		IMG_NAME="${IMG_NAME}_${PACK_MODE}"
	fi

	if [ "x${PACK_FUNC}" = "xprvt" ] ; then
		IMG_NAME="${IMG_NAME}_${PACK_FUNC}"
	fi

	if [ "x${PACK_SECURE}" = "xsecure" ] ; then
		IMG_NAME="${IMG_NAME}_${PACK_SECURE}"
	fi

	if [ "x${PACK_FUNC}" = "xprev_refurbish" ] ; then
		IMG_NAME="${IMG_NAME}_${PACK_FUNC}"
	fi

	if [ "x${PACK_SECURE}" != "xnone" -o "x${PACK_SIG}" != "xnone" ]; then
		MAIN_VERION=`awk  '$0~"MAIN_VERSION"{printf"%d", $3}' version_base.mk`

		IMG_NAME="${IMG_NAME}_v${MAIN_VERION}.img"
	else
		IMG_NAME="${IMG_NAME}.img"
	fi

	echo "imagename = $IMG_NAME" >> image.cfg
	echo "" >> image.cfg
}

function do_dts()
{	
	pack_bar "do_ini_to_dts"

	cd ${SIG_DIR}/

	local DTC_DEP_FILE=${DEVICE_NAME}.dtc.dep
	local DTC_SRC_PATH=${SIG_DIR}
	local DTC_SRC_FILE=${DEVICE_NAME}.dtc.src

	local DTC_INI_FILE_BASE=sys_config.fex
	local DTC_INI_FILE=sys_config_fix.fex

	if [ -f $DTC_INI_FILE ]; then
		rm -f $DTC_INI_FILE
	fi
	cp $DTC_INI_FILE_BASE $DTC_INI_FILE
	sed -i "s/\(\[dram\)_para\(\]\)/\1\2/g" $DTC_INI_FILE
	sed -i "s/\(\[nand[0-9]\)_para\(\]\)/\1\2/g" $DTC_INI_FILE

	if [ ! -f $DTC_COMPILER ]; then
		pack_error "Script_to_dts: Can not find dtc compiler. $DTC_COMPILER \n"
		exit 1
	fi

	$DTC_COMPILER ${DTC_FLAGS} -W no-unit_address_vs_reg -O dtb -o sunxi.dtb	\
		-b 0			\
		-i $DTC_SRC_PATH	\
		-F $DTC_INI_FILE	\
		-d $DTC_DEP_FILE $DTC_SRC_FILE

	if [ $? -ne 0 ]; then
		pack_error "Conver script to dts failed"
		exit 1
	fi

	pack_bar "Conver script to dts ok.\n"

	# It'is used for debug dtb
	$DTC_COMPILER ${DTC_FLAGS} -W no-unit_address_vs_reg -I dtb -O dts -o ${IMAGE_DIR}/${DEVICE_NAME}.dts sunxi.dtb

	if [ -f ${IMAGE_DIR}/${DEVICE_NAME}.dtb ]; then
		rm -f ${IMAGE_DIR}/${DEVICE_NAME}.dtb
	fi
	cp sunxi.dtb ${IMAGE_DIR}/${DEVICE_NAME}.dtb
	return
}

do_ramfs()
{
    local bss_sz=0;
    local CHIP="";

    local RAMDISK=${IMAGE_DIR}/rootfs.cpio.gz;
    local BASE="";
    local RAMDISK_OFFSET="";
    local KERNEL_OFFSET="";
    local BIMAGE="${IMAGE_DIR}/Image";

    cd ${SIG_DIR}/

    # update rootfs.cpio.gz with new module files
    # regen_rootfs_cpio

    CHIP=`echo ${PACK_CHIP} | sed -e 's/.*\(sun[0-9x]*i\).*/\1/g'`;

    BASE="0x40000000";
    KERNEL_OFFSET="0x80000";

    if [ -f "${LINUX_DIR}/vmlinux" ]; then
        bss_sz=`${TARGET_CROSS}readelf -S ${LINUX_DIR}/vmlinux | \
            awk  '/\.bss/ {print strtonum("0x"$5)+strtonum("0x"$6)}'`;
    fi

    # If the size of bImage larger than 16M, will offset 0x02000000
    if [ ${bss_sz} -gt $((16#1000000)) ]; then
        RAMDISK_OFFSET="0x02000000";
    else
        RAMDISK_OFFSET="0x01000000";
    fi

    mkbootimg --kernel ${BIMAGE} \
        --ramdisk ${RAMDISK} \
        --board ${CHIP}_${ARCH} \
        --base ${BASE} \
        --kernel_offset ${KERNEL_OFFSET} \
        --ramdisk_offset ${RAMDISK_OFFSET} \
        -o boot.img
}

function do_common()
{
	pack_bar "do_common"

	cd ${SIG_DIR}/

	if [ ! -f board_config.fex ]; then
		echo "[empty]" > board_config.fex
	fi

	busybox unix2dos sys_config.fex
	busybox unix2dos board_config.fex
	script  sys_config.fex > /dev/null
	cp -f   sys_config.bin config.fex

	script  board_config.fex > /dev/null
	cp -f board_config.bin board.fex

	busybox unix2dos sys_partition.fex
	script  sys_partition.fex > /dev/null

	if [ -f "sunxi.dtb" ]; then
		cp -f sunxi.dtb sunxi.fex
		update_dtb sunxi.fex 4096
	fi

	if [ -f "scp.fex" ]; then
		echo "update scp"
		update_scp scp.fex sunxi.fex >/dev/null
	fi

	# Those files for Nand or Card
	update_boot0 boot0_nand.fex	sys_config.bin NAND > /dev/null
	update_boot0 boot0_sdcard.fex	sys_config.bin SDMMC_CARD > /dev/null
	update_uboot -no_merge u-boot.fex sys_config.bin > /dev/null
	
	update_fes1  fes1.fex           sys_config.bin > /dev/null
	fsbuild	     boot-resource.ini split_xxxx.fex > /dev/null

	if [ -f boot_package.cfg ]; then
			echo "pack boot package"
			busybox unix2dos boot_package.cfg
			dragonsecboot -pack boot_package.cfg

		if [ $? -ne 0 ]
		then
			pack_error "dragon pack run error"
			exit 1
		fi
	fi

	if [ "x${PACK_FUNC}" = "xprvt" ] ; then
		u_boot_env_gen env_burn.cfg env.fex > /dev/null
	else
		u_boot_env_gen env.cfg env.fex > /dev/null
	fi

	if [ -f "arisc" ]; then
		cp -vf arisc arisc.fex
	fi
}

function do_signature()
{

	pack_bar "prepare for signature by openssl\n"

	cd ${SIG_DIR}

	if [ "x${PACK_SIG}" = "xprev_refurbish" ] ; then
		cp -v ${RESOURCE_DIR}/common/sign_config/dragon_toc_a64_no_secureos.cfg dragon_toc.cfg
	else
		if [ -f ${RESOURCE_DIR}/resource/configs/default/dragon_toc.cfg ] ; then
			cp -v ${RESOURCE_DIR}/resource/configs/default/dragon_toc.cfg dragon_toc.cfg
		else
			cp -v ${RESOURCE_DIR}/common/sign_config/dragon_toc_a64.cfg dragon_toc.cfg
		fi
	fi

	if [ $? -ne 0 ]
	then
		pack_error "dragon toc config file is not exist"
		exit 1
	fi

	rm -f cardscript.fex
	mv cardscript_secure.fex cardscript.fex
	if [ $? -ne 0 ]
	then
		pack_error "dragon cardscript_secure.fex file is not exist"
		exit 1
	fi

	dragonsecboot -key dragon_toc.cfg keys

	dragonsecboot -toc0 dragon_toc.cfg keys  > /dev/null
	if [ $? -ne 0 ]
	then
		pack_error "dragon toc0 run error"
		exit 1
	fi

	update_toc0  toc0.fex           sys_config.bin
	if [ $? -ne 0 ]
	then
		pack_error "update toc0 run error"
		exit 1
	fi

	dragonsecboot -toc1 dragon_toc.cfg keys ${RESOURCE_DIR}/common/sign_config/cnf_base.cnf version_base.mk
	if [ $? -ne 0 ]
	then
		pack_error "dragon toc1 run error"
		exit 1
	fi

	sigbootimg --image boot.fex --cert toc1/cert/boot.der --output boot_sig.fex
	if [ $? -ne 0 ] ; then
		pack_error "Pack cert to image error"
		exit 1
	else
		mv -f boot_sig.fex boot.fex
	fi

	pack_bar "secure signature ok!"
}

function do_pack()
{
	pack_bar "packing for owt \n"

	cd ${SIG_DIR}

	if [ -f ${SIG_DIR}/boot.fex ]; then
		rm -rf ${SIG_DIR}/boot.fex
	fi

	if [ -f boot.img ]; then
		cp -f boot.img boot.fex
	else 
		cp -f ${IMAGE_DIR}/boot.img        		${SIG_DIR}/boot.fex
	fi

	cp -f ${IMAGE_DIR}/_rootfs.ext4     		${SIG_DIR}/rootfs.fex

	if [ -f ${SIG_DIR}/vmlinux.fex ]; then
		rm -rf ${SIG_DIR}/vmlinux.fex
	fi
	cp -f ${SIG_DIR}/boot.fex     		${SIG_DIR}/vmlinux.fex
	cp -f ${SIG_DIR}/boot.fex     		${SIG_DIR}/recovery.fex

	if [ "x${PACK_SIG}" = "xsecure" ] ; then
		echo "secure"
		do_signature
	elif [ "x${PACK_SIG}" = "xprev_refurbish" ] ; then
		echo "prev_refurbish"
		do_signature
	else
		echo "normal"
	fi
}

function do_finish()
{
	pack_bar "do_finish"

	cd ${SIG_DIR}

	if [ ! -f sys_partition_nor.bin ]; then
		update_mbr          sys_partition.bin 4
		if [ $? -ne 0 ]; then
			pack_error "update_mbr failed"
			exit 1
		fi
	fi
	dragon image.cfg    sys_partition.fex
        if [ $? -eq 0 ]; then
	    if [ -e ${IMG_NAME} ]; then
		    mv ${IMG_NAME} ${TARGET_IMAGE_NAME}
		    pack_bar "image is at ${TARGET_IMAGE_NAME}"
	    fi
        fi

	pack_bar "pack finish\n"
}

ramdisk_install

do_prepare
do_dts
do_ramfs
do_common
do_pack
do_finish
