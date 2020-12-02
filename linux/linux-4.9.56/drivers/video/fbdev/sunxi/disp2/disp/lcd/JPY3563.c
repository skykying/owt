#include "JPY3563.h"


static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

/* ILI9488 screen size */
#define ILI9488_LCD_WIDTH  320
#define ILI9488_LCD_HEIGHT 480

/* ILI9488 ID code */
#define ILI9488_DEVICE_CODE (0x9488u)

/* Level 1 Commands (from the display Datasheet) */
#define ILI9488_CMD_NOP					        0x00
#define ILI9488_CMD_SOFTWARE_RESET			    0x01
#define ILI9488_CMD_READ_DISP_ID			    0x04
#define ILI9488_CMD_READ_ERROR_DSI			    0x05
#define ILI9488_CMD_READ_DISP_STATUS			0x09
#define ILI9488_CMD_READ_DISP_POWER_MODE		0x0A
#define ILI9488_CMD_READ_DISP_MADCTRL			0x0B
#define ILI9488_CMD_READ_DISP_PIXEL_FORMAT		0x0C
#define ILI9488_CMD_READ_DISP_IMAGE_MODE		0x0D
#define ILI9488_CMD_READ_DISP_SIGNAL_MODE		0x0E
#define ILI9488_CMD_READ_DISP_SELF_DIAGNOSTIC   0x0F
#define ILI9488_CMD_ENTER_SLEEP_MODE			0x10
#define ILI9488_CMD_SLEEP_OUT				    0x11
#define ILI9488_CMD_PARTIAL_MODE_ON			    0x12
#define ILI9488_CMD_NORMAL_DISP_MODE_ON			0x13
#define ILI9488_CMD_DISP_INVERSION_OFF			0x20
#define ILI9488_CMD_DISP_INVERSION_ON			0x21
#define ILI9488_CMD_PIXEL_OFF				    0x22
#define ILI9488_CMD_PIXEL_ON				    0x23
#define ILI9488_CMD_DISPLAY_OFF				    0x28
#define ILI9488_CMD_DISPLAY_ON				    0x29
#define ILI9488_CMD_COLUMN_ADDRESS_SET			0x2A
#define ILI9488_CMD_PAGE_ADDRESS_SET			0x2B
#define ILI9488_CMD_MEMORY_WRITE			    0x2C
#define ILI9488_CMD_MEMORY_READ				    0x2E
#define ILI9488_CMD_PARTIAL_AREA			    0x30
#define ILI9488_CMD_VERT_SCROLL_DEFINITION		0x33
#define ILI9488_CMD_TEARING_EFFECT_LINE_OFF		0x34
#define ILI9488_CMD_TEARING_EFFECT_LINE_ON		0x35
#define ILI9488_CMD_MEMORY_ACCESS_CONTROL		0x36
#define ILI9488_CMD_VERT_SCROLL_START_ADDRESS	0x37
#define ILI9488_CMD_IDLE_MODE_OFF			    0x38
#define ILI9488_CMD_IDLE_MODE_ON			    0x39
#define ILI9488_CMD_COLMOD_PIXEL_FORMAT_SET		0x3A
#define ILI9488_CMD_WRITE_MEMORY_CONTINUE		0x3C
#define ILI9488_CMD_READ_MEMORY_CONTINUE		0x3E
#define ILI9488_CMD_SET_TEAR_SCANLINE			0x44
#define ILI9488_CMD_GET_SCANLINE			    0x45
#define ILI9488_CMD_WRITE_DISPLAY_BRIGHTNESS    0x51
#define ILI9488_CMD_READ_DISPLAY_BRIGHTNESS		0x52
#define ILI9488_CMD_WRITE_CTRL_DISPLAY			0x53
#define ILI9488_CMD_READ_CTRL_DISPLAY			0x54
#define ILI9488_CMD_WRITE_CONTENT_ADAPT_BRIGHTNESS	0x55
#define ILI9488_CMD_READ_CONTENT_ADAPT_BRIGHTNESS	0x56
#define ILI9488_CMD_WRITE_MIN_CAB_LEVEL			0x5E
#define ILI9488_CMD_READ_MIN_CAB_LEVEL			0x5F
#define ILI9488_CMD_READ_ABC_SELF_DIAG_RES		0x68
#define ILI9488_CMD_READ_ID1				    0xDA
#define ILI9488_CMD_READ_ID2				    0xDB
#define ILI9488_CMD_READ_ID3				    0xDC

