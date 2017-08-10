#include <linux/slab.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/time.h>
#include <linux/types.h>
#include <linux/seq_file.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <generated/autoconf.h>
#include <linux/kobject.h>
#include <linux/regulator/consumer.h>
#include <linux/time.h>
#include <linux/delay.h>
#ifdef CONFIG_COMPAT
#include <linux/compat.h>
#endif
#include "gpio_const.h"
#include "mt_gpio.h"

#define SGM_DEBUG 0   //add for close debug log by liuxin

#if  SGM_DEBUG  //modify by liuxin
#define SGM_TAG                  "[SGM3146-K]"
#define SGM_LOG(fmt, args...)	 printk(SGM_TAG fmt, ##args)
#else
#define SGM_LOG(fmt, args...) 
#endif

#define SGM_DEVICE "SGM3146"
#define MIN_BRIGHTNESS 1 //the min brightness is level 2.
#define MAX_BRIGHTNESS 11 //the min brightness is level 13 (about 20mA).
#define MAX_UPPER_LAYER_BRIGHTNESS 255 //the upper layer max brightness level is 255.
#define GPIO_LCD_BL_EN         (GPIO66 | 0x80000000)

struct pinctrl *sgmpinctrl1;
struct pinctrl_state  *gpio_output0, *gpio_output1;
struct of_device_id sgm3146_of_match[] = {
	{ .compatible = "mediatek,sgm3146", },
	{},
};
void sgm_gpio_output(int level);
int sgm_get_gpio_info(struct platform_device *pdev);
DEFINE_MUTEX(kernel_sgm3146_mutex);

/*
note:
ersen.shang instead of mutex_lock(&kernel_sgm3146_mutex); with local_irq_save(flags); 20151112
*/
int disp_sgm3146_set_backlight( int level_16)
{
	static int oldlevel = MAX_BRIGHTNESS;
	int count;
	int bl_level;
	unsigned long flags;
	
	SGM_LOG("disp_sgm3146_set_backlight oldlevel = %d , level_16 = %d\n",oldlevel , level_16);

	local_irq_save(flags);
	//mutex_lock(&kernel_sgm3146_mutex);
	bl_level = level_16;

	if (oldlevel == bl_level )
	{
 		local_irq_restore(flags);	
		//mutex_unlock(&kernel_sgm3146_mutex);
		return 0 ;
	}

	if (0 == bl_level) 
	{
		//disable backlight
		//SGM_LOG("Report pwrkey backlight off, Before backlight level = %d ---\n",oldlevel);
		sgm_gpio_output( 0);
		mdelay(5);
	} 
	else 
	{
		if (0 == oldlevel )//enable backlight
		{
			//SGM_LOG("Report pwrkey backlight on, current backlight level = %d ---\n", bl_level);
			sgm_gpio_output( 0);
			udelay(10); 
			sgm_gpio_output(1);
			udelay(35);

			for (count = 0;count < 15 -bl_level;count++)
			{
				sgm_gpio_output(0);
				udelay(1);
				sgm_gpio_output(1);
				udelay(2);
			}
		}
		else//modify backlight level
		{        
			//SGM_LOG("modify backlight level = %d ---\n", bl_level);
			//16-bl_level+oldlevel
			for (count = 0;count < 16-(16 - oldlevel) +(16 - bl_level);count++)
			{
				sgm_gpio_output(0);
				udelay(1);
				sgm_gpio_output(1);
				udelay(2);
			}
		}
	}

	//mutex_unlock(&kernel_sgm3146_mutex);
 	local_irq_restore(flags);	
	
	oldlevel = bl_level;

	return 0;
}

