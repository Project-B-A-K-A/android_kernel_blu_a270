//TP driver
#include "tpd.h"
#include <linux/interrupt.h>
#include <cust_eint.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>

#include <linux/module.h>
#include <linux/input.h>
#include <linux/earlysuspend.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <linux/miscdevice.h>
#include <linux/hrtimer.h>

#include "ektf2k_driver.h"


#include <mach/mt_pm_ldo.h> 
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>

#include "cust_gpio_usage.h"

// for linux 2.6.36.3
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <asm/ioctl.h>

//dma
#include <linux/dma-mapping.h>

/////////////////////////////////////////////////////////
#define I2C_NUM 1

//#define SOFTKEY_AXIS_VER
//#define ELAN_TEN_FINGERS
//#define _DMA_MODE_

#define ELAN_BUTTON
#define LCT_VIRTUAL_KEY
#define TPD_HAVE_BUTTON
//#define ELAN_3K_IC_SOLUTION

//#define NON_MTK_MODE   //I2C Support > 8bits Transfer


#ifdef MT6575 
 extern void mt65xx_eint_unmask(unsigned int line);
 extern void mt65xx_eint_mask(unsigned int line);
 extern void mt65xx_eint_set_hw_debounce(kal_uint8 eintno, kal_uint32 ms);
 extern kal_uint32 mt65xx_eint_set_sens(kal_uint8 eintno, kal_bool sens);
 extern void mt65xx_eint_registration(kal_uint8 eintno, kal_bool Dbounce_En,
									  kal_bool ACT_Polarity, void (EINT_FUNC_PTR)(void),
									  kal_bool auto_umask);
#else

	extern void mt65xx_eint_unmask(unsigned int line);
	extern void mt65xx_eint_mask(unsigned int line);
	extern void mt65xx_eint_set_hw_debounce(unsigned int eint_num, unsigned int ms);
	extern unsigned int mt65xx_eint_set_sens(unsigned int eint_num, unsigned int sens);
	extern void mt65xx_eint_registration(unsigned int eint_num, unsigned int is_deb_en, unsigned int pol, void (EINT_FUNC_PTR)(void), unsigned int is_auto_umask);
#endif
#ifdef ELAN_TEN_FINGERS
#define PACKET_SIZE             44          /* support 10 fingers packet */
#else
#define PACKET_SIZE             8           /* support 2 fingers packet  */
//#define PACKET_SIZE             18        /* support 5 fingers packet  */
#endif

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
static int tpd_wb_start_local[TPD_WARP_CNT] = TPD_WARP_START;
static int tpd_wb_end_local[TPD_WARP_CNT]   = TPD_WARP_END;
#endif
#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
static int tpd_calmat_local[8]     = TPD_CALIBRATION_MATRIX;
static int tpd_def_calmat_local[8] = TPD_CALIBRATION_MATRIX;
#endif

#define ELAN_DEBUG

#define PWR_STATE_DEEP_SLEEP              0
#define PWR_STATE_NORMAL                  1
#define PWR_STATE_MASK                    BIT(3)

#define CMD_S_PKT                         0x52
#define CMD_R_PKT                         0x53
#define CMD_W_PKT                         0x54

#define HELLO_PKT                         0x55
#define TWO_FINGERS_PKT                   0x5A
#define FIVE_FINGERS_PKT                  0x5D
#define MTK_FINGERS_PKT                   0x6D    /** 2 Fingers: 5A, 5 Fingers: 5D, 10 Fingers: 62 **/
#define TEN_FINGERS_PKT                   0x62

#define RESET_PKT                         0x77
#define CALIB_PKT                         0xA8

#define TPD_OK 0

#define MTK_ELAN_DEBUG
#ifdef MTK_ELAN_DEBUG
         #define MTK_TP_DEBUG(fmt, args ...) printk("mtk-tpd: %5d: " fmt, __LINE__,##args)
		 #define printkElan printk
#else
        #define MTK_TP_DEBUG(fmt, args ...)
		printkElan(const char *s, ...)
		{

		}
#endif


#ifdef TPD_HAVE_BUTTON
#define TPD_BUTTON_HEIGH        100
#define TPD_KEY_COUNT           3
#define TPD_KEYS                { KEY_MENU, /*KEY_HOME*/KEY_HOMEPAGE, KEY_BACK}
#define TPD_KEYS_DIM            {{107,1370,109,TPD_BUTTON_HEIGH},{365,1370,109,TPD_BUTTON_HEIGH},{617,1370,102,TPD_BUTTON_HEIGH}}

static int tpd_keys_local[TPD_KEY_COUNT] = TPD_KEYS;
static int tpd_keys_dim_local[TPD_KEY_COUNT][4] = TPD_KEYS_DIM;
#endif

// modify
#define SYSTEM_RESET_PIN_SR   135

//Add these Define
#define IAP_PORTION         0           //upgrade  FW
#if IAP_PORTION                         //upgrade  FW DMA mode
#define _DMA_FW_UPGRADE_MODE_              
#endif
#define PAGERETRY           30
#define IAPRESTART           5
#define CMD_54001234

#ifdef _DMA_MODE_
static uint8_t *gpDMABuf_va = NULL;
static uint32_t *gpDMABuf_pa = NULL;
#endif

#ifdef _DMA_FW_UPGRADE_MODE_
static uint8_t *gpDMAFWBuf_va = NULL;
static uint32_t *gpDMAFWBuf_pa = NULL;
static int elan_i2c_dma_fw_recv_data(struct i2c_client *client, uint8_t *buf,uint8_t len);
static int elan_i2c_dma_fw_send_data(struct i2c_client *client, uint8_t *buf,uint8_t len);
#endif

// For Firmware Update 
#define ELAN_IOCTLID				0xD0
#define IOCTL_I2C_SLAVE				_IOW(ELAN_IOCTLID,  1, int)
#define IOCTL_MAJOR_FW_VER  		_IOR(ELAN_IOCTLID,  2, int)
#define IOCTL_MINOR_FW_VER  		_IOR(ELAN_IOCTLID,  3, int)
#define IOCTL_RESET  				_IOR(ELAN_IOCTLID,  4, int)
#define IOCTL_IAP_MODE_LOCK  		_IOR(ELAN_IOCTLID,  5, int)
#define IOCTL_CHECK_RECOVERY_MODE  	_IOR(ELAN_IOCTLID,  6, int)
#define IOCTL_FW_VER  				_IOR(ELAN_IOCTLID,  7, int)
#define IOCTL_X_RESOLUTION  		_IOR(ELAN_IOCTLID,  8, int)
#define IOCTL_Y_RESOLUTION  		_IOR(ELAN_IOCTLID,  9, int)
#define IOCTL_FW_ID  				_IOR(ELAN_IOCTLID, 10, int)
#define IOCTL_ROUGH_CALIBRATE  		_IOR(ELAN_IOCTLID, 11, int)
#define IOCTL_IAP_MODE_UNLOCK  		_IOR(ELAN_IOCTLID, 12, int)
#define IOCTL_I2C_INT  				_IOR(ELAN_IOCTLID, 13, int)
#define IOCTL_RESUME  				_IOR(ELAN_IOCTLID, 14, int)
#define IOCTL_POWER_LOCK  			_IOR(ELAN_IOCTLID, 15, int)
#define IOCTL_POWER_UNLOCK  		_IOR(ELAN_IOCTLID, 16, int)
#define IOCTL_BC_VER  				_IOR(ELAN_IOCTLID, 18, int)
//{ iap-ram
#define IOCTL_IAPRAM				_IOR(ELAN_IOCTLID, 20, int) 
#define IOCTL_IAPRAM_CHECKSUM		_IOW(ELAN_IOCTLID, 21, unsigned long)
//} iap-ram

#define IOCTL_FW_UPDATE  			_IOR(ELAN_IOCTLID, 17, int)
#define IOCTL_GET_UPDATE_PROGREE	_IOR(CUSTOMER_IOCTLID,  2, int)


//{ iap-ram
static uint8_t file_iapRam_data[] = {
#include "HUARUICHUANG_ELAN2K_PR6041_e12_V001.i"
};
uint8_t IAP_RAM_ADDR = 0x15;
enum
{
	PageSize		= 132,
	PageNum		    = 249,
	ACK_Fail		= 0x00,
	ACK_OK			= 0xAA,
	ACK_REWRITE		= 0x55,
};

enum
{
	E_FD			= -1,
};
//} iap-ram



extern struct tpd_device *tpd;

uint8_t RECOVERY=0x00;
int FW_VERSION=0x00;
int X_RESOLUTION=0x00;  
int Y_RESOLUTION=0x00;
int FW_ID=0x00;
int BC_VERSION = 0x00;
int work_lock=0x00;
int power_lock=0x00;
int circuit_ver=0x01;
int button_state = 0;
static int probe_flage=0;

/*++++i2c transfer start+++++++*/
#ifdef ELAN_3K_IC_SOLUTION
int file_fops_addr=0x10;
#else
int file_fops_addr=0x15;
#endif
/*++++i2c transfer end+++++++*/

int tpd_down_flag=0;
static BOOTMODE bootMode = NORMAL_BOOT;  //  add for TP button,by tyd-lg

struct i2c_client *i2c_client = NULL;
struct task_struct *thread = NULL;
struct task_struct *update_thread = NULL;

static DECLARE_WAIT_QUEUE_HEAD(waiter);
// iap-ram
static int cc_packet_handler(struct i2c_client *client);
//static int cc_packet_handler(uint8_t *buf);
int IAP_RAM( struct i2c_client *client );
static int elan_ktf2k_ts_iapRam_continue(struct i2c_client *client);
// iap-ram	
static inline int elan_ktf2k_ts_parse_xy(uint8_t *data,
                            uint16_t *x, uint16_t *y);

#define TP_PROXIMITY_SENSOR_NEW
#ifdef TP_PROXIMITY_SENSOR_NEW
#include <linux/hwmsensor.h>
#include <linux/hwmsen_dev.h>
#include <linux/sensors_io.h>
static int PROXIMITY =0;
static int PROXIMITY_STATE = -1;
static U8 PROXIMITY_SLEEP = 0;
static U8 TP_SLEEP = 0;

static int CTP_Face_Mode_State(void);
static  int CTP_Face_Mode_Switch(int onoff_state);
static  int Get_Ctp_Face_Mode(void);
 s32 ektf2k_ps_operate(void *self, u32 command, void *buff_in, s32 size_in,
                   void *buff_out, s32 size_out, s32 *actualout);
#endif
//-----variables---------
#define ESD_CHECK
#ifdef ESD_CHECK
	static int have_interrupts = 0;
	static struct workqueue_struct *esd_wq = NULL;
	static struct delayed_work esd_work;
	static unsigned long delay = 2*HZ;
#endif
static struct tpd_driver_t tpd_device_driver;
static void elan_touch_esd_func(struct work_struct *work);

static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int tpd_detect(struct i2c_client *client, int kind, struct i2c_board_info *info);
static int tpd_remove(struct i2c_client *client);
static int touch_event_handler(void *unused);


static int tpd_flag = 0;

#if 0
static int key_pressed = -1;

struct osd_offset{
         int left_x;
         int right_x;
         unsigned int key_event;
};

static struct osd_offset OSD_mapping[] = {    //Range need define by Case!
         { 35, 290, KEY_MENU},                //menu_left_x, menu_right_x, KEY_MENU
         {303, 467, KEY_HOME},                //home_left_x, home_right_x, KEY_HOME
         {473, 637, KEY_BACK},                //back_left_x, back_right_x, KEY_BACK
         {641, 905, KEY_SEARCH},              //search_left_x, search_right_x, KEY_SEARCH
};
#endif 

static const struct i2c_device_id tpd_id[] = {{"ekft2k", 0}, {}};

#ifdef ELAN_3K_IC_SOLUTION
static struct i2c_board_info __initdata ekft2k_i2c_tpd = { I2C_BOARD_INFO("ekft2k", (0x20>>1))};
#else
static struct i2c_board_info __initdata ekft2k_i2c_tpd = { I2C_BOARD_INFO("ekft2k", (0x2a>>1))};
#endif

#if IAP_PORTION
uint8_t ic_status=0x00;  //0:OK 1:master fail 2:slave fail
int update_progree=0;

#ifdef ELAN_3K_IC_SOLUTION
uint8_t I2C_DATA[3] = {0x10, 0x20, 0x21};/*I2C devices address*/  
#else
uint8_t I2C_DATA[3] = {0x15, 0x20, 0x21};/*I2C devices address*/  
#endif


/*The newest firmware, if update must be changed here*/
static uint8_t file_fw_data[] = {
//#include "fw_data.i"
};

enum
{
         PageSize           = 132,
         PageNum            = 249,
         ACK_Fail           = 0x00,
         ACK_OK             = 0xAA,
         ACK_REWRITE        = 0x55,
};

enum
{
         E_FD               = -1,
};
#endif

static struct i2c_driver tpd_i2c_driver =
{
    .driver = {
        .name = "ekft2k",//.name = TPD_DEVICE
        .owner = THIS_MODULE,
    },
    .probe = tpd_probe,
    .remove =  tpd_remove,
    .id_table = tpd_id,
    .detect = tpd_detect,
   // .address_data = &addr_data,
};

struct elan_ktf2k_ts_data {
         struct i2c_client *client;
         struct input_dev *input_dev;
         struct workqueue_struct *elan_wq;
         struct work_struct work;
         struct early_suspend early_suspend;
         int intr_gpio;
// Firmware Information
         int fw_ver;
         int fw_id;
         int bc_ver;
         int x_resolution;
         int y_resolution;
// For Firmare Update 
         struct miscdevice firmware;
         struct hrtimer timer;
};

static struct elan_ktf2k_ts_data *private_ts;
static int __hello_packet_handler(struct i2c_client *client);
static int __fw_packet_handler(struct i2c_client *client);
static int elan_ktf2k_ts_rough_calibrate(struct i2c_client *client);
static int tpd_resume(struct i2c_client *client);

