#if defined(BUILD_LK)
#include <string.h>
#else
#include <linux/string.h>
#endif


#if defined(BUILD_LK)
#include "cust_gpio_usage.h"
#include <platform/mt_gpio.h>
#endif

#include "lcm_drv.h"

// ---------------------------------------------------------------------------
//  Local Constants
// ---------------------------------------------------------------------------

#define FRAME_WIDTH  										(480)
#define FRAME_HEIGHT 										(800)
#define REGFLAG_DELAY             							0XFE
#define REGFLAG_END_OF_TABLE      							0xFF   // END OF REGISTERS MARKER
#define LCM_DSI_CMD_MODE									0

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

//#define LCM_ESD_DEBUG

// ---------------------------------------------------------------------------
//  Local Variables
// ---------------------------------------------------------------------------

static LCM_UTIL_FUNCS lcm_util = {0};

#define SET_RESET_PIN(v)    								(lcm_util.set_reset_pin((v)))
#define UDELAY(n) 											(lcm_util.udelay(n))
#define MDELAY(n) 											(lcm_util.mdelay(n))

// ---------------------------------------------------------------------------
//  Local Functions
// ---------------------------------------------------------------------------
#define dsi_set_cmdq_V2(cmd, count, ppara, force_update)	lcm_util.dsi_set_cmdq_V2(cmd, count, ppara, force_update)
#define dsi_set_cmdq(pdata, queue_size, force_update)		lcm_util.dsi_set_cmdq(pdata, queue_size, force_update)
#define wrtie_cmd(cmd)										lcm_util.dsi_write_cmd(cmd)
#define write_regs(addr, pdata, byte_nums)					lcm_util.dsi_write_regs(addr, pdata, byte_nums)
#define read_reg(cmd)										lcm_util.dsi_dcs_read_lcm_reg(cmd)
#define read_reg_v2(cmd, buffer, buffer_size)   			lcm_util.dsi_dcs_read_lcm_reg_v2(cmd, buffer, buffer_size)

struct LCM_setting_table {
	unsigned char cmd;
	unsigned char count;
	unsigned char para_list[64];
};

/*Begin ersen.shang remove some delays for task 117114 reference pixi3-4*/
static struct LCM_setting_table lcm_initialization_setting_hx8379c_tdt[] = {

	/*
	Note :

	Data ID will depends on the following rule.

	count of parameters > 1	=> Data ID = 0x39
	count of parameters = 1	=> Data ID = 0x15
	count of parameters = 0	=> Data ID = 0x05

	Structure Format :

	{DCS command, count of parameters, {parameter list}}
	{REGFLAG_DELAY, milliseconds of time, {}},

	...

	Setting ending by predefined flag

	{REGFLAG_END_OF_TABLE, 0x00, {}}
	*/

