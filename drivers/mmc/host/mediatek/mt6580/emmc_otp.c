#include <generated/autoconf.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/timer.h>
#include <linux/ioport.h>
#include <linux/device.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/blkdev.h>
#include <linux/mmc/host.h>
#include <linux/mmc/card.h>
#include <linux/mmc/core.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>
#include <linux/mmc/sdio.h>
#include <linux/dma-mapping.h>

#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

//add begin yan.fang 1120 modify for task742725,742727
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <asm/segment.h>
#include <linux/buffer_head.h>
//add end  yan.fang 1120 modify for task742725,742727
#include "mt_sd.h"
#include "emmc_otp.h"
#include <mt-plat/partition.h>

#define DRV_NAME_MISC			"otp"
#define PROCNAME				 "driver/otp"

#ifndef CONFIG_MTK_EMMC_SUPPORT_OTP
#define CONFIG_MTK_EMMC_SUPPORT_OTP
#endif

#define EMMC_OTP_DEBUG		   1

/* static spinlock_t			   g_emmc_otp_lock; */
static struct emmc_otp_config g_emmc_otp_func;
static unsigned int sg_wp_size;	/* byte unit */
static unsigned int g_otp_user_ccci = 2 * 1024 * 1024;
//add begin  yan.fang 1120 modify for task742725,742727
// remember open the macro in ProjectConfig.mk
// MTK_EMMC_SUPPORT_OTP = yes


typedef struct
{
	unsigned char imei[8];
	unsigned char imei_checksum[8];
	unsigned char res[16];
} OTP_LAYOUT;

typedef struct
{
	unsigned char imei[8];
	unsigned char svn;
	unsigned char pad;
} IMEI_SVN;

#define   INPUT_IMEI_OFFSET    10
// make sure same as  the tools team
#define   OTP_IMEI_OFFSET        32  


//old:#define TRACE_REGION "/dev/block/platform/mtk-msdc.0/by-name/proinfo" //"/dev/pro_info"
//modify by fangyan according pixi4-4 proinfo path
#define TRACE_REGION "/dev/block/platform/mtk-msdc.0/11120000.msdc0/by-name/proinfo"
#define TRACE_REGION_SZ (0x200)
#define TRACE_ID_OFFSET (0x169)

typedef struct {
    unsigned int group;
    unsigned int offset;
    int          cached;
} id_offset_t;
static id_offset_t g_id_offset;

#define  IS_SUPPORT_IMEI_REFURBISH    1

static void id_offset(struct otp_ctl *p) {
    int fd;
    char buffer[TRACE_REGION_SZ];
    char bf;
    int len;

    g_id_offset.offset = p->Offset;
	
    if(1 == g_id_offset.cached)
        goto ready;

    mm_segment_t oldfs = get_fs();
    set_fs(KERNEL_DS);

    fd = sys_open(TRACE_REGION, O_RDONLY, 0);
    if(fd >= 0) {
        /*default lseek*/
        len = sys_read(fd, buffer, TRACE_REGION_SZ);
        if (len < TRACE_REGION_SZ) {
            printk(KERN_DEBUG "[EMMC_OTP]read %s file failed, len=%d\n", TRACE_REGION, len);
            goto fail;
        } else {
            printk(KERN_DEBUG "[EMMC_OTP]read %s ok, len=%d\n", TRACE_REGION, len);
        }
        sys_close(fd);
    } else {
        printk(KERN_DEBUG "[EMMC_OTP]can not open %s file\n", TRACE_REGION);
        goto fail;
    }

    set_fs(oldfs);

    bf = buffer[TRACE_ID_OFFSET] & 0xF; /*go encrypted?*/
    if (bf == 0x0 || bf == 0x1) {
        printk(KERN_DEBUG "[EMMC_OTP]use id group %d\n", bf);
        g_id_offset.group = bf;
        goto ready;
    }

fail:
    g_id_offset.group = 0x0;
ready:
    printk(KERN_ERR "[EMMC_OTP]g_id_offset.offset is 0x%x\n", g_id_offset.offset);
    p->Offset = (g_id_offset.offset/0xA)*0x20;
    
    #if  IS_SUPPORT_IMEI_REFURBISH
     p->Offset = p->Offset  +  g_id_offset.group*0x80;
    #endif 
    printk(KERN_ERR "[EMMC_OTP]use id offset 0x%x\n", p->Offset);
    // added by yongming.li for Latam requirement 
    g_id_offset.offset = p->Offset;
    
    g_id_offset.cached = 1;

}



