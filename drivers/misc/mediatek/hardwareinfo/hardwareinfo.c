/* Huaqin  Inc. (C) 2011. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("HUAQIN SOFTWARE")
 * RECEIVED FROM HUAQIN AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. HUAQIN EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES HUAQIN PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE HUAQIN SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN HUAQIN SOFTWARE. HUAQIN SHALL ALSO NOT BE RESPONSIBLE FOR ANY HUAQIN
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND HUAQIN'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE HUAQIN SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT HUAQIN'S OPTION, TO REVISE OR REPLACE THE HUAQIN SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * HUAQIN FOR SUCH HUAQIN SOFTWARE AT ISSUE.
 *

 */


/*******************************************************************************
* Dependency
*******************************************************************************/

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
#include <linux/mtd/mtd.h>
//#include <linux/mtd/nand.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/proc_fs.h>
#include "lcm_drv.h"
//#include "../../../drivers/input/touchscreen/mediatek/tpd.h"
#include "tpd.h"
extern LCM_DRIVER  *g_lcm_drv; 
//extern char *lcm_temp_name; //temp lcm name 

//extern flashdev_info devinfo;
extern char *g_main_camera;
extern char *g_sub_camera;
/****************************************************************************** 
 * Function Configuration
******************************************************************************/


/****************************************************************************** 
 * Debug configuration
******************************************************************************/


//hardware info driver
#if 0
static ssize_t show_hardware_info(struct device *dev,struct device_attribute *attr, char *buf)
{   

    return 0;
}
#endif

static ssize_t show_lcm(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    if(g_lcm_drv != NULL)
    	ret_value = sprintf(buf, "lcd name    :%s\n", g_lcm_drv->name);  
	else
    	ret_value = sprintf(buf, "lcd name    :%s\n", "not_found");  

    return ret_value;
}
static ssize_t store_lcm(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[%s]: Not Support Write Function\n",__func__);  
    return size;
}


extern struct tpd_device  *tpd;
extern struct tpd_driver_t *g_tpd_drv;



static ssize_t show_ctp(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;

//begin-20150602-fangjie-modify-for-minisw tp Rawdata test.
//    old: ret_value = sprintf(buf + count, "ctp driver  :%s\n", g_tpd_drv->tpd_device_name);
     if(g_tpd_drv)
     {
//begin 20160310 liujunting modify for tp0 issue
	ret_value = sprintf(buf, "%s\n", g_tpd_drv->tpd_device_name);//20160226 liujunting modify for hardwareinfo
        ret_value += sprintf(buf+ret_value, "tp:%s;", g_tpd_drv->tpd_device_name);
//end 20160310 liujunting modify for tp0 issue
        ret_value += sprintf(buf+ret_value, "FW_version:0x%x;", tpd->tp_firmware_version); //20160226 liujunting add for hardwareinfo
        ret_value += sprintf(buf+ret_value, "vendor_id:0x%x\n", tpd->tp_vendor_id); //20160226 liujunting add for hardwareinfo
     }
	else
        ret_value = sprintf(buf, "NULL\n");	
//end-20150602-fangjie-modify-for-minisw tp Rawdata test.

    //count += sprintf(buf + count, "ctp version :0x%x\n", tpd->dev->id.version); 
    //count += sprintf(buf + count, "ctp version :%x\n", tpd->hq_ctp_firmware_version); 

    //if(tpd->hq_ctp_module_id)
   //     count += sprintf(buf + count, "ctp modlue id  :%d\n", tpd->hq_ctp_module_id);
   // if(tpd->hq_ctp_module_name)
  //      count += sprintf(buf + count, "ctp modlue  :%s\n", tpd->hq_ctp_module_name);
    
   // ret_value = count;
    
    return ret_value;
}
static ssize_t store_ctp(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[%s]: Not Support Write Function\n",__func__);  
    return size;
}


static ssize_t show_main_camera(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;

        ret_value = sprintf(buf , "main camera :%s\n", g_main_camera);
  
    return ret_value;
}
static ssize_t store__main_camera(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[%s]: Not Support Write Function\n",__func__);  
    return size;
}


static ssize_t show_sub_camera(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = sprintf(buf , "sub camera  :%s\n", g_sub_camera);
    
    return ret_value;
}
static ssize_t store_sub_camera(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[%s]: Not Support Write Function\n",__func__);  
    return size;
}




