////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2014 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (??MStar Confidential Information??) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

/**
 *
 * @file    mstar_drv_platform_porting_layer.c
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */
 
/*=============================================================*/
// INCLUDE FILE
/*=============================================================*/

#include "mstar_drv_platform_porting_layer.h"
#include "mstar_drv_ic_fw_porting_layer.h"
#include "mstar_drv_platform_interface.h"
#include "mstar_drv_utility_adaption.h"
#include "mstar_drv_main.h"

//begin-add for close debug log by liuxin
#define DEBUG_SWITCH 0

#if DEBUG_SWITCH
#define GSE_TAG				  "[Msg22xx] "
#define GSE_ERR(fmt, args...)	pr_err(GSE_TAG fmt, ##args)
#define GSE_LOG(fmt, args...)	pr_debug(GSE_TAG fmt, ##args)
#else
#define GSE_TAG
#define GSE_ERR(fmt, args...)	do {} while (0)
#define GSE_LOG(fmt, args...)	do {} while (0)
#endif
//end-add for close debug log by liuxin

#ifdef CONFIG_ENABLE_HOTKNOT
#include "mstar_drv_hotknot_queue.h"
#endif //CONFIG_ENABLE_HOTKNOT

#ifdef CONFIG_ENABLE_JNI_INTERFACE
#include "mstar_drv_jni_interface.h"
#endif //CONFIG_ENABLE_JNI_INTERFACE

/*=============================================================*/
// EXTREN VARIABLE DECLARATION
/*=============================================================*/

extern struct kset *g_TouchKSet;
extern struct kobject *g_TouchKObj;

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
extern struct kset *g_GestureKSet;
extern struct kobject *g_GestureKObj;
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
extern u8 g_FaceClosingTp;
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
extern struct tpd_device *tpd;
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
extern struct regulator *g_ReguVdd;
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON
#endif

#ifdef CONFIG_ENABLE_HOTKNOT
extern struct miscdevice hotknot_miscdevice;
extern u8 g_HotKnotState;
#endif //CONFIG_ENABLE_HOTKNOT

extern u8 IS_FIRMWARE_DATA_LOG_ENABLED;

/*=============================================================*/
// LOCAL VARIABLE DEFINITION
/*=============================================================*/

struct mutex g_Mutex;
spinlock_t _gIrqLock;
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
static struct work_struct _gFingerTouchWork;
#endif
static int _gInterruptFlag = 0;

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
#ifdef CONFIG_ENABLE_NOTIFIER_FB
static struct notifier_block _gFbNotifier;
#else
//static struct early_suspend _gEarlySuspend;
#endif //CONFIG_ENABLE_NOTIFIER_FB
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM || CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifndef CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
static DECLARE_WAIT_QUEUE_HEAD(_gWaiter);
static struct task_struct *_gThread = NULL;
static int _gTpdFlag = 0;
#endif //CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

/*=============================================================*/
// GLOBAL VARIABLE DEFINITION
/*=============================================================*/

#ifdef CONFIG_TP_HAVE_KEY
const int g_TpVirtualKey[] = {TOUCH_KEY_BACK, TOUCH_KEY_MENU, TOUCH_KEY_HOME/*, TOUCH_KEY_SEARCH*/};

#ifdef CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#define BUTTON_W (100)
#define BUTTON_H (100)

const int g_TpVirtualKeyDimLocal[MAX_KEY_NUM][4] = {{BUTTON_W/2*1,TOUCH_SCREEN_Y_MAX+BUTTON_H/2,BUTTON_W,BUTTON_H},{BUTTON_W/2*3,TOUCH_SCREEN_Y_MAX+BUTTON_H/2,BUTTON_W,BUTTON_H},{BUTTON_W/2*5,TOUCH_SCREEN_Y_MAX+BUTTON_H/2,BUTTON_W,BUTTON_H}};
#endif //CONFIG_ENABLE_REPORT_KEY_WITH_COORDINATE
#endif //CONFIG_TP_HAVE_KEY

struct input_dev *g_InputDevice = NULL;
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
static int _gIrq = -1;
#endif
unsigned int touch_irq = 0;
/*=============================================================*/
// LOCAL FUNCTION DEFINITION
/*=============================================================*/
#if defined(CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM)
        /* initialize the finger touch work queue */ 
/* read data through I2C then report data to input sub-system when interrupt occurred */
static void _DrvPlatformLyrFingerTouchDoWork(struct work_struct *pWork)
{
    unsigned long nIrqFlag;

    GSE_LOG("*** %s() ***\n", __func__);

    DrvIcFwLyrHandleFingerTouch(NULL, 0);

    GSE_LOG("*** %s() _gInterruptFlag = %d ***\n", __func__, _gInterruptFlag);  // add for debug

    GSE_LOG("*** %s() spin_lock_irqsave() ***\n", __func__);  // add for debug
    spin_lock_irqsave(&_gIrqLock, nIrqFlag);

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)

    if (_gInterruptFlag == 0) 
    {
        enable_irq(_gIrq);
        GSE_LOG("*** %s() enable_irq(_gIrq) ***\n", __func__);  // add for debug

        _gInterruptFlag = 1;
    } 
        
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)

    if (_gInterruptFlag == 0) 
    {
      //  mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
        GSE_LOG("*** %s() mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM) ***\n", __func__);  // add for debug
        enable_irq(touch_irq);
        _gInterruptFlag = 1;
    }