static void  dumpBuf(void * buf)
{
   unsigned char * temp;
   
   temp=(char *)buf;

   int i=0;
      printk(KERN_ERR  "[%s] \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n",__func__,  \
	   	temp[i+0],temp[i+1],temp[i+2],temp[i+3],temp[i+4],temp[i+5],temp[i+6],temp[i+7], \
	   	temp[i+8],temp[i+9],temp[i+10],temp[i+11],temp[i+12],temp[i+13],temp[i+14],temp[i+15],\
	   	temp[i+16],temp[i+17],temp[i+18],temp[i+19],temp[i+20],temp[i+21],temp[i+22],temp[i+23],\
	   	temp[i+24],temp[i+25],temp[i+26],temp[i+27],temp[i+28],temp[i+29],temp[i+30],temp[i+31],\
	   	temp[i+32],temp[i+33],temp[i+34],temp[i+35],temp[i+36],temp[i+37],temp[i+38],temp[i+39],\
	   	temp[i+40],temp[i+41],temp[i+42],temp[i+43],temp[i+44],temp[i+45],temp[i+46],temp[i+47],\
	   	temp[i+48],temp[i+49],temp[i+50],temp[i+51],temp[i+52],temp[i+53],temp[i+54],temp[i+55],\
	   	temp[i+56],temp[i+57],temp[i+58],temp[i+59],temp[i+60],temp[i+61],temp[i+62],temp[i+63],\
	   	temp[i+64],temp[i+65],temp[i+66],temp[i+67],temp[i+68],temp[i+69],temp[i+70],temp[i+71],\
	   	temp[i+72],temp[i+73],temp[i+74],temp[i+75],temp[i+76],temp[i+77],temp[i+78],temp[i+79],\
	   	temp[i+80],temp[i+81],temp[i+82],temp[i+83],temp[i+84],temp[i+85],temp[i+86],temp[i+87],\
	   	temp[i+88],temp[i+89],temp[i+90],temp[i+91],temp[i+92],temp[i+93],temp[i+94],temp[i+95],\
	   	temp[i+96],temp[i+97],temp[i+98],temp[i+99],temp[i+100],temp[i+101],temp[i+102],temp[i+103],\
	   	temp[i+104],temp[i+105],temp[i+106],temp[i+107],temp[i+108],temp[i+109],temp[i+110],temp[i+111],\
	   	temp[i+112],temp[i+113],temp[i+114],temp[i+115],temp[i+116],temp[i+117],temp[i+118],temp[i+119],\
	   	temp[i+120],temp[i+121],temp[i+122],temp[i+123],temp[i+124],temp[i+125],temp[i+126],temp[i+127]);
     return;
temp=(char *)buf+128;

    printk(KERN_ERR  "[%s+128+0x80] \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n \
%02x %02x %02x %02x %02x %02x %02x %02x \r\n",__func__,  \
	   	temp[i+0],temp[i+1],temp[i+2],temp[i+3],temp[i+4],temp[i+5],temp[i+6],temp[i+7], \
	   	temp[i+8],temp[i+9],temp[i+10],temp[i+11],temp[i+12],temp[i+13],temp[i+14],temp[i+15],\
	   	temp[i+16],temp[i+17],temp[i+18],temp[i+19],temp[i+20],temp[i+21],temp[i+22],temp[i+23],\
	   	temp[i+24],temp[i+25],temp[i+26],temp[i+27],temp[i+28],temp[i+29],temp[i+30],temp[i+31],\
	   	temp[i+32],temp[i+33],temp[i+34],temp[i+35],temp[i+36],temp[i+37],temp[i+38],temp[i+39],\
	   	temp[i+40],temp[i+41],temp[i+42],temp[i+43],temp[i+44],temp[i+45],temp[i+46],temp[i+47],\
	   	temp[i+48],temp[i+49],temp[i+50],temp[i+51],temp[i+52],temp[i+53],temp[i+54],temp[i+55],\
	   	temp[i+56],temp[i+57],temp[i+58],temp[i+59],temp[i+60],temp[i+61],temp[i+62],temp[i+63],\
	   	temp[i+64],temp[i+65],temp[i+66],temp[i+67],temp[i+68],temp[i+69],temp[i+70],temp[i+71],\
	   	temp[i+72],temp[i+73],temp[i+74],temp[i+75],temp[i+76],temp[i+77],temp[i+78],temp[i+79],\
	   	temp[i+80],temp[i+81],temp[i+82],temp[i+83],temp[i+84],temp[i+85],temp[i+86],temp[i+87],\
	   	temp[i+88],temp[i+89],temp[i+90],temp[i+91],temp[i+92],temp[i+93],temp[i+94],temp[i+95],\
	   	temp[i+96],temp[i+97],temp[i+98],temp[i+99],temp[i+100],temp[i+101],temp[i+102],temp[i+103],\
	   	temp[i+104],temp[i+105],temp[i+106],temp[i+107],temp[i+108],temp[i+109],temp[i+110],temp[i+111],\
	   	temp[i+112],temp[i+113],temp[i+114],temp[i+115],temp[i+116],temp[i+117],temp[i+118],temp[i+119],\
	   	temp[i+120],temp[i+121],temp[i+122],temp[i+123],temp[i+124],temp[i+125],temp[i+126],temp[i+127]);

}