static ssize_t show_wifi(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = sprintf(buf, "wifi name   :MT6580\n");     
    return ret_value;
}
static ssize_t show_bt(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = sprintf(buf, "bt name     :MT6580\n");     
    return ret_value;
}
static ssize_t show_gps(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = sprintf(buf, "GPS name    :MT6580\n");     
    return ret_value;
}
static ssize_t show_fm(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    ret_value = sprintf(buf, "FM name     :MT6580\n");     
    return ret_value;
}




static ssize_t store_wifi(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[%s]: Not Support Write Function\n",__func__);  
    return size;
}
static ssize_t store_bt(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[%s]: Not Support Write Function\n",__func__);  
    return size;
}
static ssize_t store_gps(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[%s]: Not Support Write Function\n",__func__);  
    return size;
}
static ssize_t store_fm(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[%s]: Not Support Write Function\n",__func__);  
    return size;
}

char *g_alsps_name  = NULL;
static ssize_t show_alsps(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    #if defined(CONFIG_CUSTOM_KERNEL_ALSPS)
    if(g_alsps_name)
//begin-20151105-yuduan.xie-modify-for-mini Task 799154
        //ret_value = sprintf(buf, "AlSPS name  :%s\n",g_alsps_name);
    	ret_value = sprintf(buf, "%s\n",g_alsps_name);
//end-20151105-yuduan.xie-modify-for-mini Task 799154
    else
        ret_value = sprintf(buf, "AlSPS name  :Not found\n");
    #else
    ret_value = sprintf(buf, "AlSPS name  :Not support ALSPS\n");   
    #endif
    return ret_value;
}

static ssize_t store_alsps(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[%s]: Not Support Write Function\n",__func__);  
    return size;
}


char *g_gsensor_name  = NULL;
static ssize_t show_gsensor(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    #if defined(CONFIG_CUSTOM_KERNEL_ACCELEROMETER)
    if(g_gsensor_name)
        ret_value = sprintf(buf, "GSensor name:%s\n",g_gsensor_name);   
    else
        ret_value = sprintf(buf, "GSensor name:Not found\n"); 
    #else
    ret_value = sprintf(buf, "GSensor name:Not support GSensor\n");     
    #endif
    return ret_value;
}

static ssize_t store_gsensor(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[%s]: Not Support Write Function\n",__func__);  
    return size;
}

char *g_msensor_name  = NULL;
static ssize_t show_msensor(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    #if defined(CONFIG_CUSTOM_KERNEL_MAGNETOMETER)
    if(g_msensor_name)
        ret_value = sprintf(buf, "MSensor name:%s\n",g_msensor_name);   
    else
        ret_value = sprintf(buf, "MSensor name:Not found\n"); 
    #else
    ret_value = sprintf(buf, "MSensor name:Not support MSensor\n");     
    #endif
    return ret_value;
}

static ssize_t store_msensor(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[%s]: Not Support Write Function\n",__func__);  
    return size;
}

char *g_Gyro_name  = NULL;
static ssize_t show_gyro(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret_value = 1;
    #if defined(CONFIG_CUSTOM_KERNEL_GYROSCOPE)
    if(g_Gyro_name)
        ret_value = sprintf(buf, "Gyro  name  :%s\n",g_Gyro_name);  
    else
        ret_value = sprintf(buf, "Gyro  name  :Not found\n"); 
    #else
    ret_value = sprintf(buf, "Gyro  name  :Not support Gyro\n");    
    #endif
    return ret_value;
}

static ssize_t store_gyro(struct device *dev,struct device_attribute *attr, const char *buf, size_t size)
{
    printk("[%s]: Not Support Write Function\n",__func__);  
    return size;
}





static DEVICE_ATTR(lcm, 0644, show_lcm, store_lcm);
static DEVICE_ATTR(ctp, 0644, show_ctp, store_ctp);
static DEVICE_ATTR(main_camera, 0644, show_main_camera, store__main_camera);
static DEVICE_ATTR(sub_camera, 0644, show_sub_camera, store_sub_camera);

//static DEVICE_ATTR(04_flash, 0666, show_flash, store_flash);
static DEVICE_ATTR(gsensor, 0644, show_gsensor, store_gsensor);
static DEVICE_ATTR(msensor, 0644, show_msensor, store_msensor);
static DEVICE_ATTR(gyro, 0644, show_gyro, store_gyro);


static DEVICE_ATTR(alsps, 0644, show_alsps, store_alsps);


static DEVICE_ATTR(wifi, 0644, show_wifi, store_wifi);
static DEVICE_ATTR(bt, 0644, show_bt, store_bt);
static DEVICE_ATTR(gps, 0644, show_gps, store_gps);
static DEVICE_ATTR(fm, 0644, show_fm, store_fm);