#endif

    spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);
    GSE_LOG("*** %s() spin_unlock_irqrestore() ***\n", __func__);  // add for debug
}
#endif
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
/* The interrupt service routine will be triggered when interrupt occurred */
static irqreturn_t _DrvPlatformLyrFingerTouchInterruptHandler(s32 nIrq, void *pDeviceId)
{
    unsigned long nIrqFlag;

    GSE_LOG("*** %s() ***\n", __func__);

    GSE_LOG("*** %s() _gInterruptFlag = %d ***\n", __func__, _gInterruptFlag);  // add for debug

    GSE_LOG("*** %s() spin_lock_irqsave() ***\n", __func__);  // add for debug
    spin_lock_irqsave(&_gIrqLock, nIrqFlag);

    if (_gInterruptFlag == 1) 
    {
        disable_irq_nosync(_gIrq);
        GSE_LOG("*** %s() disable_irq_nosync(_gIrq) ***\n", __func__);  // add for debug

        _gInterruptFlag = 0;

        schedule_work(&_gFingerTouchWork);
    }

    spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);
    GSE_LOG("*** %s() spin_unlock_irqrestore() ***\n", __func__);  // add for debug
    
    return IRQ_HANDLED;
}
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
static irqreturn_t _DrvPlatformLyrFingerTouchInterruptHandler(s32 nIrq, void *pDeviceId)
{
    unsigned long nIrqFlag;

    GSE_LOG("*** %s() ***\n", __func__);  

    GSE_LOG("*** %s() _gInterruptFlag = %d ***\n", __func__, _gInterruptFlag);  // add for debug

    GSE_LOG("*** %s() spin_lock_irqsave() ***\n", __func__);  // add for debug
    spin_lock_irqsave(&_gIrqLock, nIrqFlag);

#ifdef CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM

    if (_gInterruptFlag == 1)
    {
     //  mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
       disable_irq_nosync(touch_irq);
        GSE_LOG("*** %s() mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM) ***\n", __func__);  // add for debug

        _gInterruptFlag = 0;

        schedule_work(&_gFingerTouchWork);
    }

#else    

    if (_gInterruptFlag == 1) 
    {    
      //  mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
        GSE_LOG("*** %s() mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM) ***\n", __func__);  // add for debug
		disable_irq_nosync(touch_irq);
        _gInterruptFlag = 0;

        _gTpdFlag = 1;
        wake_up_interruptible(&_gWaiter);
    }        
#endif //CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM

    spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);
    GSE_LOG("*** %s() spin_unlock_irqrestore() ***\n", __func__);  // add for debug
  return IRQ_HANDLED;
}

#ifndef CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
static int _DrvPlatformLyrFingerTouchHandler(void *pUnUsed)
{
    unsigned long nIrqFlag;
    struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
    sched_setscheduler(current, SCHED_RR, &param);

    GSE_LOG("*** %s() ***\n", __func__);  
	
    do
    {
        set_current_state(TASK_INTERRUPTIBLE);
        wait_event_interruptible(_gWaiter, _gTpdFlag != 0);
        _gTpdFlag = 0;
        
        set_current_state(TASK_RUNNING);

        DrvIcFwLyrHandleFingerTouch(NULL, 0);

        GSE_LOG("*** %s() _gInterruptFlag = %d ***\n", __func__, _gInterruptFlag);  // add for debug

        GSE_LOG("*** %s() spin_lock_irqsave() ***\n", __func__);  // add for debug
        spin_lock_irqsave(&_gIrqLock, nIrqFlag);

        if (_gInterruptFlag == 0)        
        {
           // mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
            GSE_LOG("*** %s() mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM) ***\n", __func__);  // add for debug
		enable_irq(touch_irq);
            _gInterruptFlag = 1;
        } 

        spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);
        GSE_LOG("*** %s() spin_unlock_irqrestore() ***\n", __func__);  // add for debug
		
    } while (!kthread_should_stop());
	
    return 0;
}
#endif //CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
#endif


void DrvIcFwLyrIsRegisterFingerTouchInterruptHandler(void)
{

	struct device_node *node = NULL;
	int ret = 0;
	u32 ints[2] = { 0, 0 };
       GSE_LOG("*** %s() ***\n", __func__);
	  node = of_find_matching_node(node, touch_of_match);
	if (node) {
		of_property_read_u32_array(node, "debounce", ints, ARRAY_SIZE(ints));
		gpio_set_debounce(ints[0], ints[1]);

		/*touch_irq = gpio_to_irq(tpd_int_gpio_number);*/
		touch_irq = irq_of_parse_and_map(node, 0);
	    TPD_DMESG("[%s] tpd request_irq =%d.", __func__,touch_irq);
		ret = request_irq(touch_irq, _DrvPlatformLyrFingerTouchInterruptHandler,
					IRQF_TRIGGER_FALLING, TPD_DEVICE,NULL);
			if (ret > 0)
				TPD_DMESG("tpd request_irq IRQ LINE NOT AVAILABLE!.");
	} else {
		TPD_DMESG("[%s] tpd request_irq can not find touch eint device node!.", __func__);
	}
	return;
   // Indicate that it is necessary to register interrupt handler with GPIO INT pin when firmware is running on IC
}