#if IAP_PORTION
static int update_fw_handler(void *unused);
int Update_FW_One(/*struct file *filp,*/ struct i2c_client *client, int recovery);
int IAPReset();
#endif


#ifdef _DMA_MODE_
static int elan_i2c_dma_recv_data(struct i2c_client *client, uint8_t *buf,uint8_t len)
{
         int rc;
         uint8_t *pReadData = 0;
         unsigned short addr = 0;
         addr = client->addr ;
         client->addr |= I2C_DMA_FLAG;     
         pReadData = gpDMABuf_va;
  if(!pReadData){
                   printkElan("[elan] dma_alloc_coherent failed!\n");
                   return -1;
  }
         rc = i2c_master_recv(client, gpDMABuf_pa, len);
         printkElan("[elan] elan_i2c_dma_recv_data rc=%d!\n",rc);
         copy_to_user(buf, pReadData, len);
                   client->addr = addr;
         return rc;
}

static int elan_i2c_dma_send_data(struct i2c_client *client, uint8_t *buf,uint8_t len)
{
         int rc;
         unsigned short addr = 0;
         addr = client->addr ;
         client->addr |= I2C_DMA_FLAG;     
         uint8_t *pWriteData = gpDMABuf_va;
  if(!pWriteData){
                   printkElan("[elan] dma_alloc_coherent failed!\n");
                   return -1;
  }
  copy_from_user(pWriteData, ((void*)buf), len);

         rc = i2c_master_send(client, gpDMABuf_pa, len);
         printkElan("[elan] elan_i2c_dma_send_data rc=%d!\n",rc);
                   client->addr = addr;
         return rc;
}
#endif

//DMA_FW_Upgrade Start Function
#ifdef _DMA_FW_UPGRADE_MODE_
static int elan_i2c_dma_fw_recv_data(struct i2c_client *client, uint8_t *buf,uint8_t len)
{
         int rc;
         uint8_t *pReadData = 0;
         unsigned short addr = 0;
         addr = client->addr ;
         client->addr |= I2C_DMA_FLAG;     
         pReadData = gpDMAFWBuf_va;
  if(!pReadData){
                   printkElan("[elan] dma_alloc_coherent failed!\n");
                   return -1;
  }
         rc = i2c_master_recv(client, gpDMAFWBuf_pa, len);
         printkElan("[elan] elan_i2c_dma_recv_data rc=%d!\n",rc);
         copy_to_user(buf, pReadData, len);
                   client->addr = addr;
         return rc;
}

static int elan_i2c_dma_fw_send_data(struct i2c_client *client, uint8_t *buf,uint8_t len)
{
         int rc;
         unsigned short addr = 0;
         addr = client->addr ;
         client->addr |= I2C_DMA_FLAG;     
         uint8_t *pWriteData = gpDMAFWBuf_va;
  if(!pWriteData){
                   printkElan("[elan] dma_alloc_coherent failed!\n");
                   return -1;
  }
  copy_from_user(pWriteData, ((void*)buf), len);

         rc = i2c_master_send(client, gpDMAFWBuf_pa, len);
         printkElan("[elan] elan_i2c_dma_send_data rc=%d!\n",rc);
                   client->addr = addr;
         return rc;
}
#endif
//DMA_FW_Upgrade End Function

// For Firmware Update 
int elan_iap_open(struct inode *inode, struct file *filp){ 

      printkElan("[ELAN]into elan_iap_open\n");
      if (private_ts == NULL)  
	  		printkElan("private_ts is NULL~~~");
                   
      return 0;
}

int elan_iap_release(struct inode *inode, struct file *filp){    
      return 0;
}

static ssize_t elan_iap_write(struct file *filp, const char *buff, size_t count, loff_t *offp){  
    int ret;
    char *tmp;

    printkElan("[ELAN]into elan_iap_write\n");
    if (count > 8192)
        count = 8192;

    tmp = kmalloc(count, GFP_KERNEL);
    
    if (tmp == NULL)
        return -ENOMEM;

    if (copy_from_user(tmp, buff, count)) {
        return -EFAULT;
    }
#ifdef _DMA_MODE_    
    ret = elan_i2c_dma_send_data(private_ts->client, tmp, count);
#else    
    ret = i2c_master_send(private_ts->client, tmp, count);
#endif    
    kfree(tmp);
    return (ret == 1) ? count : ret;

}

ssize_t elan_iap_read(struct file *filp, char *buff, size_t count, loff_t *offp){    
    char *tmp;
    int ret;  
    long rc;

    printkElan("[ELAN]into elan_iap_read\n");
    if (count > 8192)
        count = 8192;

    tmp = kmalloc(count, GFP_KERNEL);

    if (tmp == NULL)
        return -ENOMEM;
#ifdef _DMA_MODE_
    ret = elan_i2c_dma_recv_data(private_ts->client, tmp, count);
#else    
    ret = i2c_master_recv(private_ts->client, tmp, count);
#endif  
    if (ret >= 0)
        rc = copy_to_user(buff, tmp, count);
    
    kfree(tmp);

    //return ret;
    return (ret == 1) ? count : ret;
         
}

static long elan_iap_ioctl(/*struct inode *inode,*/ struct file *filp,    unsigned int cmd, unsigned long arg){

    int __user *ip = (int __user *)arg;
    printkElan("[ELAN]into elan_iap_ioctl\n");
	printkElan("cmd value %x, %ld\n", cmd, arg);
	uint64_t checksum;
	uint8_t buf[64]={0};
         switch (cmd) {        
                   case IOCTL_I2C_SLAVE: 
                            private_ts->client->addr = (int __user)arg;
                            private_ts->client->addr &= I2C_MASK_FLAG; 
                            private_ts->client->addr |= I2C_ENEXT_FLAG;
                            //file_fops_addr = 0x15;
                            break;   
                   case IOCTL_MAJOR_FW_VER:            
                            break;        
                  case IOCTL_MINOR_FW_VER:            
                            break;        
                   case IOCTL_RESET:

                            mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
                           mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
                            mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
                            mdelay(10);
		//	#if !defined(EVB)
                            mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
		//	#endif
                           mdelay(10);
                            mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );

                            break;
                   case IOCTL_IAP_MODE_LOCK:
                            if(work_lock==0)
                            {
                                     printkElan("[elan]%s %x=IOCTL_IAP_MODE_LOCK\n", __func__,IOCTL_IAP_MODE_LOCK);
                                     work_lock=1;
                                     //disable_irq(CUST_EINT_TOUCH_PANEL_NUM);
                                     mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
                                     //cancel_work_sync(&private_ts->work);
                                    #ifdef ESD_CHECK	
                                        cancel_delayed_work_sync(&esd_work);
                                    #endif	
                            }
                            break;
                   case IOCTL_IAP_MODE_UNLOCK:
                            if(work_lock==1)
                            {                           
                                     work_lock=0;
                                     //enable_irq(CUST_EINT_TOUCH_PANEL_NUM);
                                     mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
                                    #ifdef ESD_CHECK
                                        queue_delayed_work(esd_wq, &esd_work, delay);	
                                    #endif
                            }
                            break;
                   case IOCTL_CHECK_RECOVERY_MODE:
                            return RECOVERY;
                            break;
                   case IOCTL_FW_VER:
                            __fw_packet_handler(private_ts->client);
                           return FW_VERSION;
                            break;
                   case IOCTL_X_RESOLUTION:
                            __fw_packet_handler(private_ts->client);
                            return X_RESOLUTION;
                            break;
                   case IOCTL_Y_RESOLUTION:
                            __fw_packet_handler(private_ts->client);
                            return Y_RESOLUTION;
                            break;
                   case IOCTL_FW_ID:
                            __fw_packet_handler(private_ts->client);
                            return FW_ID;
                            break;
                   case IOCTL_ROUGH_CALIBRATE:
                            return elan_ktf2k_ts_rough_calibrate(private_ts->client);
                   case IOCTL_I2C_INT:
                            put_user(mt_get_gpio_in(GPIO_CTP_EINT_PIN),ip);
                            printkElan("[elan]GPIO_CTP_EINT_PIN = %d\n", mt_get_gpio_in(GPIO_CTP_EINT_PIN));

			break;	
                   case IOCTL_RESUME:
                            tpd_resume(private_ts->client);
			break;	
                   case IOCTL_POWER_LOCK:
                            power_lock=1;
                            break;
                   case IOCTL_POWER_UNLOCK:
                            power_lock=0;
                            break;
#if IAP_PORTION		
                   case IOCTL_FW_UPDATE:
                            //RECOVERY = IAPReset(private_ts->client);
                            RECOVERY=0;
                            Update_FW_One(private_ts->client, RECOVERY);
#endif
                   case IOCTL_BC_VER:
                            __fw_packet_handler(private_ts->client);
                            return BC_VERSION;
                            break;
//{ iap-ram
		case IOCTL_IAPRAM:
			IAP_RAM(private_ts->client);
			break;
		case IOCTL_IAPRAM_CHECKSUM:
			printkElan("[elan] IOCTL_IAPRAM_CHECKSUM,%d\n",(int*)arg);
			//len = 1;
			if (copy_from_user(buf, (const void*)arg, 4))
			{
				printkElan("[elan] copy_from_user error\n");
				return -EFAULT;
			}
			printkElan("[elan] IOCTL_SET_COMMAND:%x:%x:%x:%x\n",
				buf[3], buf[2], buf[1], buf[0]);
			
			file_iapRam_data[2241] = buf[3];
			file_iapRam_data[2240] = buf[2];
			file_iapRam_data[2239] = buf[1];
			file_iapRam_data[2238] = buf[0];
			break;
//} iap-ram
                   default:            
                            break;   
         }       
         return 0;
}

struct file_operations elan_touch_fops = {    
        .open =            	elan_iap_open,    
        .write =         	elan_iap_write,    
        .read =          	elan_iap_read,    
        .release =        	elan_iap_release,    
        .unlocked_ioctl = 	elan_iap_ioctl, 
 };


int EnterISPMode(struct i2c_client *client, uint8_t  *isp_cmd)
{
         char buff[4] = {0};
         int len = 0;
         
         #ifdef _DMA_FW_UPGRADE_MODE_
          len = elan_i2c_dma_fw_send_data(client,isp_cmd,  sizeof(isp_cmd));
         #else
          len = i2c_master_send(client, isp_cmd,  sizeof(isp_cmd));
         #endif
        
         if (len != sizeof(buff)) {
                   printkElan("[ELAN] ERROR: EnterISPMode fail! len=%d\r\n", len);
                   return -1;
         }
         else{
                   printkElan("[ELAN] IAPMode write data successfully! cmd = [%2x, %2x, %2x, %2x]\n", isp_cmd[0], isp_cmd[1], isp_cmd[2], isp_cmd[3]);
         }
         return 0;
}

int WritePage(struct i2c_client *client, uint8_t * szPage, int byte)
{
         int len = 0;

#ifdef _DMA_FW_UPGRADE_MODE_
          len = elan_i2c_dma_fw_send_data(client, szPage,  byte);
#else
          len = i2c_master_send(client, szPage,  byte);
#endif

         if (len != byte) 
         {
                  printkElan("[ELAN] ERROR: write page error, write error. len=%d\r\n", len);
                   return -1;
         }

         return 0;
}

int ExtractPage(struct file *filp, uint8_t * szPage, int byte)
{
         int len = 0;

         len = filp->f_op->read(filp, szPage,byte, &filp->f_pos);
         if (len != byte) 
         {
                   printkElan("[ELAN] ExtractPage ERROR: read page error, read error. len=%d\r\n", len);
                   return -1;
         }

         return 0;
}

int GetAckData(struct i2c_client *client)
{
         int len = 0;

         char buff[2] = {0};
         
#ifdef _DMA_FW_UPGRADE_MODE_
         len = elan_i2c_dma_fw_recv_data(client, buff, sizeof(buff));
#else
         len = i2c_master_recv(client, buff, sizeof(buff));
#endif

         if (len != sizeof(buff)) {
                   printkElan("[ELAN] ERROR: read data error, write 50 times error. len=%d\r\n", len);
                   return -1;
         }

         printkElan("[ELAN] GetAckData:%x,%x\n",buff[0],buff[1]);
         if (buff[0] == 0xaa/* && buff[1] == 0xaa*/) 
                   return ACK_OK;
         else if (buff[0] == 0x55 && buff[1] == 0x55)
                   return ACK_REWRITE;
         else
                   return ACK_Fail;

         return 0;
}

void print_progress(int page, int ic_num, int j)
{
         int i, percent,page_tatol,percent_tatol;
         char str[256];
         str[0] = '\0';
         for (i=0; i<((page)/10); i++) {
                   str[i] = '#';
                   str[i+1] = '\0';
         }
         
         page_tatol=page+249*(ic_num-j);
         percent = ((100*page)/(249));
         percent_tatol = ((100*page_tatol)/(249*ic_num));

         if ((page) == (249))
                   percent = 100;

         if ((page_tatol) == (249*ic_num))
                   percent_tatol = 100;                 

         printkElan("\rprogress %s| %d%%", str, percent);
         
         if (page == (249))
                   printkElan("\n");
}
#if IAP_PORTION
/* 
* Restet and (Send normal_command ?)
* Get Hello Packet
*/
int  IAPReset()
{
     int res;

     mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
     mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
     mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
     mdelay(10);
     //#if !defined(EVB)
     mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
     //#endif
     mdelay(10);
     mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
     return 1;

#if 0
     printkElan("[ELAN] read Hello packet data!\n");     
     res= __hello_packet_handler(client);
     return res;
#endif 
}

