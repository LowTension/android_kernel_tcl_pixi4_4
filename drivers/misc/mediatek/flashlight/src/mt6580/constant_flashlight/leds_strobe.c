
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
//#include <gpio_const.h>
//#include <cust_gpio_usage.h>
//#include <mt_gpio.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/version.h>
#include <linux/i2c.h>
#include <linux/leds.h>

#include "kd_flashlight.h"
#include "kd_camera_typedef.h"

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
#include <linux/mutex.h>
#else
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,27)
#include <linux/semaphore.h>
#else
#include <asm/semaphore.h>
#endif
#endif

/******************************************************************************
* Debug configuration
******************************************************************************/
#define TAG_NAME "[leds_strobe.c]"
#define PK_DBG_NONE(fmt, arg...)    do {} while (0)
#define PK_DBG_FUNC(fmt, arg...)    pr_debug(TAG_NAME "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_WARN(fmt, arg...)        pr_warning(TAG_NAME "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_NOTICE(fmt, arg...)      pr_notice(TAG_NAME "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_INFO(fmt, arg...)        pr_info(TAG_NAME "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_TRC_FUNC(f)              pr_debug(TAG_NAME "<%s>\n", __FUNCTION__)
#define PK_TRC_VERBOSE(fmt, arg...) pr_debug(TAG_NAME fmt, ##arg)
#define PK_ERROR(fmt, arg...)       pr_err(TAG_NAME "%s: " fmt, __FUNCTION__ ,##arg)


#define DEBUG_LEDS_STROBE
#ifdef  DEBUG_LEDS_STROBE
#define PK_DBG PK_DBG_FUNC
#define PK_VER PK_TRC_VERBOSE
#define PK_ERR PK_ERROR
#else
#define PK_DBG(a,...)
#define PK_VER(a,...)
#define PK_ERR(a,...)
#endif

/******************************************************************************
* local variables
******************************************************************************/

static DEFINE_SPINLOCK(g_strobeSMPLock); /* cotta-- SMP proection */

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,37))
static DEFINE_MUTEX(g_strobeSem);
#else
static DECLARE_MUTEX(g_strobeSem);
#endif


static u32 strobe_Res = 0;
static u32 strobe_Timeus = 0;
static BOOL g_strobe_On = 0;

static int g_duty=-1;
static int g_timeOutTimeMs=0;
static struct hrtimer g_timeOutTimer;

static struct work_struct workTimeOut;

/*****************************************************************************
Functions
*****************************************************************************/

int FL_Enable(void)
{
	if(g_duty==0)
	{
		strobe_gpio_output(KD_FLASHLIGHT_ENABLE,1);
		strobe_gpio_output(KD_FLASHLIGHT_TORCH_,0);
		PK_DBG(" FL_Enable line=%d\n",__LINE__);
	}
	else
	{
		strobe_gpio_output(KD_FLASHLIGHT_ENABLE,1);
		strobe_gpio_output(KD_FLASHLIGHT_TORCH_,1);
		PK_DBG(" FL_Enable line=%d\n",__LINE__);
	}
	return 0;
}

int FL_Disable(void)
{
	strobe_gpio_output(KD_FLASHLIGHT_ENABLE,0);
	strobe_gpio_output(KD_FLASHLIGHT_TORCH_,0);
	PK_DBG(" FL_Disable line=%d\n",__LINE__);
	return 0;
}

int FL_dim_duty(kal_uint32 duty)
{
	PK_DBG(" FL_dim_duty line=%d\n",__LINE__);
	g_duty = duty;
	return 0;
}

int TORCH_Enable(void)
{
	strobe_gpio_output(KD_FLASHLIGHT_ENABLE,1);
	strobe_gpio_output(KD_FLASHLIGHT_TORCH_,0);
	return 0;
}

int TORCH_Disable(void)
{
	strobe_gpio_output(KD_FLASHLIGHT_ENABLE,0);
	strobe_gpio_output(KD_FLASHLIGHT_TORCH_,0);
	return 0;
}

int FL_Init(void)
{
	strobe_gpio_output(KD_FLASHLIGHT_ENABLE,0);
	strobe_gpio_output(KD_FLASHLIGHT_TORCH_,0);
	PK_DBG(" FL_Init line=%d\n",__LINE__);
	return 0;
}

int FL_Uninit(void)
{
	FL_Disable();
	return 0;
}

/*****************************************************************************
User interface
*****************************************************************************/

static void work_timeOutFunc(struct work_struct *data)
{
	FL_Disable();
	PK_DBG("ledTimeOut_callback\n");
}