/*=============================================================*/
// GLOBAL FUNCTION DEFINITION
/*=============================================================*/

#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
void DrvPlatformLyrTouchDeviceRegulatorPowerOn(void)
{
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    s32 nRetVal = 0;

    GSE_LOG("*** %s() ***\n", __func__);
    
    nRetVal = regulator_set_voltage(g_ReguVdd, 2800000, 2800000); // For specific SPRD BB chip(ex. SC7715) or QCOM BB chip(ex. MSM8610), need to enable this function call for correctly power on Touch IC.

    if (nRetVal)
    {
        GSE_LOG("Could not set to 2800mv.\n");
    }
    regulator_enable(g_ReguVdd);

    mdelay(20);
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    s32 nRetVal = 0;
    GSE_LOG("*** %s() ***\n", __func__);
   // hwPowerOn(MT6323_POWER_LDO_VGP1, VOL_2800, "TP"); // For specific MTK BB chip(ex. MT6582), need to enable this function call for correctly power on Touch IC.
  //hwPowerOn(MT6325_POWER_LDO_VGP1, VOL_2800, "TP");  // For specific MTK BB chip(ex. MT6735), need to enable this function call for correctly power on Touch IC.
	if(tpd->reg==NULL)
	{
	printk(" TPD DrvPlatformLyrTouchDeviceRegulatorPowerOn tpd->reg==NULL!\n");
	}
	printk(" TPD DrvPlatformLyrTouchDeviceRegulatorPowerOn regulator_set_voltage!\n");
       nRetVal = regulator_set_voltage(tpd->reg, 2800000, 2800000); // For specific SPRD BB chip(ex. SC7715) or QCOM BB chip(ex. MSM8610), need to enable this function call for correctly power on Touch IC.
   //  nRetVal = regulator_set_voltage(g_ReguVdd, 2800000, 2800000);
	if (nRetVal)
    {
        GSE_LOG("Could not set to 2800mv.\n");
    }
  printk(" TPD DrvPlatformLyrTouchDeviceRegulatorPowerOn regulator_set_voltage ok !\n");
    nRetVal = regulator_enable(tpd->reg);
    mdelay(20);
#endif
}
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON

void DrvPlatformLyrTouchDevicePowerOn(void)
{
    GSE_LOG("*** %s() ***\n", __func__);
    
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    gpio_direction_output(MS_TS_MSG_IC_GPIO_RST, 1);
//    gpio_set_value(MS_TS_MSG_IC_GPIO_RST, 1); 
    udelay(100); 
    gpio_set_value(MS_TS_MSG_IC_GPIO_RST, 0);
    udelay(100); 
    gpio_set_value(MS_TS_MSG_IC_GPIO_RST, 1);
    mdelay(25); 
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#if 0
    mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_RST, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(MS_TS_MSG_IC_GPIO_RST, GPIO_OUT_ONE);  
    udelay(100); 

    mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_RST, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(MS_TS_MSG_IC_GPIO_RST, GPIO_OUT_ZERO);  
    udelay(100); 
#endif

    tpd_gpio_output(GTP_RST_PORT,1);
    udelay(50);
    tpd_gpio_output(GTP_RST_PORT,0);
	udelay(50);
#ifdef TPD_CLOSE_POWER_IN_SLEEP
    hwPowerDown(TPD_POWER_SOURCE, "TP"); 
    mdelay(100);
    hwPowerOn(TPD_POWER_SOURCE, VOL_2800, "TP"); 
    mdelay(10);  // reset pulse
#endif //TPD_CLOSE_POWER_IN_SLEEP
    tpd_gpio_output(GTP_RST_PORT,1);
#if 0
    mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_RST, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(MS_TS_MSG_IC_GPIO_RST, GPIO_OUT_ONE);
#endif
    mdelay(25); 
#endif
}

void DrvPlatformLyrTouchDevicePowerOff(void)
{
    GSE_LOG("*** %s() ***\n", __func__);
    
    DrvIcFwLyrOptimizeCurrentConsumption();

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
//    gpio_direction_output(MS_TS_MSG_IC_GPIO_RST, 0);
    gpio_set_value(MS_TS_MSG_IC_GPIO_RST, 0);
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#if 0
    mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_RST, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(MS_TS_MSG_IC_GPIO_RST, GPIO_OUT_ZERO);  
#endif
    tpd_gpio_output(GTP_RST_PORT,0);
#ifdef TPD_CLOSE_POWER_IN_SLEEP
    hwPowerDown(TPD_POWER_SOURCE, "TP");
#endif //TPD_CLOSE_POWER_IN_SLEEP
#endif    
}