/* Check Master & Slave is "55 aa 33 cc" */
int CheckIapMode(void)
{
         char buff[4] = {0},len = 0;
         //WaitIAPVerify(1000000);
         //len = read(fd, buff, sizeof(buff));
         
         
#ifdef _DMA_FW_UPGRADE_MODE_
	len = elan_i2c_dma_fw_recv_data(private_ts->client, buff, sizeof(buff));
#else
	len = i2c_master_recv(private_ts->client, buff, sizeof(buff));
#endif
         
    if (len != sizeof(buff)) 
    {
         printkElan("[ELAN] CheckIapMode ERROR: read data error,len=%d\r\n", len);
         return -1;
    }
    else
    {
         
         if (buff[0] == 0x55 && buff[1] == 0xaa && buff[2] == 0x33 && buff[3] == 0xcc)
         {
             //printkElan("[ELAN] CheckIapMode is 55 aa 33 cc\n");
             return 0;
         }
         else// if ( j == 9 )
         {
             printkElan("[ELAN] Mode= 0x%x 0x%x 0x%x 0x%x\r\n", buff[0], buff[1], buff[2], buff[3]);
             printkElan("[ELAN] ERROR:  CheckIapMode error\n");
             return -1;
         }
    }
    printkElan("\n");     
}

int Update_FW_One(struct i2c_client *client, int recovery)
{
         int res = 0,ic_num = 1;
         int iPage = 0, rewriteCnt = 0; //rewriteCnt for PAGE_REWRITE
         int i = 0;
         uint8_t data;

         int restartCnt = 0, checkCnt = 0; // For IAP_RESTART
         //uint8_t recovery_buffer[4] = {0};
         int byte_count;
         uint8_t *szBuff = NULL;
         int curIndex = 0;
#if CMD_54001234
	uint8_t isp_cmd[] = {0x54, 0x00, 0x12, 0x34};	 //54 00 12 34
#else
	uint8_t isp_cmd[] = {0x45, 0x49, 0x41, 0x50};	 //45 49 41 50
#endif
         uint8_t recovery_buffer[4] = {0};

IAP_RESTART: 

         data=I2C_DATA[0];//Master
         printkElan("[ELAN] %s: address data=0x%x \r\n", __func__, data);

//      if(recovery != 0x80)
//      {
            printkElan("[ELAN] Firmware upgrade normal mode !\n");

            IAPReset();
            mdelay(20);       

            res = EnterISPMode(private_ts->client, isp_cmd); //enter ISP mode
                                                        
                                                        
#ifdef _DMA_FW_UPGRADE_MODE_
            res = elan_i2c_dma_fw_recv_data(private_ts->client, recovery_buffer, 4);
#else
            res = i2c_master_recv(private_ts->client, recovery_buffer, 4);   //55 aa 33 cc 
#endif
                                                
			printkElan("[ELAN] recovery byte data:%x,%x,%x,%x \n",recovery_buffer[0],recovery_buffer[1],recovery_buffer[2],recovery_buffer[3]);                       

			mdelay(10);
#if 0
        //Check IC's status is IAP mode(55 aa 33 cc) or not
        res = CheckIapMode();    //Step 1 enter ISP mode
        if (res == -1) //CheckIapMode fail
        {        
                 checkCnt ++;
                 if (checkCnt >= 5)
                 {
                          printkElan("[ELAN] ERROR: CheckIapMode %d times fails!\n", IAPRESTART);
                          return E_FD;
                 }
                 else
                 {
                          printkElan("[ELAN] CheckIapMode retry %dth times! And restart IAP~~~\n\n", checkCnt);
                          goto IAP_RESTART;
                 }
        }
        else
                 printkElan("[ELAN]  CheckIapMode ok!\n");
#endif
//      } else
//               printkElan("[ELAN] Firmware upgrade recovery mode !\n");
         // Send Dummy Byte        
         printkElan("[ELAN] send one byte data:%x,%x",private_ts->client->addr,data);
         
         #ifdef _DMA_FW_UPGRADE_MODE_
         	res = elan_i2c_dma_fw_send_data(client, &data,  sizeof(data));
         #else
         	res = i2c_master_send(client, &data,  sizeof(data));
         #endif
         
         if(res!=sizeof(data))
         {
                   printkElan("[ELAN] dummy error code = %d\n",res);
         }        
         mdelay(50);

         // Start IAP
         for( iPage = 1; iPage <= PageNum; iPage++ ) 
         {
PAGE_REWRITE:
#if 0 
                  // 8byte mode
                   //szBuff = fw_data + ((iPage-1) * PageSize); 
                   for(byte_count=1;byte_count<=17;byte_count++)
                   {
                            if(byte_count!=17)
                            {                 
         //                         printkElan("[ELAN] byte %d\n",byte_count);          
         //                         printkElan("[ELAN] curIndex =%d\n",curIndex);
                                     szBuff = file_fw_data + curIndex;
                                     curIndex =  curIndex + 8;

                                     //ioctl(fd, IOCTL_IAP_MODE_LOCK, data);
                                     res = WritePage(szBuff, 8);
                            }
                            else
                            {
         //                         printkElan("[ELAN] byte %d\n",byte_count);
         //                         printkElan("[ELAN] curIndex =%d\n",curIndex);
                                     szBuff = file_fw_data + curIndex;
                                     curIndex =  curIndex + 4;
                                     //ioctl(fd, IOCTL_IAP_MODE_LOCK, data);
				res = WritePage(client, szBuff, 4); 
                            }
                   } // end of for(byte_count=1;byte_count<=17;byte_count++)
#endif

#if 1 // 132byte mode                
                   szBuff = file_fw_data + curIndex;
                   curIndex =  curIndex + PageSize;
                   res = WritePage(szBuff, PageSize);
#endif

#if 0
                   if(iPage==249 || iPage==1)
                   {
                            mdelay(300);                       
                   }
                   else
                   {
                            mdelay(50);                         
                   }
                                        
#endif 
                   mdelay(50);
                   res = GetAckData(client);

                   if (ACK_OK != res) 
                   {
                            mdelay(50); 
                            printkElan("[ELAN] ERROR: GetAckData fail! res=%d\r\n", res);
                            if ( res == ACK_REWRITE ) 
                            {
                                     rewriteCnt = rewriteCnt + 1;
                                     if (rewriteCnt == PAGERETRY)
                                     {
                                               printkElan("[ELAN] ID 0x%02x %dth page ReWrite %d times fails!\n", data, iPage, PAGERETRY);
                                               return E_FD;
                                     }
                                     else
                                     {
                                               printkElan("[ELAN] ---%d--- page ReWrite %d times!\n",  iPage, rewriteCnt);
                                               curIndex = curIndex - PageSize;
                                               goto PAGE_REWRITE;
                                     }
                            }
                            else
                          {
                                     restartCnt = restartCnt + 1;
                                     if (restartCnt >= 5)
                                     {
                                               printkElan("[ELAN] ID 0x%02x ReStart %d times fails!\n", data, IAPRESTART);
                                               return E_FD;
                                     }
                                     else
                                     {
                                               printkElan("[ELAN] ===%d=== page ReStart %d times!\n",  iPage, restartCnt);
                                               goto IAP_RESTART;
                                     }
                            }
                   }
                   else
                   {       printkElan("  data : 0x%02x ",  data);  
                            rewriteCnt=0;
                            print_progress(iPage,ic_num,i);
                   }

                   mdelay(10);
         } // end of for(iPage = 1; iPage <= PageNum; iPage++)

         //if (IAPReset() > 0)
                   printkElan("[ELAN] Update ALL Firmware successfully!\n");
         return 0;
}

#endif
// End Firmware Update

static ssize_t elan_ktf2k_gpio_show(struct device *dev,
                   struct device_attribute *attr, char *buf)
{
         int ret = 0;
         struct elan_ktf2k_ts_data *ts = private_ts;

         //ret = gpio_get_value(ts->intr_gpio);
         ret = mt_get_gpio_in(GPIO_CTP_EINT_PIN);
         printkElan(KERN_DEBUG "GPIO_TP_INT_N=%d\n", ts->intr_gpio);
         sprintf(buf, "GPIO_TP_INT_N=%d\n", ret);
         ret = strlen(buf) + 1;
         return ret;
}

static DEVICE_ATTR(gpio, S_IRUGO, elan_ktf2k_gpio_show, NULL);

static ssize_t elan_ktf2k_vendor_show(struct device *dev,
                   struct device_attribute *attr, char *buf)
{
         ssize_t ret = 0;
         struct elan_ktf2k_ts_data *ts = private_ts;

         sprintf(buf, "%s_x%4.4x\n", "ELAN_KTF2K", ts->fw_ver);
         ret = strlen(buf) + 1;
         return ret;
}
#if 0
static DEVICE_ATTR(vendor, S_IRUGO, elan_ktf2k_vendor_show, NULL);

static struct kobject *android_touch_kobj;

static int elan_ktf2k_touch_sysfs_init(void)
{
         int ret ;

         android_touch_kobj = kobject_create_and_add("android_touch", NULL) ;
         if (android_touch_kobj == NULL) {
                   printkElan(KERN_ERR "[elan]%s: subsystem_register failed\n", __func__);
                   ret = -ENOMEM;
                   return ret;
         }
         ret = sysfs_create_file(android_touch_kobj, &dev_attr_gpio.attr);
         if (ret) {
                   printkElan(KERN_ERR "[elan]%s: sysfs_create_file failed\n", __func__);
                   return ret;
         }
         ret = sysfs_create_file(android_touch_kobj, &dev_attr_vendor.attr);
         if (ret) {
                   printkElan(KERN_ERR "[elan]%s: sysfs_create_group failed\n", __func__);
                   return ret;
         }
         return 0 ;
}

static void elan_touch_sysfs_deinit(void)
{
         sysfs_remove_file(android_touch_kobj, &dev_attr_vendor.attr);
         sysfs_remove_file(android_touch_kobj, &dev_attr_gpio.attr);
         kobject_del(android_touch_kobj);
}        
#endif


static int elan_ktf2k_ts_poll(struct i2c_client *client)
{
         struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
         int status = 0, retry = 10;

         do {
                   //status = gpio_get_value(ts->intr_gpio);
                   status = mt_get_gpio_in(GPIO_CTP_EINT_PIN);
                   printkElan("mtk-tpd:[elan]: %s: status = %d\n", __func__, status);
                   retry--;
                   mdelay(20);
         } while (status == 1 && retry > 0);

         printkElan( "mtk-tpd:[elan]%s: poll interrupt status %s\n",
                            __func__, status == 1 ? "high" : "low");
         
         //status=0;
         //printkElan("[elan]: %s: force status = 0\n", __func__);

         return (status == 0 ? 0 : -ETIMEDOUT);
}

static int elan_ktf2k_ts_get_data(struct i2c_client *client, uint8_t *cmd,
                            uint8_t *buf, size_t size)
{
         int rc;

         dev_dbg(&client->dev, "[elan]%s: enter\n", __func__);

         if (buf == NULL)
                   return -EINVAL;

         if ((i2c_master_send(client, cmd, 4)) != 4) {
                   dev_err(&client->dev,
                            "[elan]%s: i2c_master_send failed\n", __func__);
                   return -EINVAL;
         }

         rc = elan_ktf2k_ts_poll(client);
         if (rc < 0){
                   return -EINVAL;
         }
         else {

                   if (i2c_master_recv(client, buf, size) != size ||buf[0] != CMD_S_PKT){
                       printkElan("mtk-tpd:[elan_ktf2k_ts_get_data] buf[0]=%x buf[1]=%x buf[2]=%x buf[3]=%x\n", buf[0], buf[1], buf[2], buf[3]);
                       return -EINVAL;
                   }
         }
         return 0;
}

static int __hello_packet_handler(struct i2c_client *client)
{
         int rc;
         uint8_t buf_recv[8] = { 0 };
         //uint8_t buf_recv1[4] = { 0 };

         //mdelay(1500);
          mdelay(100);
         rc = elan_ktf2k_ts_poll(client);
         if (rc < 0) {
                   printkElan( "mtk-tpd:[elan] %s: Int poll failed!\n", __func__);
                   RECOVERY=0x80;
                   return RECOVERY;  
         }

         rc = i2c_master_recv(client, buf_recv, 8);

         printkElan("mtk-tpd:[elan] %s: Hello Packet %2x:%2x:%2x:%2x\n", __func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3]);
/*  Received 8 bytes data will let TP die on old firmware on ektf21xx carbon player and MK5     
    rc = i2c_master_recv(client, buf_recv, 8);
         printkElan("[elan] %s: hello packet %2x:%2X:%2x:%2x:%2x:%2X:%2x:%2x\n", __func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3] , buf_recv[4], buf_recv[5], buf_recv[6], buf_recv[7]);
*/
         if(buf_recv[0]==0x55 && buf_recv[1]==0x55 && buf_recv[2]==0x80 && buf_recv[3]==0x80)
         {
        RECOVERY=0x80;

        rc = i2c_master_recv(client, buf_recv, 8);

        printkElan("mtk-tpd:[elan] %s: Bootcode Verson %2x:%2X:%2x:%2x\n", __func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3]);
        return RECOVERY; 
         }

         return 0;
}