void OTP_IMEI_Write (void *BufferPtr)
{
	int i, j;
	OTP_LAYOUT  * otp_layout;

	printk(KERN_ERR "[%s]: begin begin begin begin begin begin begin begin begin begin begin begin begin\n", __func__);
       dumpBuf(BufferPtr);
       otp_layout= (OTP_LAYOUT*)((unsigned char*)BufferPtr+g_id_offset.offset);
	for (j=0; j<7/*8*/; j++)
	{ 

		if (0xFF == otp_layout->imei[j])
		{
		        printk(KERN_ERR"[%s]: invalid imei serial number,please try again j is %d\n", __func__,j);
			 return  -1;
		}
	}

	for (j=0; j<8; j++)
	{

	    otp_layout->imei_checksum[j] = 0xFF-otp_layout->imei[j];
	}
	printk(KERN_ERR "[%s]: ---------------------------------------------------------------------------\n", __func__);
	printk(KERN_ERR "[%s]: ---------------------------------------------------------------------------\n", __func__);
  dumpBuf(BufferPtr);   
       
  printk(KERN_ERR "[%s]: end end end end end end end end end end end end end end end end end end end end \n", __func__);
}


void OTP_IMEI_Read (void *BufferPtr)
{
	int  j;
	unsigned char bValid = true;
	OTP_LAYOUT *otp_layout;
	IMEI_SVN imei_svn;
	memset(imei_svn.imei, 0xff,sizeof(IMEI_SVN));

       printk(KERN_ERR "[%s]: begin begin begin begin begin begin begin begin begin begin begin begin begin\n", __func__);
       dumpBuf(BufferPtr);
	
	    otp_layout= (OTP_LAYOUT*)((unsigned char*)BufferPtr+g_id_offset.offset);
       

	for (j=0, bValid = true; j<8; j++)
	{
		if ((otp_layout->imei[j]+otp_layout->imei_checksum[j]) != 0xFF)
		{
			bValid = false;
			printk(KERN_ERR "[%s]:imei[%d] =%x, imei_checksum =%x \n", __func__, j,otp_layout->imei[j],otp_layout->imei_checksum[j]);
			break;
		}
	}
	if (bValid == true)
	{
		memcpy(imei_svn.imei, otp_layout->imei, 8);
		imei_svn.svn = 0x01;
		imei_svn.pad = 0x00;
	}
	// FR572306 added by yongming.li for otp
	memset(otp_layout, 0xff, INPUT_IMEI_OFFSET);
  memcpy(otp_layout, imei_svn.imei,INPUT_IMEI_OFFSET);
	
	printk(KERN_ERR "[%s]: ---------------------------------------------------------------------------\n", __func__);
	printk(KERN_ERR "[%s]: ---------------------------------------------------------------------------\n", __func__);
	
	dumpBuf(BufferPtr);
	printk(KERN_ERR "[%s]: end end end end end end end end end end end end end end end end end end end end \n", __func__);
	
}

//add end  yan.fang 1120 modify for task742725,742727

struct msdc_host *emmc_otp_get_host(void)
{
	return mtk_msdc_host[EMMC_HOST_NUM];
}


/******************************************************************************
 * EMMC OTP operations
 * ***************************************************************************/
unsigned int emmc_get_wp_size(void)
{
	unsigned char l_ext_csd[512];
	struct msdc_host *host_ctl;
	u32 *resp = NULL;
	unsigned int write_prot_grpsz = 0;

	/* not to change ERASE_GRP_DEF after card initialized */
	host_ctl = emmc_otp_get_host();

	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);
	BUG_ON(!host_ctl->mmc->card);

	if (0 != sg_wp_size) {
		pr_debug("msdc[%s:%d]mmc_host = 0x%p,sg_wp_size=%d\n", __func__,
			__LINE__, host_ctl->mmc, sg_wp_size);
		return sg_wp_size;
	}

	pr_debug("[%s:%d]mmc_host = 0x%p\n", __func__, __LINE__, host_ctl->mmc);

	mmc_claim_host(host_ctl->mmc);

	/* pr_debug("[%s:%d]claim host done!\n", __func__, __LINE__);
	 * pr_debug("claim_status=%d, claim_cnt=%d, claimer=0x%x, current=0x%x\n",
	 *	host_ctl->mmc->claimed, host_ctl->mmc->claim_cnt, host_ctl->mmc->claimer
	 *	, current);
	 */

	/*
	 * As the ext_csd is so large and mostly unused, we don't store the
	 * raw block in mmc_card, so re-get it here.
	 */
	mmc_send_ext_csd(host_ctl->mmc->card, l_ext_csd);

