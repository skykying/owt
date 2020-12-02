#include "S101WV20_MIPI_RGB.h"


//ILI9881C+BOE 10.1寸 8001280 B4 TV101WXU-N90  Code_20191101

static void LCD_power_on(u32 sel);
static void LCD_power_off(u32 sel);
static void LCD_bl_open(u32 sel);
static void LCD_bl_close(u32 sel);

static void LCD_panel_init(u32 sel);
static void LCD_panel_exit(u32 sel);

#define Bist_mode	(1)

#define panel_reset(val) sunxi_lcd_gpio_set_value(sel, 0, val)
#define power_en(val)  sunxi_lcd_gpio_set_value(sel, 1, val)


#define boe_dsi_write_3  sunxi_lcd_dsi_gen_write_3para
#define boe_dsi_write_2  sunxi_lcd_dsi_gen_write_2para
#define boe_dsi_write_1  sunxi_lcd_dsi_gen_write_1para
#define boe_dsi_write_4  sunxi_lcd_dsi_gen_write_4para

#define boe_delay  sunxi_lcd_delay_ms
#define boe_dsi_clk_enable sunxi_lcd_dsi_clk_enable

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
	pr_info("[ILI9881C]LCD_open_flow\n");
	
	LCD_OPEN_FUNC(sel, LCD_power_on, 200);   //open lcd power, and delay 50ms
	LCD_OPEN_FUNC(sel, LCD_panel_init, 200);   //open lcd power, than delay 200ms
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable,500);  	
	LCD_OPEN_FUNC(sel, LCD_bl_open, 0);     //open lcd backlight, and delay 0ms

	return 0;
}

static s32 LCD_close_flow(u32 sel)
{
	pr_info("[ILI9881C]LCD_close_flow\n");
	LCD_CLOSE_FUNC(sel, LCD_bl_close, 0);       //close lcd backlight, and delay 0ms
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);         //close lcd controller, and delay 0ms
	LCD_CLOSE_FUNC(sel, LCD_panel_exit,	20);   //open lcd power, than delay 200ms
	LCD_CLOSE_FUNC(sel, LCD_power_off, 50);   //close lcd power, and delay 500ms

	return 0;
}

static void LCD_power_on(u32 sel)
{
	pr_info("[ILI9881C]LCD_power_on\n");
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
	sunxi_lcd_delay_ms(130);
	panel_reset(1);
}

static void LCD_power_off(u32 sel)
{
	pr_info("[ILI9881C]LCD_power_off\n");
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
	pr_info("[ILI9881C]LCD_bl_open\n");
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_delay_ms(10);
	sunxi_lcd_backlight_enable(sel);//config lcd_bl_en pin to open lcd backlight
}

static void LCD_bl_close(u32 sel)
{
	pr_info("[ILI9881C]LCD_bl_close\n");
	sunxi_lcd_backlight_disable(sel);//config lcd_bl_en pin to close lcd backlight
	sunxi_lcd_pwm_disable(sel);
}