static int __fw_packet_handler(struct i2c_client *client)
{
	int rc;
	int major, minor;
	uint8_t cmd[] = {CMD_R_PKT, 0x00, 0x00, 0x01};  	/* Get Firmware Version*/
	uint8_t cmd_x[] = {0x53, 0x60, 0x00, 0x00};     	/*Get x resolution*/
	uint8_t cmd_y[] = {0x53, 0x63, 0x00, 0x00};     	/*Get y resolution*/
	uint8_t cmd_id[] = {0x53, 0xf0, 0x00, 0x01};   	/*Get firmware ID*/
	//uint8_t cmd_bc[] = {CMD_R_PKT, 0x01, 0x00, 0x01};		/* Get BootCode Version-for 22xx*/
	uint8_t cmd_bc[] = {CMD_R_PKT, 0x10, 0x00, 0x01};	/* Get BootCode Version-for 21xx*/
	uint8_t buf_recv[8] = {0};

	printkElan( "mtk-tpd:[elan] %s: \n", __func__);

#if 1
// Firmware version
         rc = elan_ktf2k_ts_get_data(client, cmd, buf_recv, 4);
         if (rc < 0){
            return rc;
         }
         major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
         minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
//      ts->fw_ver = major << 8 | minor;
        FW_VERSION = major << 8 | minor;

 //zx++
    {
	extern char tpd_desc[50];
	//extern int tpd_fw_version;
	sprintf(tpd_desc, "%s-%s(0x%x)","Elan", tpd_device_driver.tpd_device_name,
        FW_VERSION);
	printkElan("[elan] tpd_desc_show = %d\n",FW_VERSION);
	//tpd_fw_version = FW_VERSION;
    }
    //zx--

#endif
         
#if 1
// Firmware ID
         rc = elan_ktf2k_ts_get_data(client, cmd_id, buf_recv, 4);
         if (rc < 0){
            return rc;
         }
         major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
         minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
         //ts->fw_id = major << 8 | minor;
         FW_ID = major << 8 | minor;
#endif

#if 1
// X Resolution
         rc = elan_ktf2k_ts_get_data(client, cmd_x, buf_recv, 4);
         if (rc < 0){
            return rc;
         }
         minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
         //ts->x_resolution =minor;
         X_RESOLUTION = minor;
#endif

#if 1        
// Y Resolution          
         rc = elan_ktf2k_ts_get_data(client, cmd_y, buf_recv, 4);
         if (rc < 0){
            return rc;
         }
         minor = ((buf_recv[2])) | ((buf_recv[3] & 0xf0) << 4);
         //ts->y_resolution =minor;
         Y_RESOLUTION = minor;
#endif

#if 1                             
// Bootcode version
         rc = elan_ktf2k_ts_get_data(client, cmd_bc, buf_recv, 4);
         if (rc < 0){
            return rc;
         }
         major = ((buf_recv[1] & 0x0f) << 4) | ((buf_recv[2] & 0xf0) >> 4);
         minor = ((buf_recv[2] & 0x0f) << 4) | ((buf_recv[3] & 0xf0) >> 4);
         //ts->bc_ver = major << 8 | minor;
         BC_VERSION = major << 8 | minor;
#endif
         
         printkElan( "mtk-tpd:[elan] %s: firmware version: 0x%4.4x\n",
                            __func__, FW_VERSION);
         printkElan( "mtk-tpd:[elan] %s: firmware ID: 0x%4.4x\n",
                            __func__, FW_ID);
         printkElan( "mtk-tpd:[elan] %s: x resolution: %d, y resolution: %d\n",
                            __func__, X_RESOLUTION, Y_RESOLUTION);
         printkElan( "mtk-tpd:[elan] %s: bootcode version: 0x%4.4x\n",
                            __func__, BC_VERSION);
         return 0;
}

static inline int elan_ktf2k_ts_parse_xy(uint8_t *data,
                            uint16_t *x, uint16_t *y)
{
         *x = *y = 0;

         *x = (data[0] & 0xf0);
         *x <<= 4;
         *x |= data[1];

         *y = (data[0] & 0x0f);
         *y <<= 8;
         *y |= data[2];

         return 0;
}

static int elan_ktf2k_ts_setup(struct i2c_client *client)
{
         int rc;
   
         rc = __hello_packet_handler(client);
         printkElan("[elan] hellopacket's rc = %d\n",rc);

         mdelay(10);
         if (rc != 0x80){
             rc = __fw_packet_handler(client);
             if (rc < 0){
             	printkElan("mtk-tpd:[elan] %s, fw_packet_handler fail, rc = %d", __func__, rc);
             }
             else{
                  printkElan("mtk-tpd:[elan] %s: firmware checking done.\n", __func__);
             }
			 /* Check for FW_VERSION, if 0x0000 means FW update fail! */
             if ( FW_VERSION == 0x00)
             {
                   rc = 0x80;
                   printkElan("mtk-tpd:[elan] FW_VERSION = %d, last FW update fail\n", FW_VERSION);
             }
         }
         return rc; /* Firmware need to be update if rc equal to 0x80(Recovery mode)   */
}

static int elan_ktf2k_ts_rough_calibrate(struct i2c_client *client){
      uint8_t cmd[] = {CMD_W_PKT, 0x29, 0x00, 0x01};

         printkElan("[elan] %s: enter\n", __func__);
         printkElan("[elan] dump cmd: %02x, %02x, %02x, %02x\n",
                   cmd[0], cmd[1], cmd[2], cmd[3]);

         if ((i2c_master_send(client, cmd, sizeof(cmd))) != sizeof(cmd)) {
                   dev_err(&client->dev,
                            "[elan] %s: i2c_master_send failed\n", __func__);
                   return -EINVAL;
         }

         return 0;
}

static int elan_ktf2k_ts_set_power_state(struct i2c_client *client, int state)
{
         uint8_t cmd[] = {CMD_W_PKT, 0x50, 0x00, 0x01};

         dev_dbg(&client->dev, "[elan] %s: enter\n", __func__);

         cmd[1] |= (state << 3);

         dev_dbg(&client->dev,
                   "[elan] dump cmd: %02x, %02x, %02x, %02x\n",
                   cmd[0], cmd[1], cmd[2], cmd[3]);

         if ((i2c_master_send(client, cmd, sizeof(cmd))) != sizeof(cmd)) {
                   dev_err(&client->dev,
                            "[elan] %s: i2c_master_send failed\n", __func__);
                   return -EINVAL;
         }

         return 0;
}

static int elan_ktf2k_ts_get_power_state(struct i2c_client *client)
{
         int rc = 0;
         uint8_t cmd[] = {CMD_R_PKT, 0x50, 0x00, 0x01};
         uint8_t buf[4], power_state;

         rc = elan_ktf2k_ts_get_data(client, cmd, buf, 4);
         if (rc)
           return rc;

         power_state = buf[1];
         dev_dbg(&client->dev, "[elan] dump repsponse: %0x\n", power_state);
         power_state = (power_state & PWR_STATE_MASK) >> 3;
         dev_dbg(&client->dev, "[elan] power state = %s\n",power_state == PWR_STATE_DEEP_SLEEP ? "Deep Sleep" : "Normal/Idle");

         return power_state;
}

static int elan_ktf2k_read_block(struct i2c_client *client, u8 addr, u8 *data, u8 len)
{
    int err;
    u8 beg = addr;
    struct i2c_msg msgs[2] = {
        {
            .addr = client->addr,    
            .flags = 0,
            .len = 1,                
            .buf= &beg
        },
        {
            .addr = client->addr,    
            .flags = I2C_M_RD,
            .len = len,             
            .buf = data,
            //.ext_flag = I2C_DMA_FLAG,
        }
    };
   
    if (!client)
        return -EINVAL;

    err = i2c_transfer(client->adapter, msgs, sizeof(msgs)/sizeof(msgs[0]));
    if (err != len) {
        printkElan("[elan] elan_ktf2k_read_block err=%d\n", err);
        err = -EIO;
    } else {
        printkElan("[elan] elan_ktf2k_read_block ok\n");
        err = 0;    //no error
    }
    return err;
}