#if EMMC_OTP_DEBUG
	{
		int i;

		for (i = 0; i < 512; i++) {
			pr_debug("%x", l_ext_csd[i]);
			if (0 == ((i + 1) % 16))
				pr_debug("\n");
		}
	}
#endif

	mmc_release_host(host_ctl->mmc);

	/* otp length equal to one write protect group size */
	if (l_ext_csd[EXT_CSD_ERASE_GROUP_DEF] & 0x1) {
		/* use high-capacity erase uint size, hc erase timeout, hc wp size,
		 * store in EXT_CSD */
		sg_wp_size =
			(512 * 1024 * l_ext_csd[EXT_CSD_HC_ERASE_GRP_SIZE] *
			 l_ext_csd[EXT_CSD_HC_WP_GRP_SIZE]);
	} else {
		resp = host_ctl->mmc->card->raw_csd;
		write_prot_grpsz = UNSTUFF_BITS(resp, 32, 5);
		/* use old erase group size,write protect group size, store in CSD*/
		sg_wp_size = (512 * host_ctl->mmc->card->erase_size)*
					(write_prot_grpsz + 1);
	}
	pr_debug("write_protect_group_size = %d MByte %s:%d\n", sg_wp_size / 1048576);

	return sg_wp_size;
}

/* need subtract emmc reserved region for combo feature */
unsigned int emmc_otp_start(void)
{
	unsigned int l_addr;
/*	unsigned int l_offset;*/	  /* for emmc combo reserved */
/*	unsigned int l_reserve;*/
	struct msdc_host *host_ctl;
	struct hd_struct *lp_hd_struct = NULL;

	if (0 == sg_wp_size)
		emmc_get_wp_size();

	host_ctl = emmc_otp_get_host();

	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);
	BUG_ON(!host_ctl->mmc->card);

	lp_hd_struct = get_part("otp");
	if (NULL == lp_hd_struct) {
		pr_debug("not find otp in scatter file error!\n");
		put_part(lp_hd_struct);
		return -EFAULT;
	}

	l_addr = lp_hd_struct->start_sect;	  /* block unit */

	/* find wp group start address in 43MB reserved region */
	if (l_addr % (sg_wp_size >> 9))
		l_addr += ((sg_wp_size >> 9) - l_addr % (sg_wp_size >> 9));

	if (NULL != lp_hd_struct)
		put_part(lp_hd_struct);

	pr_debug("find OTP start address is 0x%x\n", l_addr);

	return l_addr;
}

unsigned int emmc_otp_query_length(unsigned int *QLength)
{
	/* otp length equal to one write protect group size */
	*QLength = emmc_get_wp_size() - g_otp_user_ccci;

	return 0;
}

unsigned int emmc_otp_read(unsigned int blk_offset, void *BufferPtr)
{
	char l_buf[512];
	unsigned char *l_rcv_buf = (unsigned char *)BufferPtr;
	unsigned int l_addr;
	unsigned int l_otp_size;
	unsigned int l_ret;
	struct scatterlist msdc_sg;
	struct mmc_data msdc_data;
	struct mmc_command msdc_cmd;
	struct mmc_request msdc_mrq;
	struct msdc_host *host_ctl;

	/* check parameter */
	l_addr = emmc_otp_start();
	l_otp_size = emmc_get_wp_size();
	if (blk_offset > (l_otp_size >> 9))
		return OTP_ERROR_OVERSCOPE;

	l_addr += blk_offset;

	host_ctl = emmc_otp_get_host();

	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);

	pr_debug("[%s:%d]mmc_host is : 0x%p\n", __func__, __LINE__, host_ctl->mmc);
	mmc_claim_host(host_ctl->mmc);

	/* make sure access user data area */
	mmc_send_ext_csd(host_ctl->mmc->card, l_buf);
	if (l_buf[179] & 0x7) {
		l_buf[179] &= ~0x7;
		l_buf[179] |= 0x0;
		mmc_switch(host_ctl->mmc->card, 0, 179, l_buf[179], 1000);
	}

#if EMMC_OTP_DEBUG
	pr_debug("EMMC_OTP: start MSDC_SINGLE_READ_WRITE!\n");