/* Level 2 Commands (from the display Datasheet) */
#define ILI9488_CMD_INTERFACE_MODE_CONTROL		0xB0
#define ILI9488_CMD_FRAME_RATE_CONTROL_NORMAL		0xB1
#define ILI9488_CMD_FRAME_RATE_CONTROL_IDLE_8COLOR	0xB2
#define ILI9488_CMD_FRAME_RATE_CONTROL_PARTIAL		0xB3
#define ILI9488_CMD_DISPLAY_INVERSION_CONTROL		0xB4
#define ILI9488_CMD_BLANKING_PORCH_CONTROL		0xB5
#define ILI9488_CMD_DISPLAY_FUNCTION_CONTROL	0xB6
#define ILI9488_CMD_ENTRY_MODE_SET			    0xB7
#define ILI9488_CMD_BACKLIGHT_CONTROL_1			0xB9
#define ILI9488_CMD_BACKLIGHT_CONTROL_2			0xBA
#define ILI9488_CMD_HS_LANES_CONTROL			0xBE
#define ILI9488_CMD_POWER_CONTROL_1			    0xC0
#define ILI9488_CMD_POWER_CONTROL_2			    0xC1
#define ILI9488_CMD_POWER_CONTROL_NORMAL_3		0xC2
#define ILI9488_CMD_POWER_CONTROL_IDEL_4		0xC3
#define ILI9488_CMD_POWER_CONTROL_PARTIAL_5		0xC4
#define ILI9488_CMD_VCOM_CONTROL_1			    0xC5
#define ILI9488_CMD_CABC_CONTROL_1			    0xC6
#define ILI9488_CMD_CABC_CONTROL_2			    0xC8
#define ILI9488_CMD_CABC_CONTROL_3			    0xC9
#define ILI9488_CMD_CABC_CONTROL_4			    0xCA
#define ILI9488_CMD_CABC_CONTROL_5			    0xCB
#define ILI9488_CMD_CABC_CONTROL_6			    0xCC
#define ILI9488_CMD_CABC_CONTROL_7			    0xCD
#define ILI9488_CMD_CABC_CONTROL_8			    0xCE
#define ILI9488_CMD_CABC_CONTROL_9			    0xCF
#define ILI9488_CMD_NVMEM_WRITE				    0xD0
#define ILI9488_CMD_NVMEM_PROTECTION_KEY		0xD1
#define ILI9488_CMD_NVMEM_STATUS_READ			0xD2
#define ILI9488_CMD_READ_ID4				    0xD3
#define ILI9488_CMD_ADJUST_CONTROL_1			0xD7
#define ILI9488_CMD_READ_ID_VERSION			    0xD8
#define ILI9488_CMD_POSITIVE_GAMMA_CORRECTION   0xE0
#define ILI9488_CMD_NEGATIVE_GAMMA_CORRECTION	0xE1
#define ILI9488_CMD_DIGITAL_GAMMA_CONTROL_1		0xE2
#define ILI9488_CMD_DIGITAL_GAMMA_CONTROL_2		0xE3
#define ILI9488_CMD_SET_IMAGE_FUNCTION			0xE9
#define ILI9488_CMD_ADJUST_CONTROL_2			0xF2
#define ILI9488_CMD_ADJUST_CONTROL_3			0xF7
#define ILI9488_CMD_ADJUST_CONTROL_4			0xF8
#define ILI9488_CMD_ADJUST_CONTROL_5			0xF9
#define ILI9488_CMD_SPI_READ_SETTINGS			0xFB
#define ILI9488_CMD_ADJUST_CONTROL_6			0xFC
#define ILI9488_CMD_ADJUST_CONTROL_7			0xFF


/*
 * ILI9488 pixel format flags
 *
 * DBI is the pixel format of CPU interface
 */
