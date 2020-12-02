cmd_drivers/video/sunxi/disp2/disp/de/lowlevel_sun50iw1/de_feat.o := /aosp/brandy/u-boot-2014.07/../gcc-linaro/bin/arm-linux-gnueabi-gcc -Wp,-MD,drivers/video/sunxi/disp2/disp/de/lowlevel_sun50iw1/.de_feat.o.d  -nostdinc -isystem /aosp/brandy/gcc-linaro/bin/../lib/gcc/arm-linux-gnueabi/4.6.3/include -Iinclude  -I/aosp/brandy/u-boot-2014.07/arch/arm/include -I/aosp/brandy/u-boot-2014.07/include/openssl -D__KERNEL__ -DCONFIG_SYS_TEXT_BASE=0x4A000000 -Wall -Wstrict-prototypes -Wno-format-security -fno-builtin -ffreestanding -Os -fno-stack-protector -g -fstack-usage -Wno-format-nonliteral -march=armv7-a -Werror -mno-unaligned-access -DCONFIG_ARM -D__ARM__ -marm -mno-thumb-interwork -mabi=aapcs-linux -mword-relocations -ffunction-sections -fdata-sections -fno-common -ffixed-r9 -msoft-float -pipe    -D"KBUILD_STR(s)=\#s" -D"KBUILD_BASENAME=KBUILD_STR(de_feat)"  -D"KBUILD_MODNAME=KBUILD_STR(de_feat)" -c -o drivers/video/sunxi/disp2/disp/de/lowlevel_sun50iw1/de_feat.o drivers/video/sunxi/disp2/disp/de/lowlevel_sun50iw1/de_feat.c

source_drivers/video/sunxi/disp2/disp/de/lowlevel_sun50iw1/de_feat.o := drivers/video/sunxi/disp2/disp/de/lowlevel_sun50iw1/de_feat.c

deps_drivers/video/sunxi/disp2/disp/de/lowlevel_sun50iw1/de_feat.o := \
  drivers/video/sunxi/disp2/disp/de/lowlevel_sun50iw1/de_feat.h \
    $(wildcard include/config/use/ac200.h) \

drivers/video/sunxi/disp2/disp/de/lowlevel_sun50iw1/de_feat.o: $(deps_drivers/video/sunxi/disp2/disp/de/lowlevel_sun50iw1/de_feat.o)

$(deps_drivers/video/sunxi/disp2/disp/de/lowlevel_sun50iw1/de_feat.o):