static int elan_ktf2k_ts_recv_data(struct i2c_client *client, uint8_t *buf)
{
         int rc, bytes_to_recv=PACKET_SIZE;
         uint8_t *pReadData = 0;
         unsigned short addr = 0;

         if (buf == NULL)
                   return -EINVAL;
         memset(buf, 0, bytes_to_recv);

#ifdef _DMA_MODE_
         addr = client->addr ;
         client->addr |= I2C_DMA_FLAG;
         pReadData = gpDMABuf_va;
  if(!pReadData){
	printkElan("mtk-tpd:[elan] dma_alloc_coherent failed!\n");
  }
         rc = i2c_master_recv(client, gpDMABuf_pa, bytes_to_recv);
         copy_to_user(buf, pReadData, bytes_to_recv);
         client->addr = addr;
         #ifdef ELAN_DEBUG
        MTK_TP_DEBUG("[elan_debug] %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15],buf[16], buf[17]);
         #endif
         
#else         
         #ifdef NON_MTK_MODE //I2C support > 8bits transfer
         rc = i2c_master_recv(client, buf, bytes_to_recv);           //for two finger and non-mtk five finger and ten finger
         if (rc != bytes_to_recv)
                   printkElan("mtk-tpd:[elan_debug] The package error.\n");
         MTK_TP_DEBUG("[elan_recv] %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7],buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15],buf[16], buf[17]);
         #else    
         rc = i2c_master_recv(client, buf, 8);        //for two finger and non-mtk five finger and ten finger
         if (rc != 8)
                   printkElan("mtk-tpd:[elan_debug] The first package error.\n");
         MTK_TP_DEBUG("[elan_recv] %x %x %x %x %x %x %x %x\n", buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
         msleep(1);
         
         if (buf[0] == MTK_FINGERS_PKT) {                      //for mtk five finger
                   rc = i2c_master_recv(client, buf+ 8, 8);  
                   if (rc != 8)
                            printkElan("mtk-tpd:[elan_debug] The second package error.\n");
                   MTK_TP_DEBUG("[elan_recv] %x %x %x %x %x %x %x %x\n", buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
                   
                   rc = i2c_master_recv(client, buf+ 16, 2);
                   if (rc != 2)
                            printkElan("mtk-tpd:[elan_debug] The third package error.\n");
                   MTK_TP_DEBUG("[elan_recv] %x %x \n", buf[16], buf[17]);
                   
         } else if (buf[0] == TEN_FINGERS_PKT) { //for ten finger
                                                         rc = i2c_master_recv(client, buf+ 8, 8);  
                   if (rc != 8)
                            printkElan("[elan_debug] The second package error.\n");
                   MTK_TP_DEBUG("[elan_recv] %x %x %x %x %x %x %x %x\n", buf[8], buf[9], buf[10], buf[11], buf[12], buf[13], buf[14], buf[15]);
                   
                   rc = i2c_master_recv(client, buf+ 16, 8);
                   if (rc != 8)
                            printkElan("mtk-tpd:[elan_debug] The third package error.\n");
                   MTK_TP_DEBUG("[elan_recv] %x %x %x %x %x %x %x %x\n", buf[16], buf[17], buf[18], buf[19], buf[20], buf[21], buf[22], buf[23]);
                   
                    rc = i2c_master_recv(client, buf+ 24, 8);
                   if (rc != 8)
                            printkElan("[elan_debug] The four package error.\n");
                   MTK_TP_DEBUG("[elan_recv] %x %x %x %x %x %x %x %x\n", buf[24], buf[25], buf[26], buf[27], buf[28], buf[29], buf[30], buf[31]);
                   
                    rc = i2c_master_recv(client, buf+ 32, 8);
                   if (rc != 8)
                            printkElan("mtk-tpd:[elan_debug] The five package error.\n");
                   MTK_TP_DEBUG("mtk-tpd:[elan_recv] %x %x %x %x %x %x %x %x\n", buf[32], buf[33], buf[34], buf[35], buf[36], buf[37], buf[38], buf[39]);
                   
                   rc = i2c_master_recv(client, buf+ 40, 4);
                   if (rc != 4)
                            printkElan("mtk-tpd:[elan_debug] The six package error.\n");
                   MTK_TP_DEBUG("mtk-tpd:[elan_recv] %x %x %x %x\n", buf[40], buf[41], buf[42], buf[43]);
                   
         }
         #endif
#endif       
         
         return rc;
}
//{ =======================================================================================iap-ram
//ifdef IAPRAM_MODE
int IAP_RAM( struct i2c_client *client )
{

	int res = 0,ic_num = 1;
	int iPage = 0, rewriteCnt = 0; //rewriteCnt for PAGE_REWRITE
	int i = 0;
	uint8_t data;
	//struct timeval tv1, tv2;
	int restartCnt = 0; // For IAP_RESTART
	uint8_t *szBuff = NULL;
	int curIndex = 0;
	//uint8_t recovery_buffer[4] = {0};
	int byte_count;
	uint8_t iapRam_cmd[] = {0x22, 0x22, 0x22, 0x22};

//	dev_dbg(&client->dev, "[ELAN] %s:  ic_num=%d\n", __func__, ic_num);
IAP_RESTART:	
    //init variable
    curIndex = 0;
	//reset
	printkElan("[ELAN] IAPRAM_MODE\n");
	data= IAP_RAM_ADDR;
//	dev_dbg(&client->dev, "[ELAN] %s: address data=0x%x \r\n", __func__, data);
	res = EnterISPMode( /*private_ts->*/client, iapRam_cmd );	 //enter ISP mode
	if( res != 0 )
	{
		printkElan("[elan] EnterISPMode error code = %d\n",res);
		return E_FD;
	}	
//mdelay(500);

	// Send Dummy Byte	
	printkElan("[ELAN] send one byte data:%x,%x",client->addr,data);
	res = i2c_master_send(client, &data,  sizeof(data));
	if( res!=sizeof(data) )
	{
		printkElan("[ELAN] dummy error code = %d\n",res);
		return E_FD;
	}	
	msleep(10);


	// Start IAP
	for( iPage = 1; iPage <= 17; iPage++ ) 
	{
PAGE_REWRITE:
#if 1// 8byte mode
		//szBuff = fw_data + ((iPage-1) * PageSize); 
		for(byte_count=1;byte_count<=17;byte_count++)
		{
			if(byte_count!=17)
			{		
	//			printkElan("[ELAN] byte %d\n",byte_count);	
	//			printkElan("curIndex =%d\n",curIndex);
				szBuff = file_iapRam_data + curIndex;
				curIndex =  curIndex + 8;

				//ioctl(fd, IOCTL_IAP_MODE_LOCK, data);
				res = WritePage(client, szBuff, 8);
				if( res != 0 )
				{
					printkElan("[elan] IAP-RAM WRITE error code = %d\n",res);
					return E_FD;
				}
			}
			else
			{
	//			printkElan("byte %d\n",byte_count);
	//			printkElan("curIndex =%d\n",curIndex);
				szBuff = file_iapRam_data + curIndex;
				curIndex =  curIndex + 4;
				//ioctl(fd, IOCTL_IAP_MODE_LOCK, data);
				res = WritePage(client, szBuff, 4); 
				if( res != 0 )
				{
					printkElan("[elan] IAP-RAM WRITE error code = %d\n",res);
					return E_FD;
				}
			}
		} // end of for(byte_count=1;byte_count<=17;byte_count++)
#else // 132byte mode		
		szBuff = file_iapRam_data + curIndex;
		curIndex =  curIndex + PageSize;
		res = WritePage(client, szBuff, PageSize);
#endif
//#if 0
		msleep(5); 			 
		res = GetAckData(/*private_ts->*/client);
		if (ACK_OK != res) 
		{
		//	mdelay(50); 
			printkElan("[ELAN] ERROR: GetAckData fail! res=%d\r\n", res);
			if ( res == ACK_REWRITE ) 
			{
				rewriteCnt = rewriteCnt + 1;
				if (rewriteCnt == 5)
				{
					printkElan("[ELAN] ID 0x%02x %dth page ReWrite %d times fails!\n", data, iPage, PAGERETRY);
					return E_FD;
				}
				else
				{
					printkElan("[ELAN] ---%d--- page ReWrite %d times!\n",  iPage, rewriteCnt);
					curIndex = curIndex - PageSize;
					goto PAGE_REWRITE;
				}
			}
			else
			{
				restartCnt = restartCnt + 1;
				if (restartCnt >= 2)
				{
					printkElan("[ELAN] ID 0x%02x ReStart %d times fails!\n", data, IAPRESTART);
					return E_FD;
				}
				else
				{
					printkElan("[ELAN] ===%d=== page ReStart %d times!\n",  iPage, restartCnt);
					goto IAP_RESTART;
				}
			}
		}
		else
		{       
//			printkElan("  data : 0x%02x ",  data);  
			rewriteCnt=0;
//			print_progress(iPage,ic_num,i);
		}

//		mdelay(10);
	} // end of for(iPage = 1; iPage <= PageNum; iPage++)
//cc_packet_handler(client);
	//if (IAPReset(client) > 0)

// reset


#if 0
	mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
    	mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
    	mt_set_gpio_out(GPIO_CTP_RST_PIN, 0);
	msleep(20);

    // for enable/reset pin
    	mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
    	mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
    	mt_set_gpio_out(GPIO_CTP_RST_PIN, 1);
	msleep(10);

#endif
		printkElan("[ELAN] IAP_RAM successfully!\n");
	return 0;
}

 
static int elan_ktf2k_ts_iapRam_continue(struct i2c_client *client)
{
	uint8_t cmd[] = { 0x33, 0x33, 0x33, 0x33 };

	printkElan("[elan]: %s %02x, %02x, %02x, %02x\n",__func__, cmd[0], cmd[1], cmd[2], cmd[3]);
	if ((i2c_master_send(client, cmd, sizeof(cmd))) != sizeof(cmd)) {
		printkElan("[elan] %s: i2c_master_send failed\n", __func__);
		return -EINVAL;
	}

	return 0;
}
/*
static int elan_ktf2k_ts_iapRam_prepare(struct i2c_client *client)
{
	uint8_t cmd[] = { 0x22, 0x22, 0x22, 0x22 };
	dev_dbg(&client->dev, "[elan] %s: enter\n", __func__);
	dev_dbg(&client->dev,
		"[elan] dump cmd: %02x, %02x, %02x, %02x\n",
		cmd[0], cmd[1], cmd[2], cmd[3]);

	if ((i2c_master_send(client, cmd, sizeof(cmd))) != sizeof(cmd)) {
		dev_err(&client->dev,
			"[elan] %s: i2c_master_send failed\n", __func__);
		return -EINVAL;
	}

	return 0;
}
*/


//Check if we get [cc cc cc cc] packet
static int cc_packet_handler(struct i2c_client *client)
//static int cc_packet_handler(uint8_t *buf_recv)
{
	int rc;
	uint8_t buf_recv[8] = { 0 };

	msleep(5);	
/*	rc = elan_ktf2k_ts_poll(client);
	if (rc < 0) {
		printkElan( "[elan] %s: Int poll failed!\n", __func__);
	//	RECOVERY=0x80;
		//return RECOVERY;
		//return -EINVAL;
	}*/
	rc = i2c_master_recv(client, buf_recv, 8);
	printkElan("[elan] %s: cc packet %2x:%2x:%2x:%2x:%2x:%2X:%2x:%2x\n", __func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3] , buf_recv[4], buf_recv[5], buf_recv[6], buf_recv[7]);

	printkElan("[elan] %s: test %2x:%2x:%2x:%2x\n", __func__, file_iapRam_data[2241], file_iapRam_data[2240], file_iapRam_data[2239], file_iapRam_data[2238]);
	if(buf_recv[0]== 0xcc && buf_recv[1]== 0xcc && buf_recv[2]==0xcc && buf_recv[3]==0xcc)
	{

		if( buf_recv[4]== file_iapRam_data[2241] && buf_recv[5]== file_iapRam_data[2240] //8c0
			&& buf_recv[6]== file_iapRam_data[2239] && buf_recv[7]== file_iapRam_data[2238] )
		{
			printkElan("[elan] checksum is right\n");
			return 1;
		}
		else
		{
			printkElan("[elan] checksum is Error, Need to be updated\n");	
			return 0;
		}
	}
/*
	else 
	{
		return 0;
	}
	return -1; 
*/
}

//}======================================================================================= iap-ram
// endif	


#ifdef SOFTKEY_AXIS_VER //SOFTKEY is reported via AXI
static void elan_ktf2k_ts_report_data(struct i2c_client *client, uint8_t *buf)
{
         //struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
         struct input_dev *idev = tpd->dev;
         uint16_t x, y;
         uint16_t fbits=0;
         uint8_t i, num, reported = 0;
         uint8_t idx, btn_idx;
         int finger_num;
         int limitY = Y_RESOLUTION - 100; // limitY need define by Case!
/* for 10 fingers       */
         if (buf[0] == TEN_FINGERS_PKT){
                  finger_num = 10;
                  num = buf[2] & 0x0f; 
                  fbits = buf[2] & 0x30;       
                       fbits = (fbits << 4) | buf[1]; 
                  idx=3;
                       btn_idx=33;
      }
// for 5 fingers 
         else if ((buf[0] == MTK_FINGERS_PKT) || (buf[0] == FIVE_FINGERS_PKT)){
                  finger_num = 5;
                  num = buf[1] & 0x07; 
        fbits = buf[1] >>3;
                  idx=2;
                  btn_idx=17;
         }else{
// for 2 fingers      
                   finger_num = 2;
                   num = buf[7] & 0x03; 
                   fbits = buf[7] & 0x03;
                   idx=1;
                   btn_idx=7;
        }

         switch (buf[0]) {
                   case MTK_FINGERS_PKT:
                   case TWO_FINGERS_PKT:
                   case FIVE_FINGERS_PKT:         
                   case TEN_FINGERS_PKT:
                            //input_report_key(idev, BTN_TOUCH, 1);
                            if (num == 0)
                            {
                                     //dev_dbg(&client->dev, "no press\n");
                                     if(key_pressed < 0){
                                               input_report_key(tpd->dev, BTN_TOUCH, 0);
                                               input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 0);
                                               input_report_abs(idev, ABS_MT_WIDTH_MAJOR, 0);
                                               input_mt_sync(idev);
                                               if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
                                               {   
                                               tpd_button(x, y, 0);  
                                               }
                                               TPD_EM_PRINT(x, y, x, y, 0, 0);
                                     }
                                     else{
                                               //dev_err(&client->dev, "[elan] KEY_RELEASE: key_code:%d\n",OSD_mapping[key_pressed].key_event);
                                               input_report_key(idev, OSD_mapping[key_pressed].key_event, 0);
                                               key_pressed = -1;
                                     }
                            }
                            else 
                            {                           
                                     //dev_dbg(&client->dev, "[elan] %d fingers\n", num);                        
                                     //input_report_key(idev, BTN_TOUCH, 1);
                                     for (i = 0; i < finger_num; i++) 
                                     {        
                                               if ((fbits & 0x01)) 
                                               {
                                                        elan_ktf2k_ts_parse_xy(&buf[idx], &x, &y);  
                                                        //elan_ktf2k_ts_parse_xy(&buf[idx], &y, &x);
                                                        //x = X_RESOLUTION-x;   
                                                        //y = Y_RESOLUTION-y; 
#if 0
         if(X_RESOLUTION > 0 && Y_RESOLUTION > 0)
         {
                   x = ( x * LCM_X_MAX )/X_RESOLUTION;
                   y = ( y * LCM_Y_MAX )/Y_RESOLUTION;
         }
         else
         {
                   x = ( x * LCM_X_MAX )/ELAN_X_MAX;
                   y = ( y * LCM_Y_MAX )/ELAN_Y_MAX;
         }
#endif                 
                                                        MTK_TP_DEBUG("[elan_debug SOFTKEY_AXIS_VER] %s, x=%d, y=%d\n",__func__, x , y);
                                                                                         
                                                        if (!((x<=0) || (y<=0) || (x>=X_RESOLUTION) || (y>=Y_RESOLUTION))) 
                                                        {   
                                                                 if ( y < limitY )
                                                                 {
                                                                                                                 MTK_TP_DEBUG("mtk-tpd elan_ktf2k_ts_report_data x=%d y=%d id=%d \n",x,y,i);
                                                                                                                 input_report_key(tpd->dev, BTN_TOUCH, 1);
                                                                                                                 input_report_abs(idev, ABS_MT_TRACKING_ID, i);
                                                                           input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 8);
                                                                           input_report_abs(idev, ABS_MT_POSITION_X, x);
                                                                           input_report_abs(idev, ABS_MT_POSITION_Y, y);
                                                                           input_mt_sync(idev);
                                                                           if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
                                                                           {   
                                                                           tpd_button(x, y, 1);  
                                                                           }
                                                                           TPD_EM_PRINT(x, y, x, y, i-1, 1);
                                                                 }
                                                                 else
                                                                 {
                                                                                             int i=0;
                                                                                             for(i=0;i<4;i++)
                                                                                             {
                                                                           if((x > OSD_mapping[i].left_x) && (x < OSD_mapping[i].right_x))
                                                                           {
                                                                                    //dev_err(&client->dev, "[elan] KEY_PRESS: key_code:%d\n",OSD_mapping[i].key_event);
                                                                                    //printkElan("[elan] %d KEY_PRESS: key_code:%d\n", i, OSD_mapping[i].key_event);
                                                                                    input_report_key(idev, OSD_mapping[i].key_event, 1);
                                                                                    key_pressed = i;
                                                                           }
                                                                                             }
                                                                 }
                                                                 reported++;
                                                                 
                                                        } // end if border
                                               } // end if finger status
                                              fbits = fbits >> 1;
                                              idx += 3;
                                     } // end for
                            }

                            if (reported)
                                     input_sync(idev);
                            else 
                            {
                                     input_mt_sync(idev);
                                     input_sync(idev);
                                     MTK_TP_DEBUG("mtk-tpd elan_ktf2k_ts_report_data up\n");
                            }

                            break;
                  default:
                                     MTK_TP_DEBUG("[elan] %s: unknown packet type: %0x\n", __func__, buf[0]);
                                     #if 0
                                     if(buf[0]==0x66)
                                     {
                                               uint8_t cmd[] = {CMD_W_PKT, 0x50, 0x00, 0x01};

                                               printkElan("[elan] TP enter into sleep mode\n");
                                               if ((i2c_master_send(private_ts->client, cmd, sizeof(cmd))) != sizeof(cmd)) 
                                               {
                                                        printkElan("[elan] %s: i2c_master_send failed\n", __func__);
                                                        //return -retval;
                                               }
                                               mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
                                               mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
                                               mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
                                               mdelay(10);
                                               mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);                              
                                               mdelay(10);
                                               mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
                                               mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
                                               printkElan("mtk-tpd elan_ktf2k_ts_report_data packet 0x66 reset\n");
                                              
                                     }
                                     #endif
                                     break;
         } // end switch
         return;
}
#else //SOFTKEY is reported via BTN bit
static void elan_ktf2k_ts_report_data(struct i2c_client *client, uint8_t *buf)
{
         /*struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);*/
         struct input_dev *idev = tpd->dev;
         static uint16_t x, y;
         uint16_t fbits=0;
         uint8_t i, num, reported = 0;
         uint8_t idx, btn_idx;
         int finger_num;
	 int rc = 0;
	#ifdef TP_PROXIMITY_SENSOR_NEW
        hwm_sensor_data sensor_data;
        int ret = 0;
    #endif
// iap-ram
#if 1
	if (buf[0] == 0x55) 
	{
	
		rc = __fw_packet_handler(client);
		if (rc < 0){
		   printkElan("mtk-tpd:[elan] %s, fw_packet_handler fail, rc = %d", __func__, rc);
		}
		else{
			 printkElan("mtk-tpd:[elan] %s: firmware checking done.\n", __func__);
		}
	}
	else if (buf[0] == 0xcc) 
	{
		mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
		//msleep(100);

		printkElan("[elan] %s: cc packet %2x:%2x:%2x:%2x:%2x:%2X:%2x:%2x\n", __func__, buf[0], buf[1], buf[2], buf[3] , buf[4], buf[5], buf[6], buf[7]);
		printkElan("[elan] %s: via rom data %2x:%2x:%2x:%2x\n", __func__, file_iapRam_data[2241], file_iapRam_data[2240], file_iapRam_data[2239], file_iapRam_data[2238]);
		if(buf[0]== 0xcc && buf[1]== 0xcc && buf[2]==0xcc && buf[3]==0xcc)
		{

			if( buf[4]== file_iapRam_data[2241] && buf[5]== file_iapRam_data[2240] //8c0
				&& buf[6]== file_iapRam_data[2239] && buf[7]== file_iapRam_data[2238] )
			{
				printkElan("[elan] checksum is right\n");
				elan_ktf2k_ts_iapRam_continue(i2c_client);
//				msleep(500);
			
			}
			else
			{
				printkElan("[elan] checksum is Error, Need to be updated\n");	
				IAP_RAM(i2c_client);

				msleep(10);	
				rc = cc_packet_handler(i2c_client);	                           
				printkElan("[elan] %s Enter into normal report system\n", __func__);
				elan_ktf2k_ts_iapRam_continue(i2c_client);
			//	msleep(500);
			}
		}
		mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	}
#ifdef TP_PROXIMITY_SENSOR_NEW
	else if(buf[0] == 0xFA)
	{
	   if(buf[2] == 0xAA && PROXIMITY == 1 ) // close to
		 {
			 printkElan("TP_PROXIMITY_SENSOR VVV by cheehwa\n");
			 PROXIMITY_STATE = 0;
			 printkElan("tpd_touchinfo PROXIMITY_STATE %d\n",PROXIMITY_STATE );
		 }
		 else if(buf[2] == 0x55 && PROXIMITY == 1 ) // leave
		 {
			printkElan("TP_PROXIMITY_SENSOR XXX by cheehwa\n");
			 PROXIMITY_STATE = 1;
			printkElan("tpd_touchinfo PROXIMITY_STATE %d\n",PROXIMITY_STATE );
		 }				 
	} 
	
	if (PROXIMITY == 1)
	{
	   //get raw data
/*	   TPD_DEBUG(" ps change\n");
	   //map and store data to hwm_sensor_data
	   sensor_data.values[0] = Get_Ctp_Face_Mode();
	   sensor_data.value_divide = 1;
	   sensor_data.status = SENSOR_STATUS_ACCURACY_MEDIUM;
	   //report to the up-layer
	   ret = hwmsen_get_interrupt_data(ID_PROXIMITY, &sensor_data);
	
	   if (ret)
	   {
		   TPD_DEBUG("Call hwmsen_get_interrupt_data fail = %d\n", ret);
	   }*/
	   extern void ctp_ps_event_handler(void);
	   ctp_ps_event_handler();
	}
#endif	
	#endif
//iap-ram

/* for 10 fingers       */
         if (buf[0] == TEN_FINGERS_PKT){
                  finger_num = 10;
                  num = buf[2] & 0x0f; 
                  fbits = buf[2] & 0x30;       
                       fbits = (fbits << 4) | buf[1]; 
                  idx=3;
                       btn_idx=33;
      }
// for 5 fingers 
         else if ((buf[0] == MTK_FINGERS_PKT) || (buf[0] == FIVE_FINGERS_PKT)){
                  finger_num = 5;
                  num = buf[1] & 0x07; 
        fbits = buf[1] >>3;
                  idx=2;
                  btn_idx=17;
         }else{
// for 2 fingers      
                   finger_num = 2;
                   num = buf[7] & 0x03; 
                   fbits = buf[7] & 0x03;
                   idx=1;
                   btn_idx=7;
         }
                   
         switch (buf[0]) {
                   case MTK_FINGERS_PKT:
                   case TWO_FINGERS_PKT:
                   case FIVE_FINGERS_PKT:         
                   case TEN_FINGERS_PKT:
                            //input_report_key(idev, BTN_TOUCH, 1);
                            if ((num == 0)||(button_state != 0))//no finger or BUTTON is down
                            {
                                     dev_dbg(&client->dev, "no press\n");
                                     #ifdef ELAN_DEBUG
                                     MTK_TP_DEBUG("button_state0 = %x\n",button_state);
                                     MTK_TP_DEBUG("buf[btn_idx] = %x KEY_MENU=%x KEY_HOME=%x KEY_BACK=%x KEY_SEARCH =%x\n",buf[btn_idx], KEY_MENU, KEY_HOME, KEY_BACK, KEY_SEARCH);
                                     #endif
                                     //marked by baojun.fu--2013/8/6
                                     //if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
                                     //{   
                                     //    tpd_button(x, y, 0);
                                     //    MTK_TP_DEBUG("[elan] FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode()\n\n",button_state);
                                     //}
                                     //TPD_EM_PRINT(x, y, x, y, 0, 0);
                                               
         #ifdef ELAN_BUTTON
                                                        
                                               switch (buf[btn_idx]&0xFC) {
                                              case ELAN_KEY_BACK:
                                                        MTK_TP_DEBUG("KEY back 1\n");
                                                        #ifndef LCT_VIRTUAL_KEY
                                                        input_report_key(idev, KEY_BACK, 1);
                                                        #else
                                                        input_report_key(idev, BTN_TOUCH, 1);
                                                        input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 8);
                                                        input_report_abs(idev, ABS_MT_POSITION_X, 617);
                                                        input_report_abs(idev, ABS_MT_POSITION_Y, 1360);
                                                        #endif
                                                        //factory mode
                                                        if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
                                                        {   
                                                            x=617;
                                                            y=1360;
                                                            tpd_button(x, y, 1);  
                                                            MTK_TP_DEBUG("[elan] FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode()\n\n",button_state);
                                                        }
                                                        button_state = KEY_BACK;
                                                        break;
                                                        
                                               case ELAN_KEY_HOME:
                                                        MTK_TP_DEBUG("KEY home 1\n");
                                                        #ifndef LCT_VIRTUAL_KEY
                                                        input_report_key(idev, /*KEY_HOME*/KEY_HOMEPAGE, 1);
                                                        #else
                                                        input_report_key(idev, BTN_TOUCH, 1);
                                                        input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 8);
                                                        input_report_abs(idev, ABS_MT_POSITION_X, 365);
                                                        input_report_abs(idev, ABS_MT_POSITION_Y, 1360);
                                                        #endif
                                                        //factory mode
                                                        if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
                                                        {   
                                                            x=365;
                                                            y=1360;
                                                            tpd_button(x, y, 1);  
                                                            MTK_TP_DEBUG("[elan] FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode()\n\n",button_state);
                                                        }
                                               
                                                            //button_state = /*KEY_HOMEPAGE*/KEY_HOME;
                                                            button_state = KEY_HOMEPAGE/*KEY_HOME*/;
                                                        break;
                                                        
                                               case ELAN_KEY_MENU:
                                                        MTK_TP_DEBUG("KEY menu 1\n");
                                                        #ifndef LCT_VIRTUAL_KEY
                                                        input_report_key(idev, KEY_MENU, 1);
                                                        #else
                                                        input_report_key(idev, BTN_TOUCH, 1);
                                                        input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 8);
                                                        input_report_abs(idev, ABS_MT_POSITION_X, 107);
                                                        input_report_abs(idev, ABS_MT_POSITION_Y, 1360);
                                                        #endif
                                                        //factory mode
                                                        if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
                                                        {   
                                                            x=107;
                                                            y=1360;
                                                            tpd_button(x, y, 1);  
                                                            MTK_TP_DEBUG("[elan] FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode()\n\n",button_state);
                                                        }
                                                          button_state = KEY_MENU;
                                                        
                                                                 break;
                                     
                                 // TOUCH release
                                                        default:             
                                                                 MTK_TP_DEBUG("mtk-tpd:[ELAN ] test tpd up\n");
                                                                 input_report_key(idev, BTN_TOUCH, 0);
                                                                 //input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 0);
                                                                 input_report_abs(idev, ABS_MT_WIDTH_MAJOR, 0);
                                                                 input_mt_sync(idev);
                                                                tpd_down_flag = 0;
                                                                //factory mode
                                                                if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
                                                                {   
                                                                    tpd_button(x, y, 0);  
                                                                    MTK_TP_DEBUG("[elan] FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode()\n\n",button_state);
                                                                }
                                                            
                                                                    button_state = 0;
                                                               break;
                                         }
                                                                             
                //input_sync(idev);     