///////////////////////////////////////////////////////////////////////////////////////////
//// platform_driver API 
///////////////////////////////////////////////////////////////////////////////////////////
static int HardwareInfo_driver_probe(struct platform_device *dev)   
{   
    int ret_device_file = 0;

    printk("** HardwareInfo_driver_probe!! **\n" );
    if((ret_device_file = device_create_file(&(dev->dev), &dev_attr_lcm)) != 0) goto exit_error;
    if((ret_device_file = device_create_file(&(dev->dev), &dev_attr_ctp)) != 0) goto exit_error;
    if((ret_device_file = device_create_file(&(dev->dev), &dev_attr_main_camera)) != 0) goto exit_error;
    if((ret_device_file = device_create_file(&(dev->dev), &dev_attr_sub_camera)) != 0) goto exit_error;
  
    //if((ret_device_file = device_create_file(&(dev->dev), &dev_attr_04_flash)) != 0) goto exit_error;
    if((ret_device_file = device_create_file(&(dev->dev), &dev_attr_gsensor)) != 0) goto exit_error;
    if((ret_device_file = device_create_file(&(dev->dev), &dev_attr_msensor)) != 0) goto exit_error;
    if((ret_device_file = device_create_file(&(dev->dev), &dev_attr_gyro)) != 0) goto exit_error;
    if((ret_device_file = device_create_file(&(dev->dev), &dev_attr_alsps)) != 0) goto exit_error;
    if((ret_device_file = device_create_file(&(dev->dev), &dev_attr_wifi)) != 0) goto exit_error;
    if((ret_device_file = device_create_file(&(dev->dev), &dev_attr_bt)) != 0) goto exit_error;
    if((ret_device_file = device_create_file(&(dev->dev), &dev_attr_gps)) != 0) goto exit_error;
    if((ret_device_file = device_create_file(&(dev->dev), &dev_attr_fm)) != 0) goto exit_error;
    
exit_error: 
    return ret_device_file;
}

static int HardwareInfo_driver_remove(struct platform_device *dev)
{
    printk("** HardwareInfo_drvier_remove!! **");

    device_remove_file(&(dev->dev), &dev_attr_lcm);
 //   device_remove_file(&(dev->dev), &dev_attr_ctp);
    device_remove_file(&(dev->dev), &dev_attr_main_camera);
    device_remove_file(&(dev->dev), &dev_attr_sub_camera);
    
    //device_remove_file(&(dev->dev), &dev_attr_04_flash);
    device_remove_file(&(dev->dev), &dev_attr_gsensor);
    device_remove_file(&(dev->dev), &dev_attr_msensor);
    device_remove_file(&(dev->dev), &dev_attr_gyro);
    device_remove_file(&(dev->dev), &dev_attr_alsps);
    device_remove_file(&(dev->dev), &dev_attr_wifi);
    device_remove_file(&(dev->dev), &dev_attr_bt);
    device_remove_file(&(dev->dev), &dev_attr_gps);
    device_remove_file(&(dev->dev), &dev_attr_fm);
    return 0;
}





static struct platform_driver HardwareInfo_driver = {
    .probe      = HardwareInfo_driver_probe,
    .remove     = HardwareInfo_driver_remove,
    .driver     = {
        .name = "HardwareInfo",
    },
};

static struct platform_device HardwareInfo_device = {
    .name   = "HardwareInfo",
    .id     = -1,
};




static int __init HardwareInfo_mod_init(void)
{
    int ret = 0;


    ret = platform_device_register(&HardwareInfo_device);
    if (ret) {
        printk("**HardwareInfo_mod_init  Unable to driver register(%d)\n", ret);
        goto  fail_2;
    }
    

    ret = platform_driver_register(&HardwareInfo_driver);
    if (ret) {
        printk("**HardwareInfo_mod_init  Unable to driver register(%d)\n", ret);
        goto  fail_1;
    }

    goto ok_result;

    
fail_1:
    platform_driver_unregister(&HardwareInfo_driver);
fail_2:
    platform_device_unregister(&HardwareInfo_device);
ok_result:

    return ret;
}


/*****************************************************************************/
static void __exit HardwareInfo_mod_exit(void)
{
    platform_driver_unregister(&HardwareInfo_driver);
    platform_device_unregister(&HardwareInfo_device);
}
/*****************************************************************************/
module_init(HardwareInfo_mod_init);
module_exit(HardwareInfo_mod_exit);
/*****************************************************************************/
MODULE_AUTHOR("xiaopu.zhu <xiaopu.zhu@tcl.com>");
MODULE_DESCRIPTION(" Hareware Info driver");
MODULE_LICENSE("GPL");