enum hrtimer_restart ledTimeOutCallback(struct hrtimer *timer)
{
	schedule_work(&workTimeOut);
	return HRTIMER_NORESTART;
}

void timerInit(void)
{
	INIT_WORK(&workTimeOut, work_timeOutFunc);
	g_timeOutTimeMs=1000; //1s
	hrtimer_init( &g_timeOutTimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
	g_timeOutTimer.function=ledTimeOutCallback;

}

static int constant_flashlight_ioctl(unsigned int cmd, unsigned long arg)
{
	int i4RetValue = 0;
	int ior_shift;
	int iow_shift;
	int iowr_shift;
	int copysize;

	kdStrobeDrvArg kdArg;

	ior_shift = cmd - (_IOR(FLASHLIGHT_MAGIC,0, int));
	iow_shift = cmd - (_IOW(FLASHLIGHT_MAGIC,0, int));
	iowr_shift = cmd - (_IOWR(FLASHLIGHT_MAGIC,0, int));

	PK_DBG("constant_flashlight_ioctl() line=%d ior_shift=%d, iow_shift=%d iowr_shift=%d arg=%d\n",__LINE__, ior_shift, iow_shift, iowr_shift,(int)arg);

	switch(cmd)
	{

	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		PK_DBG("FLASH_IOC_SET_TIME_OUT_TIME_MS: %d\n",(int)arg);
		g_timeOutTimeMs=arg;
		break;


	case FLASH_IOC_SET_DUTY :
		PK_DBG("FLASHLIGHT_DUTY: %d\n",(int)arg);
		FL_dim_duty(arg);
		break;


	case FLASH_IOC_SET_STEP:
		PK_DBG("FLASH_IOC_SET_STEP: %d\n",(int)arg);

		break;

	case FLASH_IOC_SET_ONOFF :
		PK_DBG("FLASHLIGHT_ONOFF: %d\n",(int)arg);
		if(arg==1)
		{
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
			FL_Enable();
		}
		else
		{
			FL_Disable();
			hrtimer_cancel( &g_timeOutTimer );
		}
		break;
	case FLASHLIGHTIOC_ENABLE_STATUS:
		printk("**********torch g_strobe_on = %d \n", g_strobe_On);
		kdArg.arg = g_strobe_On;
		copysize = copy_to_user((void __user *) arg , (void*)&kdArg , sizeof(kdStrobeDrvArg));
		break;
	case FLASHLIGHT_TORCH_SELECT:
		printk("@@@@@@FLASHLIGHT_TORCH_SELECT@@@@@@\n");
		if (arg)
		{
			TORCH_Enable();
			g_strobe_On = TRUE;
		} 
		else 
		{
			TORCH_Disable();
			g_strobe_On = FALSE;
		}
		break;
	default :
		PK_DBG(" No such command \n");
		i4RetValue = -EPERM;
		break;
	}
	return i4RetValue;
}

static int constant_flashlight_open(void *pArg)
{
	int i4RetValue = 0;
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	if (0 == strobe_Res)
	{
		FL_Init();
		timerInit();
	}
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);
	spin_lock_irq(&g_strobeSMPLock);


	if(strobe_Res)
	{
		PK_ERR(" busy!\n");
		i4RetValue = -EBUSY;
	}
	else
	{
		strobe_Res += 1;
	}


	spin_unlock_irq(&g_strobeSMPLock);
	PK_DBG("constant_flashlight_open line=%d\n", __LINE__);

	return i4RetValue;
}

static int constant_flashlight_release(void *pArg)
{
	PK_DBG(" constant_flashlight_release\n");

	if (strobe_Res)
	{
		spin_lock_irq(&g_strobeSMPLock);

		strobe_Res = 0;
		strobe_Timeus = 0;

		/* LED On Status */
		g_strobe_On = FALSE;

		spin_unlock_irq(&g_strobeSMPLock);

		FL_Uninit();
	}

	PK_DBG(" Done\n");

	return 0;

}

FLASHLIGHT_FUNCTION_STRUCT	constantFlashlightFunc=
{
	constant_flashlight_open,
	constant_flashlight_release,
	constant_flashlight_ioctl
};

MUINT32 constantFlashlightInit(PFLASHLIGHT_FUNCTION_STRUCT *pfFunc)
{
	if (pfFunc != NULL)
	{
		*pfFunc = &constantFlashlightFunc;
	}
	return 0;
}

/* LED flash control for high current capture mode*/
ssize_t strobe_VDIrq(void)
{
	return 0;
}

EXPORT_SYMBOL(strobe_VDIrq);