void DrvPlatformLyrTouchDeviceResetHw(void)
{
    GSE_LOG("*** %s() ***\n", __func__);
    
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    gpio_direction_output(MS_TS_MSG_IC_GPIO_RST, 1);
//    gpio_set_value(MS_TS_MSG_IC_GPIO_RST, 1); 
    gpio_set_value(MS_TS_MSG_IC_GPIO_RST, 0);
    mdelay(100); 
    gpio_set_value(MS_TS_MSG_IC_GPIO_RST, 1);
    mdelay(100); 
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#if 0
    mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_RST, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(MS_TS_MSG_IC_GPIO_RST, GPIO_OUT_ONE);
    mdelay(10);
    mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_RST, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(MS_TS_MSG_IC_GPIO_RST, GPIO_OUT_ZERO);  
    mdelay(50);
    mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_RST, GPIO_CTP_RST_PIN_M_GPIO);
    mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_RST, GPIO_DIR_OUT);
    mt_set_gpio_out(MS_TS_MSG_IC_GPIO_RST, GPIO_OUT_ONE);
    mdelay(50); 
#endif


   tpd_gpio_output(GTP_RST_PORT,1);
    mdelay(10);
   tpd_gpio_output(GTP_RST_PORT,0);
   mdelay(50);
   tpd_gpio_output(GTP_RST_PORT,1);
   mdelay(50);
   
#endif
}

void DrvPlatformLyrDisableFingerTouchReport(void)
{
    unsigned long nIrqFlag;

    GSE_LOG("*** %s() ***\n", __func__);

    GSE_LOG("*** %s() _gInterruptFlag = %d ***\n", __func__, _gInterruptFlag);  // add for debug

    GSE_LOG("*** %s() spin_lock_irqsave() ***\n", __func__);  // add for debug
    spin_lock_irqsave(&_gIrqLock, nIrqFlag);

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)

#ifdef CONFIG_ENABLE_HOTKNOT
    if (g_HotKnotState != HOTKNOT_TRANS_STATE && g_HotKnotState != HOTKNOT_BEFORE_TRANS_STATE)
#endif //CONFIG_ENABLE_HOTKNOT
    {
        if (_gInterruptFlag == 1)  
        {
            disable_irq(_gIrq);
            GSE_LOG("*** %s() disable_irq(_gIrq) ***\n", __func__);  // add for debug

            _gInterruptFlag = 0;
        }
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)

#ifdef CONFIG_ENABLE_HOTKNOT
    if (g_HotKnotState != HOTKNOT_TRANS_STATE && g_HotKnotState != HOTKNOT_BEFORE_TRANS_STATE)
#endif //CONFIG_ENABLE_HOTKNOT           
    {
        if (_gInterruptFlag == 1) 
        {
           // mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
            GSE_LOG("*** %s() mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM) ***\n", __func__);  // add for debug
            disable_irq_nosync(touch_irq);
            _gInterruptFlag = 0;
        }
    }
#endif

    spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);
    GSE_LOG("*** %s() spin_unlock_irqrestore() ***\n", __func__);  // add for debug
}

void DrvPlatformLyrEnableFingerTouchReport(void)
{
    unsigned long nIrqFlag;

    GSE_LOG("*** %s() ***\n", __func__);

    GSE_LOG("*** %s() _gInterruptFlag = %d ***\n", __func__, _gInterruptFlag);  // add for debug

    GSE_LOG("*** %s() spin_lock_irqsave() ***\n", __func__);  // add for debug
    spin_lock_irqsave(&_gIrqLock, nIrqFlag);

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)

    if (_gInterruptFlag == 0) 
    {
        enable_irq(_gIrq);
        GSE_LOG("*** %s() enable_irq(_gIrq) ***\n", __func__);  // add for debug

        _gInterruptFlag = 1;        
    }

#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)

    if (_gInterruptFlag == 0) 
    {
     //   mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
        GSE_LOG("*** %s() mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM) ***\n", __func__);  // add for debug
enable_irq(touch_irq);
        _gInterruptFlag = 1;        
    }

#endif

    spin_unlock_irqrestore(&_gIrqLock, nIrqFlag);
    GSE_LOG("*** %s() spin_unlock_irqrestore() ***\n", __func__);  // add for debug
}

void DrvPlatformLyrFingerTouchPressed(s32 nX, s32 nY, s32 nPressure, s32 nId)
{
    GSE_LOG("*** %s() ***\n", __func__);
    DBG("point touch pressed\n");

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL // TYPE B PROTOCOL
//	input_report_key(g_InputDevice, BTN_TOUCH, 1);
    input_mt_slot(g_InputDevice, nId);
    input_mt_report_slot_state(g_InputDevice, MT_TOOL_FINGER, true);
//    input_report_abs(g_InputDevice, ABS_MT_TRACKING_ID, nId);
    input_report_abs(g_InputDevice, ABS_MT_TOUCH_MAJOR, 1);
    input_report_abs(g_InputDevice, ABS_MT_WIDTH_MAJOR, 1);
    input_report_abs(g_InputDevice, ABS_MT_POSITION_X, nX);
    input_report_abs(g_InputDevice, ABS_MT_POSITION_Y, nY);

    GSE_LOG("nId=%d, nX=%d, nY=%d\n", nId, nX, nY); // TODO : add for debug
#else // TYPE A PROTOCOL
    input_report_key(g_InputDevice, BTN_TOUCH, 1);
#if defined(CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC)
    input_report_abs(g_InputDevice, ABS_MT_TRACKING_ID, nId);
#endif //CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC
    input_report_abs(g_InputDevice, ABS_MT_TOUCH_MAJOR, 1);
    input_report_abs(g_InputDevice, ABS_MT_WIDTH_MAJOR, 1);
    input_report_abs(g_InputDevice, ABS_MT_POSITION_X, nX);
    input_report_abs(g_InputDevice, ABS_MT_POSITION_Y, nY);

    input_mt_sync(g_InputDevice);
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL


#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_TP_HAVE_KEY    
    if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode())
    {   
        tpd_button(nX, nY, 1);  
    }