#endif                      
                   }
                            else 
                            {                           
                                     //dev_dbg(&client->dev, "[elan] %d fingers\n", num);                        
                                     input_report_key(idev, BTN_TOUCH, 1);
                                     for (i = 0; i < finger_num; i++) 
                                     {        
                                               if ((fbits & 0x01)) 
                                               {
                                                        elan_ktf2k_ts_parse_xy(&buf[idx], &x, &y);  
			  
                                                       // elan_ktf2k_ts_parse_xy(&buf[idx], &y, &x); 
							       //  x=ELAN_X_MAX-x;
                                                        #if 1
                        if(X_RESOLUTION > 0 && Y_RESOLUTION > 0)
                        {
                            x = ( x * LCM_X_MAX )/X_RESOLUTION;
                            y = ( y * LCM_Y_MAX )/Y_RESOLUTION;
                        }
                        else
                        {
                            x = ( x * LCM_X_MAX )/ELAN_X_MAX;
                            y = ( y * LCM_Y_MAX )/ELAN_Y_MAX;
                        }
                                                        #endif                 

                                                        //x = ( x * LCM_X_MAX )/ELAN_X_MAX;
                                                        //y = ( y * LCM_Y_MAX )/ELAN_Y_MAX;
                                                        #ifdef ELAN_DEBUG
                                                        MTK_TP_DEBUG("mtk-tpd:[elan_debug  BTN bit] %s, x=%d, y=%d\n",__func__, x , y);
                                                        #endif
                                                        //x = LCM_X_MAX-x;        
                                                        //y = Y_RESOLUTION-y;                           
                                                        if (!((x>=LCM_X_MAX) || (y>=LCM_Y_MAX))) 
                                                        {   
                                                                 input_report_key(idev, BTN_TOUCH, 1);
                                                                 input_report_abs(idev, ABS_MT_TRACKING_ID, i);
                                                                 input_report_abs(idev, ABS_MT_TOUCH_MAJOR, 8);
                                                                 input_report_abs(idev, ABS_MT_POSITION_X, x);
                                                                 input_report_abs(idev, ABS_MT_POSITION_Y, y);
                                                                 input_mt_sync(idev);
                                                                 
                                                                 if (FACTORY_BOOT == get_boot_mode()|| RECOVERY_BOOT == get_boot_mode())
                                                                 {   
                                                                    tpd_button(x, y, 1);  
                                                                 }
                                                                 TPD_EM_PRINT(x, y, x, y, i-1, 1);
                                                                           
                                                                 reported++;
                                                                 tpd_down_flag=1;
                                                        } // end if border
                                               } // end if finger status
                                              fbits = fbits >> 1;
                                              idx += 3;
                                     } // end for
                            }
                            if (reported)
                                     input_sync(idev);
                            else 
                            {
                                     input_mt_sync(idev);
                                     input_sync(idev);
                            }
                            break;
                  default:
                                               MTK_TP_DEBUG("mtk-tpd:[elan] %s: unknown packet type: %0x,%0x,%0x,%0x\n", __func__, buf[0],buf[1],buf[2],buf[3]);
                                     break;
         } // end switch
         return;
}
#endif

static void elan_ktf2k_ts_work_func(struct work_struct *work)
{
         int rc;
         struct elan_ktf2k_ts_data *ts =
         container_of(work, struct elan_ktf2k_ts_data, work);
         uint8_t buf[PACKET_SIZE] = { 0 };

//               if (gpio_get_value(ts->intr_gpio))
                   if (mt_get_gpio_in(GPIO_CTP_EINT_PIN))
                   {
                            //enable_irq(ts->client->irq);
                            printkElan("[elan]: Detected Jitter at INT pin. \n");
                            mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
                            return;
                   }
         
                   rc = elan_ktf2k_ts_recv_data(ts->client, buf);

                   if (rc < 0)
                   {
                            //enable_irq(ts->client->irq);
                            printkElan("[elan] elan_ktf2k_ts_recv_data Error, Error code %d \n", rc);
                            mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
                            return;
                   }

                   //printkElan("[elan] %2x,%2x,%2x,%2x,%2x,%2x\n",buf[0],buf[1],buf[2],buf[3],buf[5],buf[6]);
                   elan_ktf2k_ts_report_data(ts->client, buf);

                   //enable_irq(ts->client->irq);
                   mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);

         return;
}

static irqreturn_t elan_ktf2k_ts_irq_handler(int irq, void *dev_id)
{
         struct elan_ktf2k_ts_data *ts = dev_id;
         struct i2c_client *client = ts->client;

         dev_dbg(&client->dev, "[elan] %s\n", __func__);
         mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
        tpd_flag = 1;
        wake_up_interruptible(&waiter);
         return IRQ_HANDLED;
}

static int elan_ktf2k_ts_register_interrupt(struct i2c_client *client)
{
         struct elan_ktf2k_ts_data *ts = i2c_get_clientdata(client);
         int err = 0;

         err = request_irq(client->irq, elan_ktf2k_ts_irq_handler,
                                                                                                       IRQF_TRIGGER_LOW, client->name, ts);
         if (err)
                   dev_err(&client->dev, "[elan] %s: request_irq %d failed\n",
                                     __func__, client->irq);

         return err;
}