#endif

	memset(&msdc_data, 0, sizeof(struct mmc_data));
	memset(&msdc_mrq, 0, sizeof(struct mmc_request));
	memset(&msdc_cmd, 0, sizeof(struct mmc_command));

	msdc_mrq.cmd = &msdc_cmd;
	msdc_mrq.data = &msdc_data;

	msdc_data.flags = MMC_DATA_READ;
	msdc_cmd.opcode = MMC_READ_SINGLE_BLOCK;
	msdc_data.blocks = 1;	/* one block Request */

	memset(l_rcv_buf, 0, 512);

	msdc_cmd.arg = l_addr;

	BUG_ON(!host_ctl->mmc->card);
	if (!mmc_card_blockaddr(host_ctl->mmc->card)) {
		pr_debug("the device is used byte address!\n");
		msdc_cmd.arg <<= 9;
	}

	msdc_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	msdc_data.stop = NULL;
	msdc_data.blksz = 512;
	msdc_data.sg = &msdc_sg;
	msdc_data.sg_len = 1;

	sg_init_one(&msdc_sg, l_rcv_buf, 512);
	mmc_set_data_timeout(&msdc_data, host_ctl->mmc->card);

	mmc_wait_for_req(host_ctl->mmc, &msdc_mrq);

	mmc_release_host(host_ctl->mmc);

	if (msdc_cmd.error)
		l_ret = msdc_cmd.error;

	if (msdc_data.error)
		l_ret = msdc_data.error;
	else
		l_ret = OTP_SUCCESS;

	return l_ret;
}

unsigned int emmc_otp_write(unsigned int blk_offset, void *BufferPtr)
{
	char l_buf[512];
	unsigned char *l_send_buf = (unsigned char *)BufferPtr;
	unsigned int l_ret;
	unsigned int l_addr;
	unsigned int l_otp_size;
	struct scatterlist msdc_sg;
	struct mmc_data msdc_data;
	struct mmc_command msdc_cmd;
	struct mmc_request msdc_mrq;
	struct msdc_host *host_ctl;
#ifdef MTK_MSDC_USE_CACHE
	struct mmc_command msdc_sbc;
#endif
//add begin yan.fang 1120 modify for task742725,742727
	OTP_IMEI_Write(BufferPtr);
//add end yan.fang 1120 modify for task742725,742727
	/* check parameter */
	l_addr = emmc_otp_start();
	l_otp_size = emmc_get_wp_size();
	if (blk_offset > (l_otp_size >> 9))
		return OTP_ERROR_OVERSCOPE;

	l_addr += blk_offset;

	host_ctl = emmc_otp_get_host();

	BUG_ON(!host_ctl);
	BUG_ON(!host_ctl->mmc);

	mmc_claim_host(host_ctl->mmc);

	/* make sure access user data area */
	mmc_send_ext_csd(host_ctl->mmc->card, l_buf);
	if (l_buf[179] & 0x7) {
		l_buf[179] &= ~0x7;
		l_buf[179] |= 0x0;
		mmc_switch(host_ctl->mmc->card, 0, 179, l_buf[179], 1000);
	}

#if EMMC_OTP_DEBUG
	pr_debug("EMMC_OTP: start MSDC_SINGLE_READ_WRITE!\n");
#endif

	memset(&msdc_data, 0, sizeof(struct mmc_data));
	memset(&msdc_mrq, 0, sizeof(struct mmc_request));
	memset(&msdc_cmd, 0, sizeof(struct mmc_command));
#ifdef MTK_MSDC_USE_CACHE
	memset(&msdc_sbc, 0, sizeof(struct mmc_command));
#endif

	msdc_mrq.cmd = &msdc_cmd;
	msdc_mrq.data = &msdc_data;

	msdc_data.flags = MMC_DATA_WRITE;
#ifdef MTK_MSDC_USE_CACHE
	if (mmc_card_mmc(host_ctl->mmc->card) && (host_ctl->mmc->card->ext_csd.cache_ctrl & 0x1)) {
		msdc_cmd.opcode = MMC_WRITE_MULTIPLE_BLOCK;
		msdc_mrq.sbc = &msdc_sbc;
		msdc_mrq.sbc->opcode = MMC_SET_BLOCK_COUNT;
		msdc_mrq.sbc->arg = msdc_data.blocks | (1 << 31);
		msdc_mrq.sbc->flags = MMC_RSP_R1 | MMC_CMD_AC;
	} else {
		msdc_cmd.opcode = MMC_WRITE_BLOCK;
	}
#else
	msdc_cmd.opcode = MMC_WRITE_BLOCK;
#endif
	msdc_data.blocks = 1;

	msdc_cmd.arg = l_addr;

	BUG_ON(!host_ctl->mmc->card);
	if (!mmc_card_blockaddr(host_ctl->mmc->card)) {
		pr_debug("the device is used byte address!\n");
		msdc_cmd.arg <<= 9;
	}

	msdc_cmd.flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;

	msdc_data.stop = NULL;
	msdc_data.blksz = 512;
	msdc_data.sg = &msdc_sg;
	msdc_data.sg_len = 1;

	sg_init_one(&msdc_sg, l_send_buf, 512);
	mmc_set_data_timeout(&msdc_data, host_ctl->mmc->card);

	mmc_wait_for_req(host_ctl->mmc, &msdc_mrq);

	mmc_release_host(host_ctl->mmc);

	if (msdc_cmd.error)
		l_ret = msdc_cmd.error;

	if (msdc_data.error)
		l_ret = msdc_data.error;
	else
		l_ret = OTP_SUCCESS;

	return l_ret;
}

