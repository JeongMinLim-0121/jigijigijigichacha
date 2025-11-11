#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/timer.h>
#include "../common/include/forklift.h"

#define DEV_NAME "encoder"
static dev_t devno; static struct cdev cdev_;
static struct class *cls; static struct device *devnode;

static unsigned int enc_speed; /* 가상 속도 단위 */
static unsigned int ticks;
static struct timer_list tick;

static void tick_fn(struct timer_list *t){
    ticks += enc_speed; /* 속도 비례 증가 (단위 임의) */
    mod_timer(&tick, jiffies + HZ/10);
}

static long enc_ioctl(struct file *f, unsigned int cmd, unsigned long arg){
    struct fk_enc_stat st;
    switch(cmd){
        case IOCTL_ENC_SET_SPEED:
            if (copy_from_user(&enc_speed,(void __user*)arg,sizeof(enc_speed))) return -EFAULT;
            if (!timer_pending(&tick)) mod_timer(&tick, jiffies + HZ/10);
            return 0;
        case IOCTL_ENC_GET_STAT:
            st.ticks=ticks; st.speed=enc_speed;
            if (copy_to_user((void __user*)arg,&st,sizeof(st))) return -EFAULT;
            return 0;
    }
    return -ENOTTY;
}

static const struct file_operations fops = {
    .owner=THIS_MODULE, .unlocked_ioctl=enc_ioctl,
};

static int __init enc_init(void){
    int ret;
    if ((ret=alloc_chrdev_region(&devno,0,1,DEV_NAME))) return ret;
    cdev_init(&cdev_, &fops); if((ret=cdev_add(&cdev_,devno,1))) return ret;
    cls=class_create(THIS_MODULE, DEV_NAME);
    devnode=device_create(cls,NULL,devno,NULL,DEV_NAME "%d",0);
    timer_setup(&tick, tick_fn, 0);
    pr_info("[encoder] ready: /dev/%s0\n", DEV_NAME);
    return 0;
}
static void __exit enc_exit(void){
    del_timer_sync(&tick);
    device_destroy(cls,devno); class_destroy(cls);
    cdev_del(&cdev_); unregister_chrdev_region(devno,1);
}
module_init(enc_init); module_exit(enc_exit);
MODULE_LICENSE("GPL");
