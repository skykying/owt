#
# Copyright (c) 2015 Allwinnertech, Inc.
# Tyle <tyle at allwinnertech.com>
#
#


ojb-$(CONFIG_EINK_PANEL_USED) += de_eink.o disp_eink_data.o \
		disp_waveform.o rtmx_eink.o


obj-y += de_fcc.o de_gsu.o de_hal.o disp_al.o \
		de_peak.o de_rtmx.o de_scaler_table.o\
		de_vep_table.o de_vsu.o de_feat.o \
		de_lcd.o de_lti.o de_fce.o \
		de_ase.o de_bws.o de_enhance.o \
		de_smbl.o de_ccsc.o de_dcsc.o \
		de_wb.o de_clock.o $(eink_obj) $(dsi_obj)

ifeq ($(MIPI_DSI_IP_VERSION),40)
obj-y += de_dsi.o
else
obj-y += de_dsi_28.o
endif

