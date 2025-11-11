#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/timer.h>
#include "../common/include/forklift.h"

#define DEV_NAME "imu"
static dev_t devno; static struct cdev cdev_;
static struct class *cls; static struct device *devnode;
static struct fk_imu_frame frame;
static struct timer_list tick;
static int t;

static void tick_fn(struct timer_list *ti){
    /* 가상 웨이브폼 생성 */
    t++;
    frame.ax = (t*10)%1000; frame.ay = (t*7)%1000; frame.az = 1000 - ((t*3)%1000);
    frame.gx = (t*5)%2000; frame.gy = (t*8)%2000; frame.gz = 2000 - ((t*11)%2000);
    mod_timer(&tick, jiffies + HZ/20);
}

static long imu_ioctl(struct file *f, unsigned int cmd, unsigned long arg){
    if (cmd == IOCTL_IMU_GET_FRAME){
        if(copy_to_user((void __user*)arg,&frame,sizeof(frame))) return -EFAULT;
        return 0;
    }
    return -ENOTTY;
}

static ssize_t imu_read(struct file *f, char __user *b, size_t l, loff_t *o){
    if (l < sizeof(frame)) return -EINVAL;
    if (copy_to_user(b,&frame,sizeof(frame))) return -EFAULT;
    return sizeof(frame);
}

static const struct file_operations fops={
    .owner=THIS_MODULE,.unlocked_ioctl=imu_ioctl,.read=imu_read,
};

static int __init imu_init(void){
    int ret;
    if((ret=alloc_chrdev_region(&devno,0,1,DEV_NAME))) return ret;
    cdev_init(&cdev_,&fops); if((ret=cdev_add(&cdev_,devno,1))) return ret;
    cls=class_create(THIS_MODULE,DEV_NAME);
    if (IS_ERR(cls)) { cdev_del(&cdev_); unregister_chrdev_region(devno,1); return PTR_ERR(cls); }
    devnode=device_create(cls,NULL,devno,NULL,DEV_NAME "%d",0);
    if (IS_ERR(devnode)) { class_destroy(cls); cdev_del(&cdev_); unregister_chrdev_region(devno,1); return PTR_ERR(devnode); }
    timer_setup(&tick, tick_fn, 0); mod_timer(&tick, jiffies + HZ/20);
    pr_info("[imu] ready: /dev/%s0\n", DEV_NAME);
    return 0;
}
static void __exit imu_exit(void){
    del_timer_sync(&tick);
    device_destroy(cls,devno); class_destroy(cls);
    cdev_del(&cdev_); unregister_chrdev_region(devno,1);
}
module_init(imu_init); module_exit(imu_exit);
MODULE_LICENSE("GPL");