/*
static int emmc_otp_proc_read(char *page, char **start, off_t off,
	int count, int *eof, void *data)
{
	int len;
	if (off > 0)
	{
		return 0;
	}
	return len;
}

static int emmc_otp_proc_write(struct file* file, const char* buffer,
	unsigned long count, void *data)
{
	char buf[16];
	int len = count;

	if (len >= sizeof(buf))
	{
	len = sizeof(buf) - 1;
	}

	if (copy_from_user(buf, buffer, len))
	{
	return -EFAULT;
	}

	return len;
}
*/

static int mt_otp_open(struct inode *inode, struct file *filp)
{
	pr_debug("[%s]:(MAJOR)%d:(MINOR)%d\n",
		 __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	filp->private_data = (int *)OTP_MAGIC_NUM;
	return 0;
}

static int mt_otp_release(struct inode *inode, struct file *filp)
{
	pr_debug("[%s]:(MAJOR)%d:(MINOR)%d\n",
		 __func__, MAJOR(inode->i_rdev), MINOR(inode->i_rdev));
	return 0;
}

static int mt_otp_access(unsigned int access_type, unsigned int offset, void *buff_ptr,
			 unsigned int length, unsigned int *status)
{
	unsigned int ret = 0;
	char *BufAddr = (char *)buff_ptr;
	unsigned int blkno, AccessLength = 0;
	unsigned int l_block_size = 512;
	int Status = 0;

	char *p_D_Buff;
	/* char S_Buff[64]; */

	p_D_Buff = kmalloc(l_block_size, GFP_KERNEL);

	if (!p_D_Buff) {
		ret = -ENOMEM;
		*status = OTP_ERROR_NOMEM;
		goto exit;
	}

	pr_debug("[%s]: %s (0x%x) length:(%d bytes) !\n",
		 __func__, access_type ? "WRITE" : "READ", offset, length);

	do {
		blkno = offset / l_block_size;
		if (FS_EMMC_OTP_READ == access_type) {
			memset(p_D_Buff, 0xff, l_block_size);

			pr_debug("[%s]: Read Access of page (%d)\n", __func__, blkno);

			Status = g_emmc_otp_func.read(blkno, p_D_Buff);
			*status = Status;

			if (OTP_SUCCESS != Status) {
				pr_debug("[%s]: Read status (%d)\n", __func__, Status);
				break;
			}

			AccessLength = l_block_size - (offset % l_block_size);

			if (length >= AccessLength) {
				memcpy(BufAddr, (p_D_Buff + (offset % l_block_size)), AccessLength);
			} else {
				/* last time */
				memcpy(BufAddr, (p_D_Buff + (offset % l_block_size)), length);
			}
		} else if (FS_EMMC_OTP_WRITE == access_type) {
			AccessLength = l_block_size - (offset % l_block_size);
			memset(p_D_Buff, 0x0, l_block_size);

			Status = g_emmc_otp_func.read(blkno, p_D_Buff);
			*status = Status;

			if (OTP_SUCCESS != Status) {
				pr_debug("[%s]: read before write, Read status (%d) blkno (0x%x)\n",
					 __func__, Status, blkno);
				break;
			}

			if (length >= AccessLength) {
				memcpy((p_D_Buff + (offset % l_block_size)), BufAddr, AccessLength);
			} else {
				/* last time */
				memcpy((p_D_Buff + (offset % l_block_size)), BufAddr, length);
			}

			Status = g_emmc_otp_func.write(blkno, p_D_Buff);
			*status = Status;

			if (OTP_SUCCESS != Status) {
				pr_debug("[%s]: Write status (%d)\n", __func__, Status);
				break;
			}
		} else {
			pr_err("[%s]: Error, not either read nor write operations !\n", __func__);
			break;
		}

		offset += AccessLength;
		BufAddr += AccessLength;
		if (length <= AccessLength) {
			length = 0;
			break;
		}
		length -= AccessLength;
		pr_debug("[%s]: Remaining %s (%d) !\n", __func__,
			access_type ? "WRITE" : "READ", length);

	} while (1);


	kfree(p_D_Buff);
 exit:
	return ret;
}

static long mt_otp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	char *pbuf;

	void __user *uarg = (void __user *)arg;
	struct otp_ctl otpctl;

	/* Lock */
	/* spin_lock(&g_emmc_otp_lock); */

	if (copy_from_user(&otpctl, uarg, sizeof(struct otp_ctl))) {
		ret = -EFAULT;
		goto exit;
	}
//add begin yan.fang 1120 modify for task742725,742727
	id_offset(&otpctl);
//add end  yan.fang 1120 modify for task742725,742727

	/*
	   if(false == g_bInitDone)
	   {
	   pr_err("ERROR: EMMC Flash Not initialized !!\n");
	   ret = -EFAULT;
	   goto exit;
	   } */
	pbuf = kmalloc_array(otpctl.Length, sizeof(char), GFP_KERNEL);

	if (!pbuf) {
		ret = -ENOMEM;
		goto exit;
	}

	switch (cmd) {
	case EMMC_OTP_GET_LENGTH:
		pr_debug("OTP IOCTL: EMMC_OTP_GET_LENGTH\n");
		if (('c' == (otpctl.status & 0xff)) && ('c' == ((otpctl.status >> 8) & 0xff))
			&& ('c' == ((otpctl.status >> 16) & 0xff))
			&& ('i' == ((otpctl.status >> 24) & 0xff))) {
			otpctl.QLength = g_otp_user_ccci;
		} else {
			g_emmc_otp_func.query_length(&otpctl.QLength);
		}
		otpctl.status = OTP_SUCCESS;
		pr_debug("OTP IOCTL: The Length is %d\n", otpctl.QLength);
		break;
	case EMMC_OTP_READ:
		pr_debug("OTP IOCTL: EMMC_OTP_READ Offset(0x%x), Length(0x%x)\n", otpctl.Offset,
			 otpctl.Length);
		memset(pbuf, 0xff, sizeof(char) * otpctl.Length);

		mt_otp_access(FS_EMMC_OTP_READ, otpctl.Offset, pbuf, otpctl.Length, &otpctl.status);

		if (copy_to_user(otpctl.BufferPtr, pbuf, (sizeof(char) * otpctl.Length))) {
			pr_err("EMMC_OTP IOCTL: Copy to user buffer Error !\n");
			goto error;
		}
		break;
	case EMMC_OTP_WRITE:
		pr_debug("OTP IOCTL: EMMC_OTP_WRITE Offset(0x%x), Length(0x%x)\n", otpctl.Offset,
			 otpctl.Length);
		if (copy_from_user(pbuf, otpctl.BufferPtr, (sizeof(char) * otpctl.Length))) {
			pr_err("EMMC_OTP IOCTL: Copy from user buffer Error !\n");
			goto error;
		}
		mt_otp_access(FS_EMMC_OTP_WRITE, otpctl.Offset, pbuf, otpctl.Length,
				  &otpctl.status);
		break;
	default:
		ret = -EINVAL;
	}

	ret = copy_to_user(uarg, &otpctl, sizeof(struct otp_ctl));

 error:
	kfree(pbuf);
 exit:
	/* spin_unlock(&g_emmc_otp_lock); */
	return ret;
}