/* For backward compatible */
/*level_256 from Upper layer*/
/*level_16 for SGM3146 support 16 levels */
int disp_cust_set_backlight(int level_256)
{
	int level_16 = 0;  // the backlight range is level2~level13 for pixi44

	/*Begin ersen.shang modify the level convert logic 20151112*/
	#if 0
	if ((level_256 <= 32) && level_256 )
	{
		level_16 = MIN_BRIGHTNESS;
	}  
	else if (level_256 >= 255)
	{
		level_16 = MAX_BRIGHTNESS;
	}        
	else 
	{
		level_16 =(int)(level_256 *MAX_BRIGHTNESS/MAX_UPPER_LAYER_BRIGHTNESS) ;      
	}
	#else

	if(level_256 > 0)
		level_16 = (int)(level_256/(256/(MAX_BRIGHTNESS-MIN_BRIGHTNESS)))+MIN_BRIGHTNESS;
	else
		level_16 = 0;			
	#endif
	/*End   ersen.shang modify the level convert logic 20151112*/
	
	SGM_LOG("disp_cust_set_backlight level_256=%d,level_16=%d\n",level_256,level_16);

	return disp_sgm3146_set_backlight(level_16);
}

/* for magnify velocity******************************************** */

void sgm_gpio_output(int level)
{
	//SGM_LOG("[sgm]sgm_gpio_output, level = %d\n", level);
	/*Begin ersen.shang modify the gpio control function 20151112*/
#if 0
	if (level)
		pinctrl_select_state(sgmpinctrl1, gpio_output1);
	else
		pinctrl_select_state(sgmpinctrl1, gpio_output0);
#else
	if (level)
		mt_set_gpio_out(GPIO_LCD_BL_EN, GPIO_OUT_ONE );	
	else
		mt_set_gpio_out(GPIO_LCD_BL_EN, GPIO_OUT_ZERO);
#endif
	/*End   ersen.shang modify the gpio control function 20151112*/
}

int sgm_get_gpio_info(struct platform_device *pdev)
{
	int ret;

	SGM_LOG("[sgm %d] mt_sgm_pinctrl+++++++++++++++++\n", pdev->id);
	sgmpinctrl1 = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(sgmpinctrl1)) {
		ret = PTR_ERR(sgmpinctrl1);
		dev_err(&pdev->dev, "sgm Cannot find touch pinctrl1!\n");
		return ret;
	}
	gpio_output0 = pinctrl_lookup_state(sgmpinctrl1, "state_gpio_output0");
	if (IS_ERR(gpio_output0)) {
		ret = PTR_ERR(gpio_output0);
		dev_err(&pdev->dev, "sgm Cannot find touch pinctrl state_gpio_output0!\n");
		return ret;
	}
	gpio_output1 = pinctrl_lookup_state(sgmpinctrl1, "state_gpio_output1");
	if (IS_ERR(gpio_output1)) {
		ret = PTR_ERR(gpio_output1);
		dev_err(&pdev->dev, "sgm Cannot find touch pinctrl state_gpio_output1!\n");
		return ret;
	}
	SGM_LOG("[sgm%d] mt_sgm_pinctrl----------\n", pdev->id);
	return 0;
}


/* function definitions */
static int __init sgm_device_init(void);
static void __exit sgm_device_exit(void);
static int sgm_probe(struct platform_device *pdev);
static int sgm_remove(struct platform_device *pdev);

struct platform_device sgm_device = {
	.name		= SGM_DEVICE,
	.id			= -1,
};

static struct platform_driver sgm_driver = {
	.probe = sgm_probe,
	.remove = sgm_remove,
	.driver = {
		.name = SGM_DEVICE,
		.owner = THIS_MODULE,
		.of_match_table = sgm3146_of_match,
	},
};

/* touch panel probe */
static int sgm_probe(struct platform_device *pdev)
{
	sgm_get_gpio_info(pdev);
	return 0;
}
static int sgm_remove(struct platform_device *pdev)
{
	return 0;
}

/* called when loaded into kernel */
static int __init sgm_device_init(void)
{
	SGM_LOG("MediaTek touch panel driver init\n");
	if (platform_driver_register(&sgm_driver) != 0) {
		SGM_LOG("unable to register touch panel driver.\n");
		return -1;
	}
	return 0;
}

/* should never be called */
static void __exit sgm_device_exit(void)
{
	SGM_LOG("MediaTek touch panel driver exit\n");
	/* input_unregister_device(sgm->dev); */
	platform_driver_unregister(&sgm_driver);
}

late_initcall(sgm_device_init);
module_exit(sgm_device_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("SGM3146 IC Driver");
MODULE_AUTHOR("xiaopu.zhu@tcl.com");