#define ILI9488_DBI_BPP16               0x05    /* 16 bits / pixel */
#define ILI9488_DBI_BPP18               0x06    /* 18 bits / pixel */
#define ILI9488_DBI_BPP24               0x07    /* 24 bits / pixel */

/*
 * DPI is the pixel format select of RGB interface
 */
#define ILI9488_DPI_BPP16               0x50    /* 16 bits / pixel */
#define ILI9488_DPI_BPP18               0x60    /* 18 bits / pixel */
#define ILI9488_DPI_BPP24               0x70    /* 24 bits / pixel */



#define panel_reset(val) sunxi_lcd_gpio_set_value(sel, 0, val)
#define power_en(val)  sunxi_lcd_gpio_set_value(sel, 1, val)

#define jpy_dsi_write_15 sunxi_lcd_dsi_gen_write_15para
#define jpy_dsi_write_3  sunxi_lcd_dsi_gen_write_3para
#define jpy_dsi_write_2  sunxi_lcd_dsi_gen_write_2para
#define jpy_dsi_write_1  sunxi_lcd_dsi_gen_write_1para
#define jpy_dsi_write_4  sunxi_lcd_dsi_gen_write_4para

#define jpy_delay  sunxi_lcd_delay_ms
#define jpy_dsi_clk_enable sunxi_lcd_dsi_clk_enable

static void LCD_cfg_panel_info(struct panel_extend_para * info)
{
	u32 i = 0, j=0;
	u32 items;
	u8 lcd_gamma_tbl[][2] =
	{
		//{input value, corrected value}
		{0, 0},
		{15, 15},
		{30, 30},
		{45, 45},
		{60, 60},
		{75, 75},
		{90, 90},
		{105, 105},
		{120, 120},
		{135, 135},
		{150, 150},
		{165, 165},
		{180, 180},
		{195, 195},
		{210, 210},
		{225, 225},
		{240, 240},
		{255, 255},
	};

	u32 lcd_cmap_tbl[2][3][4] = {
	{
		{LCD_CMAP_G0,LCD_CMAP_B1,LCD_CMAP_G2,LCD_CMAP_B3},
		{LCD_CMAP_B0,LCD_CMAP_R1,LCD_CMAP_B2,LCD_CMAP_R3},
		{LCD_CMAP_R0,LCD_CMAP_G1,LCD_CMAP_R2,LCD_CMAP_G3},
		},
		{
		{LCD_CMAP_B3,LCD_CMAP_G2,LCD_CMAP_B1,LCD_CMAP_G0},
		{LCD_CMAP_R3,LCD_CMAP_B2,LCD_CMAP_R1,LCD_CMAP_B0},
		{LCD_CMAP_G3,LCD_CMAP_R2,LCD_CMAP_G1,LCD_CMAP_R0},
		},
	};

	items = sizeof(lcd_gamma_tbl)/2;
	for(i=0; i<items-1; i++) {
		u32 num = lcd_gamma_tbl[i+1][0] - lcd_gamma_tbl[i][0];

		for(j=0; j<num; j++) {
			u32 value = 0;

			value = lcd_gamma_tbl[i][1] + ((lcd_gamma_tbl[i+1][1] - lcd_gamma_tbl[i][1]) * j)/num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] = (value<<16) + (value<<8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items-1][1]<<16) + (lcd_gamma_tbl[items-1][1]<<8) + lcd_gamma_tbl[items-1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));

}

static s32 LCD_open_flow(u32 sel)
{        
	pr_info("[JPY]LCD_open_flow\n");
	
	LCD_OPEN_FUNC(sel, LCD_power_on, 200);   //open lcd power, and delay 50ms
	LCD_OPEN_FUNC(sel, LCD_panel_init, 200);   //open lcd power, than delay 200ms
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable,500);  	
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);     //open lcd backlight, and delay 0ms

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	pr_info("[JPY]LCD_close_flow\n");
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 0);       //close lcd backlight, and delay 0ms
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);         //close lcd controller, and delay 0ms
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,	20);   //open lcd power, than delay 200ms
	LCD_CLOSE_FUNC(sel, LCD_power_off, 50);   //close lcd power, and delay 500ms

	return 0;
}