#ifdef CONFIG_COMPAT
static int compat_get_otp_ctl_allocation(struct compat_otp_ctl __user *arg32,
						struct otp_ctl __user *arg64)
{
	compat_uint_t u;
	compat_uptr_t p;
	int err;

	err = get_user(u, &(arg32->QLength));
	err |= put_user(u, &(arg64->QLength));
	err |= get_user(u, &(arg32->Offset));
	err |= put_user(u, &(arg64->Offset));
	err |= get_user(u, &(arg32->Length));
	err |= put_user(u, &(arg64->Length));
	err |= get_user(p, &(arg32->BufferPtr));
	err |= put_user(compat_ptr(p), &(arg64->BufferPtr));
	err |= get_user(u, &(arg32->status));
	err |= put_user(u, &(arg64->status));

	return err;
}

static int compat_put_otp_ctl_allocation(struct compat_otp_ctl __user *arg32,
						struct otp_ctl __user *arg64)
{
	compat_uint_t u;
	int err;

	err = get_user(u, &(arg64->QLength));
	err |= put_user(u, &(arg32->QLength));
	err |= get_user(u, &(arg64->Offset));
	err |= put_user(u, &(arg32->Offset));
	err |= get_user(u, &(arg64->Length));
	err |= put_user(u, &(arg32->Length));
	err |= get_user(u, &(arg64->status));
	err |= put_user(u, &(arg32->status));