#if IAP_PORTION
static int update_fw_handler(void *unused)
{
         int New_FW_ID;                
         int New_FW_VER;
         //struct i2c_client client= private_ts->client;
         
         struct sched_param param = { .sched_priority = 4 };
         sched_setscheduler(current, SCHED_RR, &param);

         if(probe_flage == 0)
         //msleep(200);

         work_lock=1;
         //disable_irq(CUST_EINT_TOUCH_PANEL_NUM);
         mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
         power_lock=1;
         printkElan("[elan] start fw update\n");
                  
/* FW ID & FW VER*/
#ifdef ELAN_3K_IC_SOLUTION
                 /*For ektf31xx iap ekt file   */       
    printkElan("[ELAN] [0x7d64]=0x%02x,  [0x7d65]=0x%02x, [0x7d66]=0x%02x, [0x7d67]=0x%02x\n",  file_fw_data[32100],file_fw_data[32101],file_fw_data[32102],file_fw_data[32103]);    
    New_FW_ID = file_fw_data[0x7d67]<<8  | file_fw_data[0x7d66] ;          
    New_FW_VER = file_fw_data[0x7d65]<<8  | file_fw_data[0x7d64] ;
    
    printkElan(" FW_ID=0x%x,   New_FW_ID=0x%x \n",  FW_ID, New_FW_ID);             
    printkElan(" FW_VERSION=0x%x,   New_FW_VER=0x%x \n",  FW_VERSION  , New_FW_VER); 
#else
                  /* For ektf21xx and ektf20xx iap ekt file  */
    printkElan("[ELAN]  [7bd0]=0x%02x,  [7bd1]=0x%02x, [7bd2]=0x%02x, [7bd3]=0x%02x\n",  file_fw_data[31696],file_fw_data[31697],file_fw_data[31698],file_fw_data[31699]);
    New_FW_ID = file_fw_data[31699]<<8  | file_fw_data[31698] ;          
    New_FW_VER = file_fw_data[31697]<<8  | file_fw_data[31696] ;
    printkElan(" FW_ID=0x%x,   New_FW_ID=0x%x \n",  FW_ID, New_FW_ID);             
    printkElan(" FW_VERSION=0x%x,   New_FW_VER=0x%x \n",  FW_VERSION  , New_FW_VER);  
#endif

#if 0
                   /* For ektf31xx 2 wire ice ex: 2wireice -b xx.bin */
    printkElan("[ELAN] [7d96]=0x%02x,  [7d97]=0x%02x, [7d98]=0x%02x, [7d99]=0x%02x\n",  file_fw_data[32150],file_fw_data[32151],file_fw_data[32152],file_fw_data[32153]);
    New_FW_ID = file_fw_data[32153]<<8  | file_fw_data[32152] ;          
    New_FW_VER = file_fw_data[32151]<<8  | file_fw_data[32150] ;
    printkElan(" FW_ID=0x%x,   New_FW_ID=0x%x \n",  FW_ID, New_FW_ID);             
    printkElan(" FW_VERSION=0x%x,   New_FW_VER=0x%x \n",  FW_VERSION  , New_FW_VER); 
#endif     
                   
                   /* for firmware auto-upgrade*/        
   if (New_FW_ID   ==  FW_ID) {                
        if (New_FW_VER != (FW_VERSION)) 
            Update_FW_One(private_ts->client, RECOVERY);
    } else if(FW_ID == 0) {                       
            printkElan("FW_ID is different!");
            RECOVERY=0x80;                  
                           Update_FW_One(private_ts->client, RECOVERY);           
    }
            
    //Update_FW_One(private_ts->client, RECOVERY);
        
    power_lock=0;           
    work_lock=0;
    //enable_irq(CUST_EINT_TOUCH_PANEL_NUM);
    mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
    printkElan("[elan] end fw update\n");
         
         kthread_should_stop();
   #ifdef _DMA_FW_UPGRADE_MODE_    
   if(gpDMAFWBuf_va){
        dma_free_coherent(NULL, 4096, gpDMAFWBuf_va, gpDMAFWBuf_pa);
        gpDMAFWBuf_va = NULL;
        gpDMAFWBuf_pa = NULL;
   }
   #endif 
                   return 0;
}
#endif

static int touch_event_handler(void *unused)
{
         int rc;
         uint8_t buf[PACKET_SIZE] = { 0 };

         int touch_state = 3;
//      int button_state = 0;
         unsigned long time_eclapse;
         struct sched_param param = { .sched_priority = RTPM_PRIO_TPD };
         sched_setscheduler(current, SCHED_RR, &param);
         int last_key = 0;
         int key;
         int index = 0;
         int i =0;
         printkElan("mtk-tpd[elan] interrupt touch_event_handler\n");

         do
         {
                   mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
                   //enable_irq(CUST_EINT_TOUCH_PANEL_NUM);
                   MTK_TP_DEBUG("mtk-tpd touch_event_handler mt65xx_eint_unmask\n");
                   set_current_state(TASK_INTERRUPTIBLE);
                   wait_event_interruptible(waiter, tpd_flag != 0);
                   tpd_flag = 0;
                   set_current_state(TASK_RUNNING);
                   //disable_irq(CUST_EINT_TOUCH_PANEL_NUM);
                   mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
                   MTK_TP_DEBUG("mtk-tpd touch_event_handler mt65xx_eint_mask\n");
                   rc = elan_ktf2k_ts_recv_data(private_ts->client, buf);
                   if (rc < 0)
                   {
                            printkElan("mtk-tpd:[elan] rc<0\n");
                            continue;
                   }

                   elan_ktf2k_ts_report_data(/*ts*/private_ts->client, buf);

         }while(!kthread_should_stop());

         return 0;
}

static int tpd_detect(struct i2c_client *client, int kind, struct i2c_board_info *info)
{
    strcpy(info->type, TPD_DEVICE);
    
    return 0;
}

static void tpd_eint_interrupt_handler(void)
{
    MTK_TP_DEBUG("TPD interrupt has been triggered\n");
//---------add in interrupt----------
#ifdef ESD_CHECK	
		have_interrupts = 1;
#endif	
    tpd_flag = 1;
    wake_up_interruptible(&waiter);
//yixuhong20131005 delete mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
}

static int __RE_K_handler(struct i2c_client *client)
{
         int rc;
         uint8_t buf_recv[4] = { 0 };
         
         rc = elan_ktf2k_ts_poll(client);
         if (rc < 0) {
                   printkElan( "[elan] %s: Int poll failed!\n", __func__);    
         }

         i2c_master_recv(client, buf_recv, 4);

         printkElan("[elan] %s: RE-K Packet %2x:%2X:%2x:%2x\n", __func__, buf_recv[0], buf_recv[1], buf_recv[2], buf_recv[3]);

         return 0;           
}



static int tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
//static int __devinit tpd_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
         int fw_err = 0;
         int New_FW_ID;      
         int New_FW_VER;   
         int retval = TPD_OK;
         static struct elan_ktf2k_ts_data ts;

//		 #ifdef TP_PROXIMITY_SENSOR_NEW
//            struct hwmsen_object obj_ps;
//            s32 err = 0;
//         #endif
       //  client->addr |= I2C_ENEXT_FLAG;
       #if 0//def TPD_POWER_SOURCE_CUSTOM
    hwPowerOn(TPD_POWER_SOURCE_CUSTOM, VOL_2800, "TP");
#else
	//hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");
#endif
         
         printkElan("mtk-tpd:[elan] %s:client , TPD_DEVICE = ektf2k\n",__func__);
         //printkElan("mtk-tpd:[elan] %s:I2C_WR_FLAG=%x,I2C_MASK_FLAG=%x,I2C_ENEXT_FLAG =%x\n",__func__,I2C_WR_FLAG,I2C_MASK_FLAG,I2C_ENEXT_FLAG);
         client->timing =  100;

         //printkElan("mtk-tpd:[elan]%x=IOCTL_I2C_INT\n",IOCTL_I2C_INT);
         //printkElan("mtk-tpd:[elan]%x=IOCTL_IAP_MODE_LOCK\n",IOCTL_IAP_MODE_LOCK);
         //printkElan("mtk-tpd:[elan]%x=IOCTL_IAP_MODE_UNLOCK\n",IOCTL_IAP_MODE_UNLOCK);

#if 1
         i2c_client = client;
         private_ts = &ts;
         private_ts->client = client;
	//printkElan("[elan] ELAN enter tpd_probe_ ,the i2c addr=0x%x", client->addr);
	//printkElan("GPIO43 =%d,GPIO_CTP_EINT_PIN =%d,GPIO_DIR_IN=%d,CUST_EINT_TOUCH_PANEL_NUM=%d",GPIO43,GPIO_CTP_EINT_PIN,GPIO_DIR_IN,CUST_EINT_TOUCH_PANEL_NUM);
#endif
/*
//power on, need confirm with SA
#ifdef TPD_POWER_SOURCE_CUSTOM
                   hwPowerOn(TPD_POWER_SOURCE_CUSTOM, VOL_2800, "TP");
#else
                   //hwPowerOn(MT6323_POWER_LDO_VGP2, VOL_2800, "TP");
                   hwPowerOn(MT65XX_POWER_LDO_VGP2, VOL_2800, "TP");
#endif

#ifdef TPD_POWER_SOURCE_1800
                   hwPowerOn(TPD_POWER_SOURCE_1800, VOL_1800, "TP");
#endif 
*/
         msleep(10);

         
/*
         //LDO enable
         mt_set_gpio_mode(GPIO_CTP_EN_PIN, GPIO_CTP_EN_PIN_M_GPIO);
         mt_set_gpio_dir(GPIO_CTP_EN_PIN, GPIO_DIR_OUT);
         mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ZERO);
         msleep(50);
         mt_set_gpio_out(GPIO_CTP_EN_PIN, GPIO_OUT_ONE);
*/
 //   MTK_TP_DEBUG("[elan] ELAN enter tpd_probe ,the i2c addr=0x%x\n", client->addr);
       
/*
// Reset Touch Pannel
         mt_set_gpio_mode( GPIO_CTP_RST_PIN, GPIO_CTP_RST_PIN_M_GPIO );
         mt_set_gpio_dir( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
        mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
         mdelay(50);
         mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
         mdelay(300);
// End Reset Touch Pannel       
*/
   // printkElan("ELAN enter tpd_probe_ ,the i2c addr=0x%x", client->addr);
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, 0);
	msleep(50);
printkElan("ektf2k reset \n");
    // for enable/reset pin
    mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
    mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
    mt_set_gpio_out(GPIO_CTP_RST_PIN, 1);
	msleep(50);

         fw_err = elan_ktf2k_ts_setup(client);
         if (fw_err < 0) {
             printkElan(KERN_INFO "[elan] No Elan chip inside\n");
         }

/*

#ifdef _DMA_MODE_    
    gpDMABuf_va = (u8 *)dma_alloc_coherent(NULL, 4096, &gpDMABuf_pa, GFP_KERNEL);
    if(!gpDMABuf_va){
        printkElan(KERN_INFO "[elan] Allocate DMA I2C Buffer failed\n");
    }
#endif
#ifdef _DMA_FW_UPGRADE_MODE_    
    gpDMAFWBuf_va = (u8 *)dma_alloc_coherent(NULL, 4096, &gpDMAFWBuf_pa, GFP_KERNEL);
    if(!gpDMAFWBuf_va){
        printkElan(KERN_INFO "[elan] Allocate DMA I2C Buffer failed\n");
    }
#endif
*/

#if 0 /*RESET RESOLUTION: tmp use ELAN_X_MAX & ELAN_Y_MAX*/ 
         printkElan("[elan] RESET RESOLUTION\n");
         input_set_abs_params(tpd->dev, ABS_X, 0,  ELAN_X_MAX, 0, 0);
         input_set_abs_params(tpd->dev, ABS_Y, 0,  ELAN_Y_MAX, 0, 0);
         input_set_abs_params(tpd->dev, ABS_MT_POSITION_X, 0, ELAN_X_MAX, 0, 0);
         input_set_abs_params(tpd->dev, ABS_MT_POSITION_Y, 0, ELAN_Y_MAX, 0, 0);
#endif 
/*
         #ifndef LCT_VIRTUAL_KEY
         set_bit( KEY_BACK,  tpd->dev->keybit );
         set_bit( KEY_HOMEPAGE,  tpd->dev->keybit );
         set_bit( KEY_MENU,  tpd->dev->keybit );
         #endif
  */       
// Setup Interrupt Pin
         mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
         mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
         mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
         mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);

	     msleep(100);
         mt65xx_eint_set_sens(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_SENSITIVE);
         mt65xx_eint_set_hw_debounce(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN);
	 	 mt65xx_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_EN, CUST_EINT_POLARITY_LOW, tpd_eint_interrupt_handler, 1);         
		 //mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, EINTF_TRIGGER_FALLING, tpd_eint_interrupt_handler, 1);
         ////mt_eint_registration(CUST_EINT_TOUCH_PANEL_NUM, EINTF_TRIGGER_LOW, tpd_eint_interrupt_handler, 1);                           
         //mdelay(200);
		 //msleep(1000);
		 mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
		 // End Setup Interrupt Pin        
         
         tpd_load_status = 1;
         
         thread = kthread_run(touch_event_handler, 0, TPD_DEVICE);
         if(IS_ERR(thread))
         {
             retval = PTR_ERR(thread);
             printkElan(TPD_DEVICE "mtk-tpd:[elan]  failed to create kernel thread: %ld\n", retval);
         }

         printkElan("mtk-tpd:[elan]  ELAN Touch Panel Device Probe %s\n", (retval < TPD_OK) ? "FAIL" : "PASS");
         
// Firmware Update
         // MISC
         ts.firmware.minor = MISC_DYNAMIC_MINOR;
         ts.firmware.name = "elan-iap";
         ts.firmware.fops = &elan_touch_fops;
         ts.firmware.mode = S_IRWXUGO; 
         
         if (misc_register(&ts.firmware) < 0)
                   printkElan("mtk-tpd:[elan] misc_register failed!!\n");
         else
           MTK_TP_DEBUG("[elan] misc_register finished!!\n"); 
// End Firmware Update 


//#ifdef TP_PROXIMITY_SENSOR_NEW
//   obj_ps.polling = 0;		   //0--interrupt mode;1--polling mode;
//   obj_ps.sensor_operate = ektf2k_ps_operate;

//   if ((err = hwmsen_attach(ID_PROXIMITY, &obj_ps)))
//   {
//	   TPD_DEBUG("hwmsen attach fail, return:%d.", err);
//   }

//#endif