#endif //CONFIG_TP_HAVE_KEY

    TPD_EM_PRINT(nX, nY, nX, nY, nId, 1);
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
}

void DrvPlatformLyrFingerTouchReleased(s32 nX, s32 nY, s32 nId)
{
    GSE_LOG("*** %s() ***\n", __func__);
    DBG("point touch released\n");

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL // TYPE B PROTOCOL
//    input_report_key(g_InputDevice, BTN_TOUCH, 0); 
    input_mt_slot(g_InputDevice, nId);
//    input_report_abs(g_InputDevice, ABS_MT_TRACKING_ID, -1);
    input_mt_report_slot_state(g_InputDevice, MT_TOOL_FINGER, false);

    GSE_LOG("nId=%d\n", nId); // TODO : add for debug
#else // TYPE A PROTOCOL
    input_report_key(g_InputDevice, BTN_TOUCH, 0);
    input_mt_sync(g_InputDevice);
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL


#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_TP_HAVE_KEY 
    if (FACTORY_BOOT == get_boot_mode() || RECOVERY_BOOT == get_boot_mode())
    {   
       tpd_button(nX, nY, 0); 
//       tpd_button(0, 0, 0); 
    }            
#endif //CONFIG_TP_HAVE_KEY    

    TPD_EM_PRINT(nX, nY, nX, nY, 0, 0);
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
}