	return err;
}

static long mt_otp_ioctl_compat(struct file *file, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	int err = 0;
	struct compat_otp_ctl *arg32;
	struct otp_ctl *arg64;

	/* void __user *uarg = compat_ptr(arg); */

	if (!file->f_op || !file->f_op->unlocked_ioctl)
		return -ENOTTY;

	arg32 = compat_ptr(arg);
	arg64 = compat_alloc_user_space(sizeof(*arg64));
	if (arg64 == NULL)
		return -EFAULT;

	ret = compat_get_otp_ctl_allocation(arg32, arg64);
	if (ret)
		return ret;
	/* Lock */
	/* spin_lock(&g_emmc_otp_lock); */

	switch (cmd) {
	case COMPAT_EMMC_OTP_GET_LENGTH:
		ret = mt_otp_ioctl(file, EMMC_OTP_GET_LENGTH, (unsigned long)arg64);
		/* pr_err("OTP IOCTL: COMPAT_EMMC_OTP_GET_LENGTH\n"); */
		if (ret)
			pr_err("EMMC_OTP_GET_LENGTH unlocked_ioctl failed.");
		break;
	case COMPAT_EMMC_OTP_READ:
		/* pr_err("OTP IOCTL: COMPAT_EMMC_OTP_READ\n"); */
		ret = mt_otp_ioctl(file, EMMC_OTP_READ, (unsigned long)arg64);
		if (ret)
			pr_err("EMMC_OTP_READ unlocked_ioctl failed.");
		break;
	case COMPAT_EMMC_OTP_WRITE:
		/* pr_err("OTP IOCTL: COMPAT_EMMC_OTP_WRITE\n"); */
		ret = mt_otp_ioctl(file, EMMC_OTP_WRITE, (unsigned long)arg64);
		if (ret)
			pr_err("EMMC_OTP_WRITE unlocked_ioctl failed.");
		break;
	default:
		ret = -EINVAL;
	}
	err = compat_put_otp_ctl_allocation(arg32, arg64);
	return ret ? ret : err;
}
#endif

static const struct file_operations emmc_otp_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = mt_otp_ioctl,
	.open = mt_otp_open,
	.release = mt_otp_release,
#ifdef CONFIG_COMPAT
	.compat_ioctl = mt_otp_ioctl_compat,
#endif
};

static struct miscdevice emmc_otp_dev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "otp",
	.fops = &emmc_otp_fops,
};



static int emmc_otp_probe(struct platform_device *pdev)
{
	int ret = 0;

	pr_debug("in emmc otp probe function\n");

	return ret;
}

static int emmc_otp_remove(struct platform_device *pdev)
{
	return 0;
}

static struct platform_driver emmc_otp_driver = {
	.probe = emmc_otp_probe,
	.remove = emmc_otp_remove,

	.driver = {
		   .name = DRV_NAME_MISC,
		   .owner = THIS_MODULE,
		   },
};

static int __init emmc_otp_init(void)
{
	int err = 0;

	pr_debug("MediaTek EMMC OTP misc driver init\n");

	/* spin_lock_init(&g_emmc_otp_lock); */

	g_emmc_otp_func.query_length = emmc_otp_query_length;
	g_emmc_otp_func.read		 = emmc_otp_read;
	g_emmc_otp_func.write		= emmc_otp_write;
#if 0
#ifndef USER_BUILD_KERNEL
	entry = create_proc_entry(PROCNAME, 0660, NULL);
#else
	entry = create_proc_entry(PROCNAME, 0440, NULL);
#endif

	if (entry == NULL) {
		pr_err("emmc OTP: unable to create /proc entry\n");
		return -ENOMEM;
	}

	entry->read_proc = emmc_otp_proc_read;
	entry->write_proc = emmc_otp_proc_write;
	/* entry->owner = THIS_MODULE; */
#endif
	platform_driver_register(&emmc_otp_driver);

	pr_debug("OTP: register EMMC OTP device ...\n");
	err = misc_register(&emmc_otp_dev);
	if (unlikely(err)) {
		pr_err("OTP: failed to register EMMC OTP device!\n");
		return err;
	}

	return 0;
}

static void __exit emmc_otp_exit(void)
{
	pr_debug("MediaTek EMMC OTP misc driver exit\n");

	misc_deregister(&emmc_otp_dev);

	g_emmc_otp_func.query_length = NULL;
	g_emmc_otp_func.read		 = NULL;
	g_emmc_otp_func.write		= NULL;

	platform_driver_unregister(&emmc_otp_driver);
	remove_proc_entry(PROCNAME, NULL);
}


module_init(emmc_otp_init);
module_exit(emmc_otp_exit);