static void LCD_panel_init(u32 id)
{

	//PAGE 3
	boe_dsi_write_3(id,0xFF,0x98,0x81,0x03);
	boe_delay(20);

	//GIP_1
	boe_dsi_write_1(id,0x01,0x00);
	boe_delay(5);
	boe_dsi_write_1(id,0x02,0x00); 
	boe_delay(5);     
	boe_dsi_write_1(id,0x03,0x73);
	boe_delay(5);
	boe_dsi_write_1(id,0x04,0x00);       
	boe_delay(5);  
	boe_dsi_write_1(id,0x05,0x00);    
	boe_delay(5);     
	boe_dsi_write_1(id,0x06,0x08);     
	boe_delay(5);    
	boe_dsi_write_1(id,0x07,0x00);  
	boe_delay(5);       
	boe_dsi_write_1(id,0x08,0x00); 
	boe_delay(5);        
	boe_dsi_write_1(id,0x09,0x00); 
	boe_delay(5);        
	boe_dsi_write_1(id,0x0A,0x01);     
	boe_delay(5);    
	boe_dsi_write_1(id,0x0B,0x01);
	boe_delay(5);         
	boe_dsi_write_1(id,0x0C,0x00);     // STV  DUTY     
	boe_delay(5);
	boe_dsi_write_1(id,0x0D,0x01);     // STV  DUTY    
	boe_delay(5);     
	boe_dsi_write_1(id,0x0E,0x01);   
	boe_delay(5);      
	boe_dsi_write_1(id,0x0F,0x00);  
	boe_delay(5);       
	boe_dsi_write_1(id,0x10,0x00);   
	boe_delay(5);      
	boe_dsi_write_1(id,0x11,0x00);   
	boe_delay(5);      
	boe_dsi_write_1(id,0x12,0x00);   
	boe_delay(5);      
	boe_dsi_write_1(id,0x13,0x1F);     // CLK DUTY        
	boe_delay(5);
	boe_dsi_write_1(id,0x14,0x1F);     // CLK DUTY
	boe_delay(5);
	boe_dsi_write_1(id,0x15,0x00);   
	boe_delay(5);      
	boe_dsi_write_1(id,0x16,0x00);  
	boe_delay(5);       
	boe_dsi_write_1(id,0x17,0x00);
	boe_delay(5);         
	boe_dsi_write_1(id,0x18,0x00);     
	boe_delay(5);    
	boe_dsi_write_1(id,0x19,0x00); 
	boe_delay(5);        
	boe_dsi_write_1(id,0x1A,0x00);       
	boe_delay(5);  
	boe_dsi_write_1(id,0x1B,0x00);   
	boe_delay(5);      
	boe_dsi_write_1(id,0x1C,0x00); 
	boe_delay(5);        
	boe_dsi_write_1(id,0x1D,0x00);   
	boe_delay(5);      
	boe_dsi_write_1(id,0x1E,0x40);  
	boe_delay(5);       
	boe_dsi_write_1(id,0x1F,0xC0);    
	boe_delay(5);     
	boe_dsi_write_1(id,0x20,0x06);    
	boe_delay(5);     
	boe_dsi_write_1(id,0x21,0x01);   
	boe_delay(5);      
	boe_dsi_write_1(id,0x22,0x06);
	boe_delay(5);      
	boe_dsi_write_1(id,0x23,0x01);   
	boe_delay(5); 
	boe_dsi_write_1(id,0x24,0x88);    
	boe_delay(5);     
	boe_dsi_write_1(id,0x25,0x88); 
	boe_delay(5);
	boe_dsi_write_1(id,0x26,0x00); 
	boe_delay(5);    
	boe_dsi_write_1(id,0x27,0x00);  
	boe_delay(5);   
	boe_dsi_write_1(id,0x28,0x3B); 
	boe_delay(5);   
	boe_dsi_write_1(id,0x29,0x03); 
	boe_delay(5);        
	boe_dsi_write_1(id,0x2A,0x00);   
	boe_delay(5);      
	boe_dsi_write_1(id,0x2B,0x00);   
	boe_delay(5);      
	boe_dsi_write_1(id,0x2C,0x00);    
	boe_delay(5);     
	boe_dsi_write_1(id,0x2D,0x00);       
	boe_delay(5);  
	boe_dsi_write_1(id,0x2E,0x00);  
	boe_delay(5);       
	boe_dsi_write_1(id,0x2F,0x00);      
	boe_delay(5);   
	boe_dsi_write_1(id,0x30,0x00); 
	boe_delay(5);        
	boe_dsi_write_1(id,0x31,0x00); 
	boe_delay(5);        
	boe_dsi_write_1(id,0x32,0x00);    
	boe_delay(5);     
	boe_dsi_write_1(id,0x33,0x00); 
	boe_delay(5);        
	boe_dsi_write_1(id,0x34,0x00);  // GPWR1/2 non overlap time 2.62us          
	boe_delay(5); 
	boe_dsi_write_1(id,0x35,0x00);     
	boe_delay(5);     
	boe_dsi_write_1(id,0x36,0x00);   
	boe_delay(5);        
	boe_dsi_write_1(id,0x37,0x00);        
	boe_delay(5);   
	boe_dsi_write_1(id,0x38,0x00);    
	boe_delay(5);       
	boe_dsi_write_1(id,0x39,0x00);    
	boe_delay(5);      
	boe_dsi_write_1(id,0x3A,0x00); 
	boe_delay(5);        
	boe_dsi_write_1(id,0x3B,0x00);    
	boe_delay(5);     
	boe_dsi_write_1(id,0x3C,0x00); 
	boe_delay(5);        
	boe_dsi_write_1(id,0x3D,0x00);      
	boe_delay(5);   
	boe_dsi_write_1(id,0x3E,0x00);
	boe_delay(5);         
	boe_dsi_write_1(id,0x3F,0x00); 
	boe_delay(5);        
	boe_dsi_write_1(id,0x40,0x00); 
	boe_delay(5);        
	boe_dsi_write_1(id,0x41,0x00);   
	boe_delay(5);      
	boe_dsi_write_1(id,0x42,0x00);   
	boe_delay(5);      
	boe_dsi_write_1(id,0x43,0x00);  
	boe_delay(5);       
	boe_dsi_write_1(id,0x44,0x00);
	boe_delay(5);

	//GIP_2           
	boe_dsi_write_1(id,0x50,0x01);  
	boe_delay(5);         
	boe_dsi_write_1(id,0x51,0x23);  
	boe_delay(5);         
	boe_dsi_write_1(id,0x52,0x45);
	boe_delay(5);           
	boe_dsi_write_1(id,0x53,0x67);  
	boe_delay(5);         
	boe_dsi_write_1(id,0x54,0x89);
	boe_delay(5);           
	boe_dsi_write_1(id,0x55,0xAB);  
	boe_delay(5);         
	boe_dsi_write_1(id,0x56,0x01);   
	boe_delay(5);        
	boe_dsi_write_1(id,0x57,0x23);
	boe_delay(5);           
	boe_dsi_write_1(id,0x58,0x45); 
	boe_delay(5);          
	boe_dsi_write_1(id,0x59,0x67);  
	boe_delay(5);         
	boe_dsi_write_1(id,0x5A,0x89); 
	boe_delay(5);          
	boe_dsi_write_1(id,0x5B,0xAB);   
	boe_delay(5);        
	boe_dsi_write_1(id,0x5C,0xCD);   
	boe_delay(5);        
	boe_dsi_write_1(id,0x5D,0xEF);     
	boe_delay(5);

	//GIP_3  
	boe_dsi_write_1(id,0x5E,0x00);
	boe_delay(5);           
	boe_dsi_write_1(id,0x5F,0x01); 
	boe_delay(5);          
	boe_dsi_write_1(id,0x60,0x01);
	boe_delay(5);           
	boe_dsi_write_1(id,0x61,0x06);  
	boe_delay(5);         
	boe_dsi_write_1(id,0x62,0x06);       
	boe_delay(5);    
	boe_dsi_write_1(id,0x63,0x07);        
	boe_delay(5);   
	boe_dsi_write_1(id,0x64,0x07); 
	boe_delay(5);          
	boe_dsi_write_1(id,0x65,0x00);   
	boe_delay(5);        
	boe_dsi_write_1(id,0x66,0x00);   
	boe_delay(5);        
	boe_dsi_write_1(id,0x67,0x02);    
	boe_delay(5);       
	boe_dsi_write_1(id,0x68,0x02);  
	boe_delay(5);         
	boe_dsi_write_1(id,0x69,0x05); 
	boe_delay(5);          
	boe_dsi_write_1(id,0x6A,0x05); 
	boe_delay(5);          
	boe_dsi_write_1(id,0x6B,0x02);  
	boe_delay(5);         
	boe_dsi_write_1(id,0x6C,0x0D);
	boe_delay(5);           
	boe_dsi_write_1(id,0x6D,0x0D);       
	boe_delay(5);    
	boe_dsi_write_1(id,0x6E,0x0C);     
	boe_delay(5);      
	boe_dsi_write_1(id,0x6F,0x0C); 
	boe_delay(5);          
	boe_dsi_write_1(id,0x70,0x0F);    
	boe_delay(5);       
	boe_dsi_write_1(id,0x71,0x0F);   
	boe_delay(5);        
	boe_dsi_write_1(id,0x72,0x0E);         
	boe_delay(5);  
	boe_dsi_write_1(id,0x73,0x0E);      
	boe_delay(5);     
	boe_dsi_write_1(id,0x74,0x02);  
	boe_delay(5);         
	boe_dsi_write_1(id,0x75,0x01); 
	boe_delay(5);          
	boe_dsi_write_1(id,0x76,0x01);  
	boe_delay(5);         
	boe_dsi_write_1(id,0x77,0x06);    
	boe_delay(5);       
	boe_dsi_write_1(id,0x78,0x06);        
	boe_delay(5);   
	boe_dsi_write_1(id,0x79,0x07);    
	boe_delay(5);       
	boe_dsi_write_1(id,0x7A,0x07);   
	boe_delay(5);        
	boe_dsi_write_1(id,0x7B,0x00);     
	boe_delay(5);      
	boe_dsi_write_1(id,0x7C,0x00);         
	boe_delay(5);  
	boe_dsi_write_1(id,0x7D,0x02);  
	boe_delay(5);         
	boe_dsi_write_1(id,0x7E,0x02);   
	boe_delay(5);        
	boe_dsi_write_1(id,0x7F,0x05);     
	boe_delay(5);      
	boe_dsi_write_1(id,0x80,0x05);     
	boe_delay(5);      
	boe_dsi_write_1(id,0x81,0x02); 
	boe_delay(5);          
	boe_dsi_write_1(id,0x82,0x0D);   
	boe_delay(5);        
	boe_dsi_write_1(id,0x83,0x0D);         
	boe_delay(5);  
	boe_dsi_write_1(id,0x84,0x0C);     
	boe_delay(5);      
	boe_dsi_write_1(id,0x85,0x0C);    
	boe_delay(5);       
	boe_dsi_write_1(id,0x86,0x0F);  
	boe_delay(5);         
	boe_dsi_write_1(id,0x87,0x0F);   
	boe_delay(5);        
	boe_dsi_write_1(id,0x88,0x0E); 
	boe_delay(5);          
	boe_dsi_write_1(id,0x89,0x0E); 
	boe_delay(5);          
	boe_dsi_write_1(id,0x8A,0x02);
	boe_delay(5);

	//Page 4 command;           
	boe_dsi_write_3(id,0xFF,0x98,0x81,0x04); 
	boe_delay(5);       
	 
	//boe_dsi_write_1(id,0x3B,0x01,0xC0);     	// ILI4003D sel
	boe_dsi_write_1(id,0x6C,0x15);        		//Set VCORE voltage =1.5V  
	boe_delay(5);         
	boe_dsi_write_1(id,0x6E,0x2A);        		//di_pwr_reg=0 for power mode 2A //VGH clamp 18V 
	boe_delay(5);          
	boe_dsi_write_1(id,0x6F,0x33);    			//45 //pumping ratio VGH=5x VGL=-3x    
	boe_delay(5);                 
	boe_dsi_write_1(id,0x8D,0x1B);        		//VGL clamp -10V
	boe_delay(5);           
	boe_dsi_write_1(id,0x87,0xBA);        		//ESD  
	boe_delay(5);
	boe_dsi_write_1(id,0x3A,0x24);        		//POWER SAVING  
	boe_delay(5);                   
	boe_dsi_write_1(id,0x26,0x76); 
	boe_delay(5);          
	boe_dsi_write_1(id,0xB2,0xD1);  
	boe_delay(5);         
	   
#if Bist_mode
	boe_dsi_write_1(id,0x2D,0x80);           
	boe_delay(10);
	boe_dsi_write_1(id,0x2F,0x01);           
	boe_delay(10);
#endif

	 // Page 1 command             
	boe_dsi_write_3(id,0xFF,0x98,0x81,0x01);        
	boe_delay(5);   
	boe_dsi_write_1(id,0x22,0x0A);        		//BGR,0x SS 
	boe_delay(5);
	//HBP=HFP=100 HSA=33 HT=1280+100+100+33 = 1513
	//VBP=30 VFP=20 VSA=2 VT=800+30+20+2 = 852
	boe_dsi_write_1(id,0x25,0x14);        		//VFP[7:0]
	boe_delay(5);
	boe_dsi_write_1(id,0x26,0x1E);        		//VBP[7:0]
	boe_delay(5);
	boe_dsi_write_1(id,0x27,0x64);        		//HBP[7:0]
	boe_delay(5);
	boe_dsi_write_1(id,0x28,0x00);        		//HBP[9:8]  
	boe_delay(5);

	// boe_dsi_write_1(id,0x25,0x14);        	//VFP[7:0]
	// boe_dsi_write_1(id,0x26,0x14);        	//VBP[7:0]
	// boe_dsi_write_1(id,0x27,0x05);        	//HBP[7:0]
	// boe_dsi_write_1(id,0x28,0x00);        	//HBP[9:8]  
	boe_dsi_write_1(id,0x31,0x00);        		//Zigzag type3 inversion        
	boe_delay(5);   
	//boe_dsi_write_1(id,0x40,0x01,0x53);       // ILI4003D sel          
	boe_dsi_write_1(id,0x43,0x66);         
	boe_delay(5);        
	boe_dsi_write_1(id,0x53,0x42);   
	boe_delay(5);               
	boe_dsi_write_1(id,0x50,0x87);  
	boe_delay(5);                  
	boe_dsi_write_1(id,0x51,0x82);  
	boe_delay(5);                  
	boe_dsi_write_1(id,0x60,0x15);  
	boe_delay(5);
	boe_dsi_write_1(id,0x61,0x01); 
	boe_delay(5);
	boe_dsi_write_1(id,0x62,0x0C); 
	boe_delay(5);
	boe_dsi_write_1(id,0x63,0x00);
	boe_delay(5);         

	// Gamma P      
	boe_dsi_write_1(id,0xA0,0x00);    
	boe_delay(5);
	boe_dsi_write_1(id,0xA1,0x13);        //VP251   
	boe_delay(5);      
	boe_dsi_write_1(id,0xA2,0x23);        //VP247 
	boe_delay(5);
	boe_dsi_write_1(id,0xA3,0x14);        //VP243   
	boe_delay(5);      
	boe_dsi_write_1(id,0xA4,0x16);        //VP239  
	boe_delay(5);       
	boe_dsi_write_1(id,0xA5,0x29);        //VP231  
	boe_delay(5);       
	boe_dsi_write_1(id,0xA6,0x1E);        //VP219   
	boe_delay(5);      
	boe_dsi_write_1(id,0xA7,0x1D);        //VP203    
	boe_delay(5);     
	boe_dsi_write_1(id,0xA8,0x86);        //VP175  
	boe_delay(5);       
	boe_dsi_write_1(id,0xA9,0x1E);        //VP144       
	boe_delay(5);  
	boe_dsi_write_1(id,0xAA,0x29);        //VP111   
	boe_delay(5);      
	boe_dsi_write_1(id,0xAB,0x74);        //VP80  
	boe_delay(5);        
	boe_dsi_write_1(id,0xAC,0x19);        //VP52  
	boe_delay(5);        
	boe_dsi_write_1(id,0xAD,0x17);        //VP36    
	boe_delay(5);      
	boe_dsi_write_1(id,0xAE,0x4B);        //VP24   
	boe_delay(5);       
	boe_dsi_write_1(id,0xAF,0x20);        //VP16        
	boe_delay(5);  
	boe_dsi_write_1(id,0xB0,0x26);        //VP12  
	boe_delay(5);        
	boe_dsi_write_1(id,0xB1,0x4C);        //VP8   
	boe_delay(5);        
	boe_dsi_write_1(id,0xB2,0x5D);        //VP4     
	boe_delay(5);      
	boe_dsi_write_1(id,0xB3,0x3F);        //VP0  
	boe_delay(5);        

	// Gamma N      
	boe_dsi_write_1(id,0xC0,0x00);        //VN255 GAMMA N    
	boe_delay(5);       
	boe_dsi_write_1(id,0xC1,0x13);        //VN251   
	boe_delay(5);      
	boe_dsi_write_1(id,0xC2,0x23);        //VN247       
	boe_delay(5);  
	boe_dsi_write_1(id,0xC3,0x14);        //VN243
	boe_delay(5);         
	boe_dsi_write_1(id,0xC4,0x16);        //VN239  
	boe_delay(5);       
	boe_dsi_write_1(id,0xC5,0x29);        //VN231 
	boe_delay(5);        
	boe_dsi_write_1(id,0xC6,0x1E);        //VN219  
	boe_delay(5);       
	boe_dsi_write_1(id,0xC7,0x1D);        //VN203
	boe_delay(5);         
	boe_dsi_write_1(id,0xC8,0x86);        //VN175 
	boe_delay(5);        
	boe_dsi_write_1(id,0xC9,0x1E);        //VN144    
	boe_delay(5);     
	boe_dsi_write_1(id,0xCA,0x29);        //VN111    
	boe_delay(5);     
	boe_dsi_write_1(id,0xCB,0x74);        //VN80      
	boe_delay(5);    
	boe_dsi_write_1(id,0xCC,0x19);        //VN52        
	boe_delay(5);  
	boe_dsi_write_1(id,0xCD,0x17);        //VN36   
	boe_delay(5);       
	boe_dsi_write_1(id,0xCE,0x4B);        //VN24     
	boe_delay(5);     
	boe_dsi_write_1(id,0xCF,0x20);        //VN16     
	boe_delay(5);     
	boe_dsi_write_1(id,0xD0,0x26);        //VN12 
	boe_delay(5);         
	boe_dsi_write_1(id,0xD1,0x4C);        //VN8  
	boe_delay(5);         
	boe_dsi_write_1(id,0xD2,0x5D);        //VN4       
	boe_delay(5);    
	boe_dsi_write_1(id,0xD3,0x3F);        //VN0
	boe_delay(5);

	// Page 0 command           
	boe_dsi_write_3(id,0xFF,0x98,0x81,0x00);
	// boe_delay(5);
	// boe_dsi_write_1(id,0x3A,0xA0);  	  // TE Off
	boe_delay(5);
	boe_dsi_write_1(id,0x35,0x00);  		// TE On
	boe_delay(5);
	boe_dsi_write_1(id,0x11,0x00);  		// Sleep Out          
	boe_delay(120);

	boe_dsi_write_1(id,0x29,0x00);  		//  Display On 
	boe_delay(20);

	boe_dsi_clk_enable(id);

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

struct __lcd_panel S101WV20_MIPI_RGB_panel = {
	/* panel driver name, must mach the name of lcd_drv_name in sys_config.fex */
	.name = "S101WV20_MIPI_RGB",
	.func = {
		.cfg_panel_info = LCD_cfg_panel_info,
		.cfg_open_flow = LCD_open_flow,
		.cfg_close_flow = LCD_close_flow,
		.lcd_user_defined_func = LCD_user_defined_func,
	},
};


#undef boe_dsi_write_3
#undef boe_dsi_write_2  
#undef boe_dsi_write_1  
#undef boe_dsi_write_4

#undef boe_delay
#undef boe_dsi_clk_enable