s32 DrvPlatformLyrInputDeviceInitialize(struct i2c_client *pClient)
{
    s32 nRetVal = 0;

    GSE_LOG("*** %s() ***\n", __func__);

    mutex_init(&g_Mutex);
    spin_lock_init(&_gIrqLock);

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    /* allocate an input device */
    g_InputDevice = input_allocate_device();
    if (g_InputDevice == NULL)
    {
        GSE_LOG("*** input device allocation failed ***\n");
        return -ENOMEM;
    }

    g_InputDevice->name = pClient->name;
    g_InputDevice->phys = "I2C";
    g_InputDevice->dev.parent = &pClient->dev;
    g_InputDevice->id.bustype = BUS_I2C;
    
    /* set the supported event type for input device */
    set_bit(EV_ABS, g_InputDevice->evbit);
    set_bit(EV_SYN, g_InputDevice->evbit);
    set_bit(EV_KEY, g_InputDevice->evbit);
    set_bit(BTN_TOUCH, g_InputDevice->keybit);
    set_bit(INPUT_PROP_DIRECT, g_InputDevice->propbit);

#ifdef CONFIG_TP_HAVE_KEY
    // Method 1.
    { 
        u32 i;
        for (i = 0; i < MAX_KEY_NUM; i ++)
        {
            input_set_capability(g_InputDevice, EV_KEY, g_TpVirtualKey[i]);
        }
    }
#endif
/*  
#ifdef CONFIG_TP_HAVE_KEY
    // Method 2.
    set_bit(TOUCH_KEY_MENU, g_InputDevice->keybit); //Menu
    set_bit(TOUCH_KEY_HOME, g_InputDevice->keybit); //Home
    set_bit(TOUCH_KEY_BACK, g_InputDevice->keybit); //Back
    set_bit(TOUCH_KEY_SEARCH, g_InputDevice->keybit); //Search
#endif
*/

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    input_set_capability(g_InputDevice, EV_KEY, KEY_POWER);
    input_set_capability(g_InputDevice, EV_KEY, KEY_UP);
    input_set_capability(g_InputDevice, EV_KEY, KEY_DOWN);
    input_set_capability(g_InputDevice, EV_KEY, KEY_LEFT);
    input_set_capability(g_InputDevice, EV_KEY, KEY_RIGHT);
    input_set_capability(g_InputDevice, EV_KEY, KEY_W);
    input_set_capability(g_InputDevice, EV_KEY, KEY_Z);
    input_set_capability(g_InputDevice, EV_KEY, KEY_V);
    input_set_capability(g_InputDevice, EV_KEY, KEY_O);
    input_set_capability(g_InputDevice, EV_KEY, KEY_M);
    input_set_capability(g_InputDevice, EV_KEY, KEY_C);
    input_set_capability(g_InputDevice, EV_KEY, KEY_E);
    input_set_capability(g_InputDevice, EV_KEY, KEY_S);
#endif //CONFIG_ENABLE_GESTURE_WAKEUP


#if defined(CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC)
    input_set_abs_params(g_InputDevice, ABS_MT_TRACKING_ID, 0, (MAX_TOUCH_NUM-1), 0, 0);
#endif //CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC
    input_set_abs_params(g_InputDevice, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(g_InputDevice, ABS_MT_WIDTH_MAJOR, 0, 15, 0, 0);
    input_set_abs_params(g_InputDevice, ABS_MT_POSITION_X, TOUCH_SCREEN_X_MIN, TOUCH_SCREEN_X_MAX, 0, 0);
    input_set_abs_params(g_InputDevice, ABS_MT_POSITION_Y, TOUCH_SCREEN_Y_MIN, TOUCH_SCREEN_Y_MAX, 0, 0);

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
//    set_bit(BTN_TOOL_FINGER, g_InputDevice->keybit);
//    input_set_abs_params(g_InputDevice, ABS_MT_SLOT, 0, MAX_TOUCH_NUM - 1, 0, 0);
    input_mt_init_slots(g_InputDevice, MAX_TOUCH_NUM,INPUT_MT_DIRECT | INPUT_MT_DROP_UNUSED);
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

    /* register the input device to input sub-system */
    nRetVal = input_register_device(g_InputDevice);
    if (nRetVal < 0)
    {
        GSE_LOG("*** Unable to register touch input device ***\n");
    }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    g_InputDevice = tpd->dev;
/*
    g_InputDevice->phys = "I2C";
    g_InputDevice->dev.parent = &pClient->dev;
    g_InputDevice->id.bustype = BUS_I2C;
    
    // set the supported event type for input device 
    set_bit(EV_ABS, g_InputDevice->evbit);
    set_bit(EV_SYN, g_InputDevice->evbit);
    set_bit(EV_KEY, g_InputDevice->evbit);
    set_bit(BTN_TOUCH, g_InputDevice->keybit);
    set_bit(INPUT_PROP_DIRECT, g_InputDevice->propbit);
*/

#ifdef CONFIG_TP_HAVE_KEY
    {
        u32 i;
        for (i = 0; i < MAX_KEY_NUM; i ++)
        {
            input_set_capability(g_InputDevice, EV_KEY, g_TpVirtualKey[i]);
        }
    }
#endif

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
    input_set_capability(g_InputDevice, EV_KEY, KEY_POWER);
    input_set_capability(g_InputDevice, EV_KEY, KEY_UP);
    input_set_capability(g_InputDevice, EV_KEY, KEY_DOWN);
    input_set_capability(g_InputDevice, EV_KEY, KEY_LEFT);
    input_set_capability(g_InputDevice, EV_KEY, KEY_RIGHT);
    input_set_capability(g_InputDevice, EV_KEY, KEY_W);
    input_set_capability(g_InputDevice, EV_KEY, KEY_Z);
    input_set_capability(g_InputDevice, EV_KEY, KEY_V);
    input_set_capability(g_InputDevice, EV_KEY, KEY_O);
    input_set_capability(g_InputDevice, EV_KEY, KEY_M);
    input_set_capability(g_InputDevice, EV_KEY, KEY_C);
    input_set_capability(g_InputDevice, EV_KEY, KEY_E);
    input_set_capability(g_InputDevice, EV_KEY, KEY_S);
#endif //CONFIG_ENABLE_GESTURE_WAKEUP


#if defined(CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC)
    input_set_abs_params(g_InputDevice, ABS_MT_TRACKING_ID, 0, (MAX_TOUCH_NUM-1), 0, 0);
#endif //CONFIG_ENABLE_TOUCH_DRIVER_FOR_MUTUAL_IC
/*
    input_set_abs_params(g_InputDevice, ABS_MT_TOUCH_MAJOR, 0, 255, 0, 0);
    input_set_abs_params(g_InputDevice, ABS_MT_WIDTH_MAJOR, 0, 15, 0, 0);
    input_set_abs_params(g_InputDevice, ABS_MT_POSITION_X, TOUCH_SCREEN_X_MIN, TOUCH_SCREEN_X_MAX, 0, 0);
    input_set_abs_params(g_InputDevice, ABS_MT_POSITION_Y, TOUCH_SCREEN_Y_MIN, TOUCH_SCREEN_Y_MAX, 0, 0);
*/

#ifdef CONFIG_ENABLE_TYPE_B_PROTOCOL
//    set_bit(BTN_TOOL_FINGER, g_InputDevice->keybit);
//    input_set_abs_params(g_InputDevice, ABS_MT_SLOT, 0, MAX_TOUCH_NUM - 1, 0, 0);
    input_mt_init_slots(g_InputDevice, MAX_TOUCH_NUM, INPUT_MT_DIRECT | INPUT_MT_DROP_UNUSED);
#endif //CONFIG_ENABLE_TYPE_B_PROTOCOL

#endif

    return nRetVal;    
}

s32 DrvPlatformLyrTouchDeviceRequestGPIO(void)
{
    s32 nRetVal = 0;

    GSE_LOG("*** %s() ***\n", __func__);
    
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    nRetVal = gpio_request(MS_TS_MSG_IC_GPIO_RST, "C_TP_RST");     
    if (nRetVal < 0)
    {
        GSE_LOG("*** Failed to request GPIO %d, error %d ***\n", MS_TS_MSG_IC_GPIO_RST, nRetVal);
    }

    nRetVal = gpio_request(MS_TS_MSG_IC_GPIO_INT, "C_TP_INT");    
    if (nRetVal < 0)
    {
        GSE_LOG("*** Failed to request GPIO %d, error %d ***\n", MS_TS_MSG_IC_GPIO_INT, nRetVal);
    }
#endif

    return nRetVal;    
}

s32 DrvPlatformLyrTouchDeviceRegisterFingerTouchInterruptHandler(void)
{
    s32 nRetVal = 0;

    GSE_LOG("*** %s() ***\n", __func__);

    if (1)
    {    	
#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
        /* initialize the finger touch work queue */ 
        INIT_WORK(&_gFingerTouchWork, _DrvPlatformLyrFingerTouchDoWork);

        _gIrq = gpio_to_irq(MS_TS_MSG_IC_GPIO_INT);

        /* request an irq and register the isr */
        nRetVal = request_threaded_irq(_gIrq/*MS_TS_MSG_IC_GPIO_INT*/, NULL, _DrvPlatformLyrFingerTouchInterruptHandler,
                      IRQF_TRIGGER_RISING | IRQF_ONESHOT/* | IRQF_NO_SUSPEND *//* IRQF_TRIGGER_FALLING */,
                      "msg2xxx", NULL); 

//        nRetVal = request_irq(_gIrq/*MS_TS_MSG_IC_GPIO_INT*/, _DrvPlatformLyrFingerTouchInterruptHandler,
//                      IRQF_TRIGGER_RISING /* | IRQF_NO_SUSPEND *//* IRQF_TRIGGER_FALLING */,
//                      "msg2xxx", NULL); 

        _gInterruptFlag = 1;
        
        if (nRetVal != 0)
        {
            GSE_LOG("*** Unable to claim irq %d; error %d ***\n", MS_TS_MSG_IC_GPIO_INT, nRetVal);
        }
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#if 0
        mt_set_gpio_mode(MS_TS_MSG_IC_GPIO_INT, GPIO_CTP_EINT_PIN_M_EINT);
        mt_set_gpio_dir(MS_TS_MSG_IC_GPIO_INT, GPIO_DIR_IN);
        mt_set_gpio_pull_enable(MS_TS_MSG_IC_GPIO_INT, GPIO_PULL_ENABLE);
        mt_set_gpio_pull_select(MS_TS_MSG_IC_GPIO_INT, GPIO_PULL_UP);

        mt_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);
        mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_TYPE/* EINTF_TRIGGER_RISING */, _DrvPlatformLyrFingerTouchInterruptHandler, 1);

        mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
#endif
       tpd_gpio_as_int(GTP_INT_PORT);
      DrvIcFwLyrIsRegisterFingerTouchInterruptHandler();
        _gInterruptFlag = 1;

#ifdef CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
        /* initialize the finger touch work queue */ 
        INIT_WORK(&_gFingerTouchWork, _DrvPlatformLyrFingerTouchDoWork);
#else
        _gThread = kthread_run(_DrvPlatformLyrFingerTouchHandler, 0, TPD_DEVICE);
        if (IS_ERR(_gThread))
        { 
            nRetVal = PTR_ERR(_gThread);
            GSE_LOG("Failed to create kernel thread: %d\n", nRetVal);
        }
#endif //CONFIG_USE_IRQ_INTERRUPT_FOR_MTK_PLATFORM
#endif
    }
    
    return nRetVal;    
}	
#if 0
void DrvPlatformLyrTouchDeviceRegisterEarlySuspend(void)
{
    GSE_LOG("*** %s() ***\n", __func__);

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
#ifdef CONFIG_ENABLE_NOTIFIER_FB
    _gFbNotifier.notifier_call = MsDrvInterfaceTouchDeviceFbNotifierCallback;
    fb_register_client(&_gFbNotifier);
#else
    _gEarlySuspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
    _gEarlySuspend.suspend = MsDrvInterfaceTouchDeviceSuspend;
    _gEarlySuspend.resume = MsDrvInterfaceTouchDeviceResume;
    register_early_suspend(&_gEarlySuspend);
#endif //CONFIG_ENABLE_NOTIFIER_FB   
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM || CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM
}
#endif
/* remove function is triggered when the input device is removed from input sub-system */
s32 DrvPlatformLyrTouchDeviceRemove(struct i2c_client *pClient)
{
    GSE_LOG("*** %s() ***\n", __func__);

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    free_irq(_gIrq, g_InputDevice);
    gpio_free(MS_TS_MSG_IC_GPIO_INT);
    gpio_free(MS_TS_MSG_IC_GPIO_RST);
    input_unregister_device(g_InputDevice);
#endif    

    if (IS_FIRMWARE_DATA_LOG_ENABLED)
    {    	
        if (g_TouchKSet)
        {
            kset_unregister(g_TouchKSet);
            g_TouchKSet = NULL;
        }
    
        if (g_TouchKObj)
        {
            kobject_put(g_TouchKObj);
            g_TouchKObj = NULL;
        }
    } //IS_FIRMWARE_DATA_LOG_ENABLED

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#ifdef CONFIG_ENABLE_GESTURE_DEBUG_MODE
    if (g_GestureKSet)
    {
        kset_unregister(g_GestureKSet);
        g_GestureKSet = NULL;
    }
    
    if (g_GestureKObj)
    {
        kobject_put(g_GestureKObj);
        g_GestureKObj = NULL;
    }
#endif //CONFIG_ENABLE_GESTURE_DEBUG_MODE
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

    DrvMainRemoveProcfsDirEntry();

#ifdef CONFIG_ENABLE_HOTKNOT
    DeleteQueue();
    DeleteHotKnotMem();
    GSE_LOG("Deregister hotknot misc device.\n");
    misc_deregister( &hotknot_miscdevice );   
#endif //CONFIG_ENABLE_HOTKNOT

#ifdef CONFIG_ENABLE_JNI_INTERFACE
    DeleteMsgToolMem();
#endif //CONFIG_ENABLE_JNI_INTERFACE

#ifdef CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM
#ifdef CONFIG_ENABLE_DMA_IIC
    DmaFree();
#endif //CONFIG_ENABLE_DMA_IIC
#endif //CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM

    return 0;
}

