
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/wait.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include "kd_camera_typedef.h"
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#ifdef CONFIG_COMPAT
#include <linux/fs.h>
#include <linux/compat.h>
#endif
#include "kd_flashlight.h"
#include <linux/errno.h>
/******************************************************************************
 * Debug configuration
******************************************************************************/
/* availible parameter */
/* ANDROID_LOG_ASSERT */
/* ANDROID_LOG_ERROR */
/* ANDROID_LOG_WARNING */
/* ANDROID_LOG_INFO */
/* ANDROID_LOG_DEBUG */
/* ANDROID_LOG_VERBOSE */
#define TAG_NAME "[sub_strobe.c]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_WARN(fmt, arg...)        pr_warn(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_NOTICE(fmt, arg...)      pr_notice(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_INFO(fmt, arg...)        pr_info(TAG_NAME "%s: " fmt, __func__ , ##arg)
#define PK_TRC_FUNC(f)              pr_debug(TAG_NAME "<%s>\n", __func__)
#define PK_TRC_VERBOSE(fmt, arg...) pr_debug(TAG_NAME fmt, ##arg)
#define PK_ERROR(fmt, arg...)       pr_err(TAG_NAME "%s: " fmt, __func__ , ##arg)

#define DEBUG_LEDS_STROBE
#ifdef DEBUG_LEDS_STROBE
#define PK_DBG PK_DBG_FUNC
#define PK_VER PK_TRC_VERBOSE
#define PK_ERR PK_ERROR
#else
#define PK_DBG(a, ...)
#define PK_VER(a, ...)
#define PK_ERR(a, ...)
#endif


static u32 strobe_Res = 0; 
static BOOL g_strobe_On = 0;

static DEFINE_SPINLOCK(g_strobeSMPLock_subflash); /* cotta-- SMP proection */

static int sub_flash_disable(void)
{	
	strobe_gpio_output(KD_FRONT_FLASHLIGHT_ENABLE,0);
	//mt_set_gpio_out(GPIO_TORCH_EN,GPIO_OUT_ZERO);
	PK_DBG(" sub_flash_Disable line=%d\n",__LINE__);
	return 0;
}
static int sub_flash_enable(void)
{
	strobe_gpio_output(KD_FRONT_FLASHLIGHT_ENABLE,1);
	//mt_set_gpio_out(GPIO_TORCH_EN,GPIO_OUT_ZERO);
	PK_DBG(" sub_flash_enable line=%d\n",__LINE__);
	return 0;
}
static int sub_flash_uninit(void)
{
	sub_flash_disable();
	return 0;
}

static int sub_flash_init(void)
{

	strobe_gpio_output(KD_FRONT_FLASHLIGHT_ENABLE,0);
	PK_DBG(" sub_flash_init line=%d\n",__LINE__);
	return 0;
}


static int sub_strobe_ioctl(unsigned int cmd, unsigned long arg)
{
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;
	int copySize;
	
	kdStrobeDrvArg kdArg;
	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC,0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC,0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC,0, int));
	PK_DBG(" sub_strobe_ioctl() line=%d ior_shift=%d, iow_shift=%d iowr_shift=%d arg=%d\n",__LINE__, ior_shift, iow_shift, iowr_shift,(int)arg);
	switch(cmd)
	{

	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		PK_DBG("FLASH_IOC_SET_TIME_OUT_TIME_MS: %d\n",(int)arg);
		//g_timeOutTimeMs=arg;
		break;


	case FLASH_IOC_SET_DUTY :
		PK_DBG("FLASHLIGHT_DUTY: %d\n",(int)arg);
		//FL_dim_duty(arg);
		break;


	case FLASH_IOC_SET_STEP:
		PK_DBG("FLASH_IOC_SET_STEP: %d\n",(int)arg);

		break;

	case FLASH_IOC_SET_ONOFF :
		PK_DBG("FLASHLIGHT_ONOFF: %d\n",(int)arg);
		if(arg==1)
		{
			/*
			int s;
			int ms;
			if(g_timeOutTimeMs>1000)
			{
			s = g_timeOutTimeMs/1000;
			ms = g_timeOutTimeMs - s*1000;
			}
			else
			{
			s = 0;
			ms = g_timeOutTimeMs;
			}

			if(g_timeOutTimeMs!=0)
			{
			ktime_t ktime;
			ktime = ktime_set( s, ms*1000000 );
			hrtimer_start( &g_timeOutTimer, ktime, HRTIMER_MODE_REL );
			}
			*/
			sub_flash_enable();
		}
		else
		{
			sub_flash_disable();
			//hrtimer_cancel( &g_timeOutTimer );
		}
		break;
	case FLASHLIGHTIOC_ENABLE_STATUS:
		printk("**********torch g_strobe_on = %d \n", g_strobe_On);
		kdArg.arg = g_strobe_On;
		copySize = copy_to_user((void __user *) arg , (void*)&kdArg , sizeof(kdStrobeDrvArg));
		break;
		/*
		case FLASHLIGHT_TORCH_SELECT:
		printk("@@@@@@FLASHLIGHT_TORCH_SELECT@@@@@@\n");
		if (arg){
		TORCH_Enable();
		g_strobe_On = TRUE;
		} else {
		TORCH_Disable();
		g_strobe_On = FALSE;
		}
		break;*/
	default :
		PK_DBG(" No such command \n");
		i4RetValue = -EPERM;
		break;
	}
	return i4RetValue;
}


static int sub_strobe_open(void *pArg)
{
	int i4RetValue = 0;
	PK_DBG("sub_strobe_open line=%d\n", __LINE__);

	if (0 == strobe_Res)
	{
		sub_flash_init();
		//timerInit(); //temp delete
	}
	PK_DBG("sub_strobe_open line=%d\n", __LINE__);

	spin_lock_irq(&g_strobeSMPLock_subflash);
	if(strobe_Res)
	{
		PK_ERR(" busy!\n");
		i4RetValue = -EBUSY;
	}
	else
	{
		strobe_Res += 1;
	}
	spin_unlock_irq(&g_strobeSMPLock_subflash);
	PK_DBG("sub_strobe_open line=%d\n", __LINE__);

	return i4RetValue;

}


static int sub_strobe_release(void *pArg)
{
	PK_DBG(" sub_strobe_release\n");
	if (strobe_Res)
	{
		spin_lock_irq(&g_strobeSMPLock_subflash);
		strobe_Res = 0;
		//strobe_Timeus = 0;
		/* LED On Status */
		g_strobe_On = FALSE;
		spin_unlock_irq(&g_strobeSMPLock_subflash);
		sub_flash_uninit();
	}
	PK_DBG(" Done\n");
	return 0;

}

FLASHLIGHT_FUNCTION_STRUCT subStrobeFunc = {
	sub_strobe_open,
	sub_strobe_release,
	sub_strobe_ioctl
};


MUINT32 subStrobeInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
	if (pfFunc != NULL)
		*pfFunc = &subStrobeFunc;
	return 0;
}