#if IAP_PORTION
         if(0)
         {
                    mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
                                     update_thread = kthread_run(update_fw_handler, 0, TPD_DEVICE);
                                     if(IS_ERR(update_thread))
                                                                 {
                                                                                    retval = PTR_ERR(update_thread);
                                                                                    printkElan(TPD_DEVICE "failed to create kernel update thread: %ld\n", retval);
                                                                 }
         }
#endif

                   probe_flage = 1;
    mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);

	msleep(200);
	__fw_packet_handler(private_ts->client);
//---------add in probe----------
#ifdef ESD_CHECK	
	INIT_DELAYED_WORK(&esd_work, elan_touch_esd_func);
	esd_wq = create_singlethread_workqueue("esd_wq");	
	if (!esd_wq) {
		retval = -ENOMEM;

	}
	
	queue_delayed_work(esd_wq, &esd_work, delay);
	
#endif	
   
              
    return 0;
}

#ifdef TP_PROXIMITY_SENSOR_NEW
static int CTP_Face_Mode_State(void)
{
     return PROXIMITY;
}

static  int CTP_Face_Mode_Switch(int onoff_state)

{
    U8 bWriteData[4] =
    {
        0x54, 0xC1, 0x00, 0x01
    };
	
	if(onoff_state==1)
	{
		PROXIMITY =1;
		bWriteData[1] = 0xC1;
		PROXIMITY_STATE = 1;
		//PROXIMITY_SLEEP = 0;
	}
	else
	{	
		PROXIMITY =0;
		bWriteData[1] = 0xC0;
		PROXIMITY_STATE = -1;
		//PROXIMITY_SLEEP = 0;
	}

printkElan("CTP_Face_Mode_Switch  onoff_state %d, PROXIMITY %d\n",onoff_state, PROXIMITY);

	return i2c_master_send( i2c_client, &bWriteData[0], 4);	
}
static  int Get_Ctp_Face_Mode(void)
{
  printkElan("Get_Ctp_Face_Mode PROXIMITY_STATE %d\n",PROXIMITY_STATE);
  
	return PROXIMITY_STATE;
}

static void ektf2k_ctp_convert(int mode,bool force)
{
	if((mode==1)&&(TP_SLEEP==1))
		PROXIMITY_SLEEP = 1;
	else
		CTP_Face_Mode_Switch(mode);
}

 s32 ektf2k_ps_operate(void *self, u32 command, void *buff_in, s32 size_in,
                   void *buff_out, s32 size_out, s32 *actualout)
{
    s32 err = 0;
    s32 value;
    hwm_sensor_data *sensor_data;

    switch (command)
    {
        case SENSOR_DELAY:
        	printkElan("hdx ektf2k_ps_operate SENSOR_DELAY\n");
            if ((buff_in == NULL) || (size_in < sizeof(int)))
            {
                TPD_DEBUG("Set delay parameter error!");
                err = -EINVAL;
            }

            // Do nothing
            break;

        case SENSOR_ENABLE:
        	 printkElan("hdx ektf2k_ps_operate SENSOR_ENABLE");
            if ((buff_in == NULL) || (size_in < sizeof(int)))
            {
                TPD_DEBUG("Enable sensor parameter error!");
                err = -EINVAL;
            }
            else
            {
                value = *(int *)buff_in;
                CTP_Face_Mode_Switch(value);
            }

            break;

        case SENSOR_GET_DATA:
        	 printkElan("hdx ektf2k_ps_operate SENSOR_GET_DATA\n");
            if ((buff_out == NULL) || (size_out < sizeof(hwm_sensor_data)))
            {
                TPD_DEBUG("Get sensor data parameter error!");
                err = -EINVAL;
            }
            else
            {
                sensor_data = (hwm_sensor_data *)buff_out;
                sensor_data->values[0] = Get_Ctp_Face_Mode();
                sensor_data->value_divide = 1;
                sensor_data->status = SENSOR_STATUS_ACCURACY_MEDIUM;
            }

            break;

        default:
            TPD_DEBUG("proxmy sensor operate function no this parameter %d!\n", command);
            err = -1;
            break;
    }

    return err;
}

#endif
//-----workqueue-----
#ifdef ESD_CHECK
static void elan_touch_esd_func(struct work_struct *work)
{	
	int res;	
	uint8_t cmd[] = {0x53, 0x00, 0x00, 0x01};	
	struct i2c_client *client = private_ts->client;	
	//add by baojun.fu for i'm alive
	static int por_cnt = 0;	

	printkElan("[elan esd] %s: enter.......\n", __FUNCTION__); 		/* elan_dlx */
	
	if(have_interrupts == 1){
		printkElan("[elan esd] %s: had interrup not need check\n", __func__);
	}
	else{
		if ((++por_cnt) >= 2)
        {
            por_cnt = 0;
			printk("[elan esd] %s: i'm alive failed need reset!\n", __func__);
            //reset here
			mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
            mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
            mt_set_gpio_out(GPIO_CTP_RST_PIN, 0);
            msleep(10);

            // for enable/reset pin
            mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
            mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
            mt_set_gpio_out(GPIO_CTP_RST_PIN, 1);
            msleep(10);
        }
        else
        {
            res = i2c_master_send(client, cmd, sizeof(cmd));
            if (res != sizeof(cmd)){
                printk("[elan esd] %s: i2c_master_send failed reset now\n", __func__);
                //reset here
                mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
                mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
                mt_set_gpio_out(GPIO_CTP_RST_PIN, 0);
                msleep(10);

                // for enable/reset pin
                mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
                mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
                mt_set_gpio_out(GPIO_CTP_RST_PIN, 1);
                msleep(10);

            }
            else
            {
                printk("[elan esd] %s: i2c_master_send successful\n", __func__);
                
                msleep(20);
                
                if(have_interrupts == 1){
                    printk("[elan esd] %s: i2c_master_send successful, had response\n", __func__);
                }
                else
                {
                    printk("[elan esd] %s: i2c_master_send successful, no response need reset\n", __func__);
                    //reset here
                    mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
                    mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
                    mt_set_gpio_out(GPIO_CTP_RST_PIN, 0);
                    msleep(10);

                    // for enable/reset pin
                    mt_set_gpio_mode(GPIO_CTP_RST_PIN, 0);
                    mt_set_gpio_dir(GPIO_CTP_RST_PIN, 1);
                    mt_set_gpio_out(GPIO_CTP_RST_PIN, 1);
                    msleep(10);
                }
            }
        }
	}
	
	have_interrupts = 0;	
	queue_delayed_work(esd_wq, &esd_work, delay);
	printkElan("[elan esd] %s: enter.......\n", __FUNCTION__); 		/* elan_dlx */
}
#endif	

static int tpd_remove(struct i2c_client *client)
{
    printkElan("mtk-tpd:[elan] TPD removed\n");
    
   #ifdef _DMA_MODE_    
   if(gpDMABuf_va){
        dma_free_coherent(NULL, 4096, gpDMABuf_va, gpDMABuf_pa);
        gpDMABuf_va = NULL;
        gpDMABuf_pa = NULL;
   }
   #endif    

   return 0;
}


static int tpd_suspend(struct i2c_client *client, pm_message_t message)
{
    int retval = TPD_OK;
    static char data = 0x3;
    uint8_t cmd[] = {CMD_W_PKT, 0x50, 0x00, 0x01};
    
    printkElan("mtk-tpd:[elan] TP enter into sleep mode\n");

#if defined(TP_PROXIMITY_SENSOR_NEW)
	 if( PROXIMITY == 1 )//PROXIMITY_STATE
	 {
	 //PROXIMITY_SLEEP = 1;
		 return ;
	 }
	 //PROXIMITY_SLEEP = 0;
 #endif //TP_PROXIMITY_SENSOR_NEW
//---------add in suspend----------
#ifdef ESD_CHECK	
	cancel_delayed_work_sync(&esd_work);
#endif	
    if ((i2c_master_send(private_ts->client, cmd, sizeof(cmd))) != sizeof(cmd)) 
    {
         printkElan("mtk-tpd:[elan] %s: i2c_master_send failed\n", __func__);
         return -retval;
    }
    mt65xx_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
	TP_SLEEP = 1;

    return retval;
}


static int tpd_resume(struct i2c_client *client)
{
    int retval = TPD_OK;
    uint8_t cmd[] = {CMD_W_PKT, 0x58, 0x00, 0x01};
    printkElan("mtk-tpd:[elan]tpd_resume TPD wake up\n");

#if defined(TP_PROXIMITY_SENSOR_NEW)
 if( PROXIMITY == 1 )
 {
   return ;
 } 
#endif //TP_PROXIMITY_SENSOR_NEW
//---------add in resume----------
#ifdef ESD_CHECK
	queue_delayed_work(esd_wq, &esd_work, delay);	
#endif

#if 1 
    mt_set_gpio_out(GPIO_CTP_RST_PIN, GPIO_OUT_ZERO);
    mdelay(10);
    mt_set_gpio_out( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
//    mdelay(10);
#else 
    if ((i2c_master_send(private_ts->client, cmd, sizeof(cmd))) != sizeof(cmd)) 
    {
        printkElan("[elan] %s: i2c_master_send failed\n", __func__);
        return -retval;
    }
    msleep(200);
#endif

    mt65xx_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	TP_SLEEP = 0;
	if(PROXIMITY_SLEEP)
	{
		mdelay(100);
		CTP_Face_Mode_Switch(1);
		PROXIMITY_SLEEP = 0;
	}
    return retval;
}

static int tpd_local_init(void)
{
// james    
 	bootMode = get_boot_mode();
	if(SW_REBOOT == bootMode)
		bootMode = NORMAL_BOOT;

    printkElan("[mtk-tpd]: ektf I2C Touchscreen Driver init\n");
    if(i2c_add_driver(&tpd_i2c_driver) != 0)
    {
        printkElan("[mtk-tpd]: unable to add i2c driver.\n");
        return -1;
    }

    if(tpd_load_status == 0) 
    {
         printkElan("ektf2k add error touch panel driver.\n");
         i2c_del_driver(&tpd_i2c_driver);
         return -1;
    }

#ifdef TPD_HAVE_BUTTON
    tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);// initialize tpd button data
#endif

#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT * 4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT * 4);
#endif
#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
         memcpy(tpd_calmat, tpd_def_calmat_local, 8*4);
         memcpy(tpd_def_calmat, tpd_def_calmat_local, 8*4);         
#endif 

    printkElan("mtk-tpd:end %s, %d\n", __FUNCTION__, __LINE__);
    tpd_type_cap = 1;
    return 0;
}
/*
static int tpd_local_init(void)
{

// james    
 	bootMode = get_boot_mode();
	if(SW_REBOOT == bootMode)
		bootMode = NORMAL_BOOT;
// end james
    printkElan("[elan]: I2C Touchscreen Driver init\n");

    if(i2c_add_driver(&tpd_i2c_driver) != 0)
    {
        printkElan("[elan]: unable to add i2c driver.\n");
        return -1;
    }

#ifdef TPD_HAVE_BUTTON
    tpd_button_setting(TPD_KEY_COUNT, tpd_keys_local, tpd_keys_dim_local);// initialize tpd button data
#endif
#if (defined(TPD_WARP_START) && defined(TPD_WARP_END))
    TPD_DO_WARP = 1;
    memcpy(tpd_wb_start, tpd_wb_start_local, TPD_WARP_CNT * 4);
    memcpy(tpd_wb_end, tpd_wb_start_local, TPD_WARP_CNT * 4);
#endif
#if 0
#if (defined(TPD_HAVE_CALIBRATION) && !defined(TPD_CUSTOM_CALIBRATION))
    memcpy(tpd_calmat, tpd_def_calmat_local, 8 * 4);
    memcpy(tpd_def_calmat, tpd_def_calmat_local, 8 * 4);
#endif
#endif
    static BOOTMODE bootMode = NORMAL_BOOT;  //  add for TP button,by tyd-lg("end %s, %d\n", __FUNCTION__, __LINE__);
    tpd_type_cap = 1;
    return 0;
}
*/

static struct tpd_driver_t tpd_device_driver =
{
    .tpd_device_name = "ekft2k",
    //.tpd_device_name = "ektf2k_mtk",       
    .tpd_local_init = tpd_local_init,
    .suspend = tpd_suspend,
    .resume = tpd_resume,
#ifdef TPD_HAVE_BUTTON
    .tpd_have_button = 1,
#else
    .tpd_have_button = 0,
#endif
#if defined(TP_PROXIMITY_SENSOR_NEW)
	.tpd_get_ps  =  Get_Ctp_Face_Mode,
	.tpd_convert =  ektf2k_ctp_convert
	#endif
};

static int __init tpd_driver_init(void)
{
         printkElan("mtk-tpd ekft2k touch panel driver init\n");

         i2c_register_board_info(I2C_NUM, &ekft2k_i2c_tpd, 1);

         if(tpd_driver_add(&tpd_device_driver) < 0)
         {
             printkElan("[mtk-tpd]: %s driver failed\n", __func__);
         }
         return 0;
}
//static struct i2c_board_info __initdata ekft2k_i2c_tpd={ I2C_BOARD_INFO("ekft2k", (0x15))};
/* called when loaded into kernel */
/*
static int __init tpd_driver_init(void)
{
    printkElan("[elan]: Driver Verison mtk0003 for MTK65xx serial\n");
    printkElan("MediaTek ekft2k touch panel driver init\n");
	  //i2c_register_board_info(0, &ekft2k_i2c_tpd, 1) 
    if(tpd_driver_add(&tpd_device_driver) < 0)
    {
        printkElan("[elan]: %s driver failed\n", __func__);
    }

#if 0//defined (__CTP_ESD__)  
    init_waitqueue_head(&esd_wait_queue);
#endif 

    return 0;
}
*/

static void __exit tpd_driver_exit(void)
{
    printkElan("[mtk-tpd]: %s elan ektf touch panel driver exit\n", __func__);
    tpd_driver_remove(&tpd_device_driver);
}

module_init(tpd_driver_init);
module_exit(tpd_driver_exit);