	{0xB9,       3,    	{0xFF, 0x83, 0x79}},
	//{REGFLAG_DELAY, 5, 	{}},
	{0xB1,      20,    	{0x44,0x1a,0x1a,0x31,0x31,0x50,0xD0,0xEA,0x62,0x80,
                		 0x38,0x38,0xF8,0x32,0x32,0x22,0x00,0x80,0x30,0x00}},
	//{REGFLAG_DELAY, 5, 	{}},
	{0xB2,       9,    	{0x80,0x3C,0x0B,0x05,0x00,0x50,0x11,0x42,0x1D}},
//	{REGFLAG_DELAY, 5, 	{}},
	{0xB4,       10,   	{0x0A,0x6A,0x0A,0x6A,0x0A,0x6A,0x22,0x80,0x22,0x80}},
//	{REGFLAG_DELAY, 5, 	{}},
	{0xCC,      1,     	{0x02}},
	//{REGFLAG_DELAY, 5, 	{}},
	{0xD2,      1,     	{0x11}},
//	{REGFLAG_DELAY, 5, 	{}},
	{0xD3,       29,   	{0x00,0x07,0x00,0x00,0x00,0x07,0x00,0x32,0x10,0x03,    //by TDT
                0x00,0x03,0x03,0x2A,0x03,0x2A,0x00,0x08,0x00,0x08,
                0x45,0x44,0x08,0x08,0x37,0x08,0x08,0x37,0x09}},
//	{REGFLAG_DELAY, 5, 	{}},
	{0xD5,       32,   	{0x18,0x18,0x19,0x19,0x18,0x18,0x20,0x21,0x24,0x25,
                		 0x18,0x18,0x18,0x18,0x00,0x01,0x02,0x03,0x04,0x05,
		                 0x06,0x07,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
         		         0x18,0x18}},
	//{REGFLAG_DELAY, 5, 	{}},
	{0xD6,       32,   	{0x18,0x18,0x18,0x18,0x19,0x19,0x25,0x24,0x21,0x20,
                 		 0x18,0x18,0x18,0x18,0x07,0x06,0x05,0x04,0x03,0x02,
                 		 0x01,0x00,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
                 		 0x18,0x18}},
	//{REGFLAG_DELAY, 5, 	{}},
	{0xE0,      42,    	{0x00,0x02,0x04,0x07,0x08,0x3e,0x17,0x2c,0x04,0x08,
                 		 0x0b,0x16,0x12,0x16,0x18,0x16,0x16,0x08,0x12,0x13,
                 		 0x18,0x00,0x02,0x04,0x07,0x08,0x3e,0x17,0x2c,0x04,
                 		 0x08,0x0b,0x16,0x12,0x16,0x18,0x16,0x16,0x08,0x12,
                 		 0x13,0x18}},
	//{REGFLAG_DELAY, 5, 	{}},
	{0xB6,       2,    	{0x4f,0x4f}},
	//{REGFLAG_DELAY, 5, 	{}},
	{0x35,       1,    	{0x00}},
	{REGFLAG_DELAY, 5, 	{}},
	{0x11,       1,    	{0x00}},
	{REGFLAG_DELAY, 150,{}},
	{0x29,       1,     {0x00}},
	{REGFLAG_DELAY, 20, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
/*End   ersen.shang remove some delays for task 117114 reference pixi3-4*/

#if 0
static struct LCM_setting_table lcm_set_window[] = {
	{0x2A,	4,	{0x00, 0x00, (FRAME_WIDTH>>8), (FRAME_WIDTH&0xFF)}},
	{0x2B,	4,	{0x00, 0x00, (FRAME_HEIGHT>>8), (FRAME_HEIGHT&0xFF)}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif

/*Begin ersen.shang remove some delays for task 117114 reference pixi3-4*/
#if 0
static struct LCM_setting_table lcm_sleep_out_setting_hx8379c_tdt[] = {
	// Sleep Out
	{0x11, 1, {0x00}},
	{REGFLAG_DELAY, 150, {}},

	// Display ON
	{0x29, 1, {0x00}},
	{REGFLAG_DELAY, 50, {}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif
/*End   ersen.shang remove some delays for task 117114 reference pixi3-4*/

static struct LCM_setting_table lcm_deep_sleep_mode_in_setting_hx8379c_tdt[] = {
	// Display off sequence
	{0x28, 1, {0x00}},

	// Sleep Mode On
	{0x10, 1, {0x00}},

	{REGFLAG_END_OF_TABLE, 0x00, {}}
};

#if 0
static struct LCM_setting_table lcm_backlight_level_setting[] = {
	{0x51, 1, {0xFF}},
	{REGFLAG_END_OF_TABLE, 0x00, {}}
};
#endif

static void push_table(struct LCM_setting_table *table, unsigned int count, unsigned char force_update)
{
	unsigned int i;

	for(i = 0; i < count; i++) {

		unsigned cmd;
		cmd = table[i].cmd;

		switch (cmd) {

		case REGFLAG_DELAY :
			MDELAY(table[i].count);
			break;

		case REGFLAG_END_OF_TABLE :
			break;				
			
		default:
			dsi_set_cmdq_V2(cmd, table[i].count, table[i].para_list, force_update);
		}
	}

}

// ---------------------------------------------------------------------------
//  LCM Driver Implementations
// ---------------------------------------------------------------------------
static void lcm_set_util_funcs(const LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util, util, sizeof(LCM_UTIL_FUNCS));
}

static void lcm_get_params(LCM_PARAMS *params)
{
	memset(params, 0, sizeof(LCM_PARAMS));

	params->type   = LCM_TYPE_DSI;

	params->width  = FRAME_WIDTH;
	params->height = FRAME_HEIGHT;

	//enable tearing-free
	//params->dbi.te_mode 				= LCM_DBI_TE_MODE_VSYNC_ONLY;
	//params->dbi.te_edge_polarity		= LCM_POLARITY_RISING;
	params->dbi.te_mode 				= LCM_DBI_TE_MODE_DISABLED;

#if (LCM_DSI_CMD_MODE)
	params->dsi.mode   = CMD_MODE;
#else
	params->dsi.mode   = SYNC_PULSE_VDO_MODE;
#endif

	// DSI
	/* Command mode setting */
	params->dsi.LANE_NUM				= LCM_TWO_LANE;

	//The following defined the fomat for data coming from LCD engine.
	params->dsi.data_format.color_order = LCM_COLOR_ORDER_RGB;
	params->dsi.data_format.trans_seq   = LCM_DSI_TRANS_SEQ_MSB_FIRST;
	params->dsi.data_format.padding     = LCM_DSI_PADDING_ON_LSB;
	params->dsi.data_format.format      = LCM_DSI_FORMAT_RGB888;

	/*Begin ersen.shang add for esd check*/
	params->dsi.esd_check_enable = 1;
	params->dsi.customization_esd_check_enable = 1 ;
	params->dsi.lcm_esd_check_table[0].cmd          = 0x09;
	params->dsi.lcm_esd_check_table[0].count        = 2;
	params->dsi.lcm_esd_check_table[0].para_list[0] = 0x80;
	params->dsi.lcm_esd_check_table[0].para_list[1] = 0x73;
	/*End   ersen.shang add for esd check*/

	// Highly depends on LCD driver capability.
	// Not support in MT6573
	params->dsi.packet_size=256;

	// Video mode setting		
	params->dsi.intermediat_buffer_num = 2;

	params->dsi.PS=LCM_PACKED_PS_24BIT_RGB888;
	params->dsi.word_count=480*3;

	params->dsi.vertical_sync_active				= 6; 
	params->dsi.vertical_backporch					= 5;
	params->dsi.vertical_frontporch					= 8;
	params->dsi.vertical_active_line				= FRAME_HEIGHT; 

	params->dsi.horizontal_sync_active				= 80;
	params->dsi.horizontal_backporch				= 70; //modify by xin_liu for frame rate set to 60. 
	params->dsi.horizontal_frontporch				= 70; //modify by xin_liu for frame rate set to 60. 
	params->dsi.horizontal_blanking_pixel			= 80;
	params->dsi.horizontal_active_pixel				= FRAME_WIDTH;

	params->dsi.PLL_CLOCK=191; //modify by xin_liu for improving RF desense. 
	params->dsi.HS_TRAIL=8;
	params->dsi.ssc_disable = 0;
	params->dsi.ssc_range   = 5;
	params->dsi.cont_clock = TRUE;
	//note: for Video mode
	//fps = data_rate * Lane_Number /( (Vsyn+VBP+VFP+Height)*(Hsyn+HBP+HFP+Width)*BPP)
	//data_rate = CLOCK_rate * 2;

}

static void lcm_init(void)
{
	SET_RESET_PIN(1);
	MDELAY(10);
	SET_RESET_PIN(0);
	MDELAY(10);
	SET_RESET_PIN(1);
	MDELAY(120);
	push_table(lcm_initialization_setting_hx8379c_tdt, sizeof(lcm_initialization_setting_hx8379c_tdt) / sizeof(struct LCM_setting_table), 1);
}

static void lcm_suspend(void)
{
	push_table(lcm_deep_sleep_mode_in_setting_hx8379c_tdt, sizeof(lcm_deep_sleep_mode_in_setting_hx8379c_tdt) / sizeof(struct LCM_setting_table), 1);

	SET_RESET_PIN(0);
	MDELAY(1);
	SET_RESET_PIN(1);
}

static void lcm_resume(void)
{
	lcm_init();

	/*Begin ersen.shang remove some delays for task 117114 reference pixi3-4*/
	/*push_table(lcm_sleep_out_setting_hx8379c_tdt, sizeof(lcm_sleep_out_setting_hx8379c_tdt) / sizeof(struct LCM_setting_table), 1);*/
	/*End   ersen.shang remove some delays for task 117114 reference pixi3-4*/
	
}

#if 0
static void lcm_update(unsigned int x, unsigned int y,
					   unsigned int width, unsigned int height)
{
	unsigned int x0 = x;
	unsigned int y0 = y;
	unsigned int x1 = x0 + width - 1;
	unsigned int y1 = y0 + height - 1;

	unsigned char x0_MSB = ((x0>>8)&0xFF);
	unsigned char x0_LSB = (x0&0xFF);
	unsigned char x1_MSB = ((x1>>8)&0xFF);
	unsigned char x1_LSB = (x1&0xFF);
	unsigned char y0_MSB = ((y0>>8)&0xFF);
	unsigned char y0_LSB = (y0&0xFF);
	unsigned char y1_MSB = ((y1>>8)&0xFF);
	unsigned char y1_LSB = (y1&0xFF);

	unsigned int data_array[16];

	data_array[0]= 0x00053902;
	data_array[1]= (x1_MSB<<24)|(x0_LSB<<16)|(x0_MSB<<8)|0x2a;
	data_array[2]= (x1_LSB);
	data_array[3]= 0x00053902;
	data_array[4]= (y1_MSB<<24)|(y0_LSB<<16)|(y0_MSB<<8)|0x2b;
	data_array[5]= (y1_LSB);
	data_array[6]= 0x002c3909;

	dsi_set_cmdq(data_array, 7, 0);

}
#endif

#if 0
static void lcm_setbacklight(unsigned int level)
{
	unsigned int default_level = 145;
	unsigned int mapped_level = 0;

	//for LGE backlight IC mapping table
	if(level > 255) 
		level = 255;

	if(level >0) 
		mapped_level = default_level+(level)*(255-default_level)/(255);
	else
		mapped_level=0;

	// Refresh value of backlight level.
	lcm_backlight_level_setting[0].para_list[0] = mapped_level;

	push_table(lcm_backlight_level_setting, sizeof(lcm_backlight_level_setting) / sizeof(struct LCM_setting_table), 1);
}
#endif


#if 0
static void lcm_setpwm(unsigned int divider)
{
	// TBD
}

static unsigned int lcm_getpwm(unsigned int divider)
{
	// ref freq = 15MHz, B0h setting 0x80, so 80.6% * freq is pwm_clk;
	// pwm_clk / 255 / 2(lcm_setpwm() 6th params) = pwm_duration = 23706
	unsigned int pwm_clk = 23706 / (1<<divider);	
	return pwm_clk;
}
#endif

static unsigned int lcm_esd_check(void)
{
#ifndef BUILD_LK
	char  buffer[5]={0};
	int   array[4];

#if defined(LCM_ESD_DEBUG)
	printk("hx8379c: lcm_esd_check enter\n");
#endif

	array[0] = 0x00043700;
	dsi_set_cmdq(array, 1, 1);

	//fangjie modify, because in DSI_dcs_read_lcm_reg_v2()
	//when the readsize>=3, the DI config is DSI_GERNERIC_READ_LONG_PACKET_ID. else DI is DSI_DCS_READ_PACKET_ID
	//old: read_reg_v2(0x09, buffer, 4); 
	read_reg_v2(0x09, buffer, 2); 

#if defined(LCM_ESD_DEBUG)
	printk("lcm_esd_check buffer[0]=0x%x\n",buffer[0]); //0x80
	printk("lcm_esd_check buffer[1]=0x%x\n",buffer[1]); //0x73
	printk("lcm_esd_check buffer[2]=0x%x\n",buffer[2]); //0x3E
	printk("lcm_esd_check buffer[3]=0x%x\n",buffer[3]); //0xA6
#endif

	if((buffer[0]==0x80)&&(buffer[1]==0x73))
	{
		return 0;
	}
	else
	{ 
		return 1;
	}
#endif
}

static unsigned int lcm_esd_recover(void)
{
#ifndef BUILD_LK 
	printk("lcm_esd_recover hx8379 enter");
	lcm_init();
	return 1;
#endif
}

#if defined(BUILD_LK)
// ---------------------------------------------------------------------------
//  Get LCM ID Information
// ---------------------------------------------------------------------------
static unsigned int lcm_compare_id(void)
{
	int id_type=0;	
	
	mt_set_gpio_mode(GPIO_LCM_ID1,GPIO_MODE_00);
	mt_set_gpio_pull_enable(GPIO_LCM_ID1, GPIO_PULL_DISABLE);
	mt_set_gpio_dir(GPIO_LCM_ID1, GPIO_DIR_IN);
	mt_set_gpio_mode(GPIO_LCM_ID2,GPIO_MODE_00);
	mt_set_gpio_pull_enable(GPIO_LCM_ID2, GPIO_PULL_DISABLE);
	mt_set_gpio_dir(GPIO_LCM_ID2, GPIO_DIR_IN);
	id_type = mt_get_gpio_in(GPIO_LCM_ID2)<<1 | mt_get_gpio_in(GPIO_LCM_ID1);
	#if defined(BUILD_LK)
	printf("\t\t 8379c [lcm_compare_id   id_type  %d ]\n" , id_type);		
        #else
	printk("\t\t 8379c [lcm_compare_id   id_type  %d ]\n" , id_type);	
        #endif
	if (id_type == 0 ) //TDT  ID pin = bit 00
	{
		return 1; 
	}
	else
	{
		return 0;
	}

} 
#endif
// ---------------------------------------------------------------------------
//  Get LCM Driver Hooks
// ---------------------------------------------------------------------------
LCM_DRIVER hx8379c_wvga_dsi_vdo_TDT_lcm_drv_pixi44 = 
{
	.name			= "hx8379c_wvga_dsi_vdo_TDT_pixi44",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params     = lcm_get_params,
	.init           = lcm_init,
	.suspend        = lcm_suspend,
	.resume         = lcm_resume,
#if (LCM_DSI_CMD_MODE)
	.update         = lcm_update,
#endif

	//	.set_backlight	= lcm_setbacklight,
	//	.set_pwm        = lcm_setpwm,
	//	.get_pwm        = lcm_getpwm,

	.esd_check     	= lcm_esd_check, 
	.esd_recover   	= lcm_esd_recover,
#if defined(BUILD_LK)
	.compare_id    	= lcm_compare_id,
#endif
};