static void LCD_power_on(u32 sel)
{
	pr_info("[JPY]LCD_power_on\n");
	sunxi_lcd_pin_cfg(sel, 1);
	power_en(0);
	sunxi_lcd_power_enable(sel, 0);//config lcd_power pin to open lcd power
	sunxi_lcd_delay_ms(10);
	sunxi_lcd_power_enable(sel, 1);//config lcd_power pin to open lcd power0
	sunxi_lcd_delay_ms(10);
	power_en(1);
	sunxi_lcd_delay_ms(50);
	panel_reset(1);
	sunxi_lcd_delay_ms(10);
	panel_reset(0);
	sunxi_lcd_delay_ms(30);
	panel_reset(1);
}

static void LCD_power_off(u32 sel)
{
	pr_info("[JPY]LCD_power_off\n");
	sunxi_lcd_pin_cfg(sel, 0);
	power_en(0);
	sunxi_lcd_delay_ms(10);
	panel_reset(0);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 2);//config lcd_power pin to close lcd power2
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 1);//config lcd_power pin to close lcd power1
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 0);//config lcd_power pin to close lcd power
}

static void LCD_bl_open(u32 sel)
{
	pr_info("[JPY]LCD_bl_open\n");
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_delay_ms(10);
	sunxi_lcd_backlight_enable(sel);//config lcd_bl_en pin to open lcd backlight
}

static void LCD_bl_close(u32 sel)
{
	pr_info("[JPY]LCD_bl_close\n");
	sunxi_lcd_backlight_disable(sel);//config lcd_bl_en pin to close lcd backlight
	sunxi_lcd_pwm_disable(sel);
}