void DrvPlatformLyrSetIicDataRate(struct i2c_client *pClient, u32 nIicDataRate)
{
    GSE_LOG("*** %s() nIicDataRate = %d ***\n", __func__, nIicDataRate);

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM)
    // TODO : Please FAE colleague to confirm with customer device driver engineer for how to set i2c data rate on SPRD platform
    sprd_i2c_ctl_chg_clk(pClient->adapter->nr, nIicDataRate); 
    mdelay(100);
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
    // TODO : Please FAE colleague to confirm with customer device driver engineer for how to set i2c data rate on QCOM platform
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
    pClient->timing = nIicDataRate/1000;
#endif
}

//------------------------------------------------------------------------------//

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION

/*Begin ersen.shang add parameters and modify the return value for cts verifier testBatchAndFlush Fail*/
int DrvPlatformLyrGetTpPsData(int* value, int* status)
{
    GSE_LOG("*** %s() g_FaceClosingTp = %d ***\n", __func__, g_FaceClosingTp);
	
    //return g_FaceClosingTp;
    *value = g_FaceClosingTp;
	*status = 3;

	return 0;
}
/*Ene   ersen.shang add parameters and modify the return value for cts verifier testBatchAndFlush Fail*/

int msg22xx_pls_enable(int nEnable)
{
    GSE_LOG("*** %s() nEnable = %d ***\n", __func__, nEnable);

    if (nEnable)
    {
       return DrvIcFwLyrEnableProximity();
    }
    else
    {
      return  DrvIcFwLyrDisableProximity();
    }
}

