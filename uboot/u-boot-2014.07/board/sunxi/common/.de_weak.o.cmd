cmd_board/sunxi/common/de_weak.o := /aosp/brandy/u-boot-2014.07/../gcc-linaro/bin/arm-linux-gnueabi-gcc -Wp,-MD,board/sunxi/common/.de_weak.o.d  -nostdinc -isystem /aosp/brandy/gcc-linaro/bin/../lib/gcc/arm-linux-gnueabi/4.6.3/include -Iinclude  -I/aosp/brandy/u-boot-2014.07/arch/arm/include -I/aosp/brandy/u-boot-2014.07/include/openssl -D__KERNEL__ -DCONFIG_SYS_TEXT_BASE=0x4A000000 -Wall -Wstrict-prototypes -Wno-format-security -fno-builtin -ffreestanding -Os -fno-stack-protector -g -fstack-usage -Wno-format-nonliteral -march=armv7-a -Werror -mno-unaligned-access -DCONFIG_ARM -D__ARM__ -marm -mno-thumb-interwork -mabi=aapcs-linux -mword-relocations -ffunction-sections -fdata-sections -fno-common -ffixed-r9 -msoft-float -pipe    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(de_weak)"  -D"KBUILD_MODNAME=KBUILD_STR(de_weak)" -c -o board/sunxi/common/de_weak.o board/sunxi/common/de_weak.c

source_board/sunxi/common/de_weak.o := board/sunxi/common/de_weak.c

deps_board/sunxi/common/de_weak.o := \
    $(wildcard include/config/sunxi/display.h) \

board/sunxi/common/de_weak.o: $(deps_board/sunxi/common/de_weak.o)

$(deps_board/sunxi/common/de_weak.o):