static void LCD_panel_init(u32 sel)
{
	pr_info("[JPY]LCD_panel_init Start \n");

	//ILI9488_CMD_POSITIVE_GAMMA_CORRECTION
	// manufactory
	jpy_dsi_write_15(sel, 0xE0, 0x00, 0x07, 0x0B, 0x03, 0x0F, 0x05, 0x30, 0x56, 0x47, 0x04, 0x0B, 0x0A, 0x2D, 0x37, 0x0F);
	//jpy_dsi_write_15(sel, 0xE0, 0x00, 0x04, 0x0E, 0x08, 0x17, 0x0A, 0x40, 0x79, 0x4D, 0x07, 0x0E, 0x0A, 0x1A, 0x1D, 0x0F);//0x20
	jpy_delay(10);

	//ILI9488_CMD_NEGATIVE_GAMMA_CORRECTION
	// manufactory
	jpy_dsi_write_15(sel, 0xE1, 0x00, 0x0E, 0x13, 0x04, 0x11, 0x07, 0x39, 0x45, 0x50, 0x07, 0x10, 0x0D, 0x32, 0x36, 0x0F); 
	//jpy_dsi_write_15(sel, 0xE1, 0x00, 0x1B, 0x1F, 0x02, 0x10, 0x05, 0x32, 0x34, 0x43, 0x02, 0x0A, 0x09, 0x33, 0x37, 0x0F);//0xE0
	jpy_delay(10);

	//ILI9488_CMD_POWER_CONTROL_1
	// manufactory
	jpy_dsi_write_2(sel, 0xC0, 0x0F, 0x0F); 
	jpy_delay(10);

	//ILI9488_CMD_POWER_CONTROL_2
	// manufactory
	jpy_dsi_write_1(sel, 0xC1, 0x47);
	//jpy_dsi_write_1(sel, 0xC1, 0x41);
	jpy_delay(10);

	//ILI9488_CMD_VCOM_CONTROL_1
	jpy_dsi_write_3(sel, 0xC5, 0x00, 0x4D, 0x80);
	//jpy_dsi_write_3(sel, 0xC5,0x00, 0x22, 0x80);
	jpy_delay(10);

	// ILI9488_CMD_MEMORY_ACCESS_CONTROL
	jpy_dsi_write_1(sel, 0x36, 0x48); 
	//jpy_dsi_write_1(sel, 0x36, 0x40); 
	// jpy_dsi_write_1(sel, 0x36, 0x00);
	jpy_delay(10);

	// ILI9488_CMD_COLMOD_PIXEL_FORMAT_SET
	//jpy_dsi_write_1(sel, 0x3A, 0x70); 
	//jpy_dsi_write_1(sel, 0x3A, 0x77); 
	//jpy_dsi_write_1(sel, 0x3A, 0x67); 
	jpy_dsi_write_1(sel, 0x3A, 0x66);
	jpy_delay(10);

	jpy_dsi_write_1(sel, 0x21, 0x00); 
	jpy_delay(10);

	//ILI9488_CMD_INTERFACE_MODE_CONTROL
	jpy_dsi_write_1(sel, 0xB0, 0x00); 
	jpy_delay(10);

	//ILI9488_CMD_FRAME_RATE_CONTROL_NORMAL
	// B0 -> 68FS, A0 -> 60FS
	jpy_dsi_write_1(sel, 0xB1, 0xA0);// 60fs
	jpy_delay(10);

	//ILI9488_CMD_DISPLAY_INVERSION_CONTROL
	jpy_dsi_write_1(sel, 0xB4, 0x02);
	jpy_delay(10);

	// ILI9488_CMD_BLANKING_PORCH_CONTROL
	// VFP = VBP = 20, HFP = HBP = 10
	jpy_dsi_write_4(sel, 0xB5, 0x14, 0x14, 0x0A, 0x0A);
	jpy_delay(10);

	//ILI9488_CMD_DISPLAY_FUNCTION_CONTROL
	jpy_dsi_write_3(sel, 0xB6, 0x02, 0x02, 0x3B);
	jpy_delay(10);

	//ILI9488_CMD_SET_IMAGE_FUNCTION
	jpy_dsi_write_1(sel, 0xE9, 0x00);
	jpy_delay(10);

	//ILI9488_CMD_ADJUST_CONTROL_3
	jpy_dsi_write_4(sel, 0xF7, 0xA9, 0x51, 0x2c, 0x82);
	jpy_delay(10);

	//ILI9488_CMD_COLUMN_ADDRESS_SET
	jpy_dsi_write_4(sel, 0x2A, 0x00, 0x00, 0x01, 0x3F);
	jpy_delay(10);

	//ILI9488_CMD_PAGE_ADDRESS_SET
	//jpy_dsi_write_4(sel, 0x2B, 0x00, 0x00, 0x00, 0xEF);
	jpy_dsi_write_4(sel, 0x2B, 0x00, 0x00, 0x01, 0xDF);
	jpy_delay(10);

	//ILI9488_CMD_SLEEP_OUT
	jpy_dsi_write_1(sel, 0x11, 0x00);
	jpy_delay(120);

	//ILI9488_CMD_DISPLAY_ON
	jpy_dsi_write_1(sel, 0x29, 0x00);
	jpy_delay(30);

	//ILI9488_CMD_MEMORY_WRITE
	jpy_dsi_write_1(sel, 0x2C, 0x00);
	jpy_delay(10);

	jpy_dsi_clk_enable(sel);

	pr_info("[JPY]LCD_panel_init End \n");
	return;
}


static void LCD_panel_exit(u32 sel)
{
	sunxi_lcd_dsi_clk_disable(sel);

	return ;
}

//sel: 0:lcd0; 1:lcd1
static s32 LCD_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

#if 0
/* sel: 0:lcd0; 1:lcd1 */
static s32 LCD_set_bright(u32 sel, u32 bright)
{
	pr_info("[JPY]LCD_set_bright start \n");
	//sunxi_lcd_dsi_clk_disable(sel);
	//jpy_dsi_write_1(sel, 0x51, bright);
	//sunxi_lcd_dsi_clk_enable(sel);
	pr_info("[JPY]LCD_set_bright end \n");
	return 0;
}
#endif
struct __lcd_panel JPY3563_MIPI_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "JPY3563_MIPI_20P",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
		/* .set_bright = LCD_set_bright, */
	},
};