int get_msg22xx_data (int* value, int* status)
{
    /*Begin ersen.shang add parameters and modify the return value for cts verifier testBatchAndFlush Fail*/
	return DrvPlatformLyrGetTpPsData(value , status);
    /*End   ersen.shang add parameters and modify the return value for cts verifier testBatchAndFlush Fail*/

}

#if defined(CONFIG_TOUCH_DRIVER_RUN_ON_SPRD_PLATFORM) || defined(CONFIG_TOUCH_DRIVER_RUN_ON_QCOM_PLATFORM)
void DrvPlatformLyrTpPsEnable(int nEnable)
{
    GSE_LOG("*** %s() nEnable = %d ***\n", __func__, nEnable);

    if (nEnable)
    {
        DrvIcFwLyrEnableProximity();
    }
    else
    {
        DrvIcFwLyrDisableProximity();
    }
}
#elif defined(CONFIG_TOUCH_DRIVER_RUN_ON_MTK_PLATFORM)
#if 0
int DrvPlatformLyrTpPsOperate(void* pSelf, u32 nCommand, void* pBuffIn, int nSizeIn,
				   void* pBuffOut, int nSizeOut, int* pActualOut)
{
    int nErr = 0;
    int nValue;
    hwm_sensor_data *pSensorData;

    switch (nCommand)
    {
        case SENSOR_DELAY:
            if ((pBuffIn == NULL) || (nSizeIn < sizeof(int)))
            {
                nErr = -EINVAL;
            }
            // Do nothing
            break;

        case SENSOR_ENABLE:
            if ((pBuffIn == NULL) || (nSizeIn < sizeof(int)))
            {
                nErr = -EINVAL;
            }
            else
            {
                nValue = *(int *)pBuffIn;
                if (nValue)
                {
                    if (DrvIcFwLyrEnableProximity() < 0)
                    {
                        GSE_LOG("Enable ps fail: %d\n", nErr);
                        return -1;
                    }
                }
                else
                {
                    if (DrvIcFwLyrDisableProximity() < 0)
                    {
                        GSE_LOG("Disable ps fail: %d\n", nErr);
                        return -1;
                    }
                }
            }
            break;

        case SENSOR_GET_DATA:
            if ((pBuffOut == NULL) || (nSizeOut < sizeof(hwm_sensor_data)))
            {
                GSE_LOG("Get sensor data parameter error!\n");
                nErr = -EINVAL;
            }
            else
            {
                pSensorData = (hwm_sensor_data *)pBuffOut;

                pSensorData->values[0] = DrvPlatformLyrGetTpPsData();
                pSensorData->value_divide = 1;
                pSensorData->status = SENSOR_STATUS_ACCURACY_MEDIUM;
            }
            break;

       default:
           GSE_LOG("Un-recognized parameter %d!\n", nCommand);
           nErr = -1;
           break;
    }

    return nErr;
}
#endif
#endif
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

//------------------------------------------------------------------------------//

