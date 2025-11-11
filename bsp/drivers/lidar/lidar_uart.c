#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/timer.h>
#include "../common/include/forklift.h"

#define DEV_NAME "lidar"
static dev_t devno; static struct cdev cdev_;
static struct class *cls; static struct device *devnode;

static __u32 rate_hz = 10;
static __u16 dist_mm = 1000;
static struct timer_list tick;
static int t;

static void tick_fn(struct timer_list *ti){
    /* 800~1200mm 왕복 */
    t++;
    dist_mm = 1000 + (100 * ((t/3)%2 ? 1 : -1));
    mod_timer(&tick, jiffies + HZ/(rate_hz?rate_hz:1));
}

static long lidar_ioctl(struct file *f, unsigned int cmd, unsigned long arg){
    switch(cmd){
        case IOCTL_LIDAR_SET_RATE:
            if (copy_from_user(&rate_hz,(void __user*)arg,sizeof(rate_hz))) return -EFAULT;
            if (rate_hz == 0) rate_hz = 1;
            mod_timer(&tick, jiffies + HZ/rate_hz);
            return 0;
        case IOCTL_LIDAR_GET_DIST:
            if (copy_to_user((void __user*)arg,&dist_mm,sizeof(dist_mm))) return -EFAULT;
            return 0;
    }
    return -ENOTTY;
}

static ssize_t lidar_read(struct file *f, char __user *b, size_t l, loff_t *o){
    if (l < sizeof(dist_mm)) return -EINVAL;
    if (copy_to_user(b,&dist_mm,sizeof(dist_mm))) return -EFAULT;
    return sizeof(dist_mm);
}

static const struct file_operations fops={
    .owner=THIS_MODULE,.unlocked_ioctl=lidar_ioctl,.read=lidar_read,
};

static int __init lidar_init(void){
    int ret;
    if((ret=alloc_chrdev_region(&devno,0,1,DEV_NAME))) return ret;
    cdev_init(&cdev_,&fops); if((ret=cdev_add(&cdev_,devno,1))) return ret;
    cls=class_create(THIS_MODULE,DEV_NAME);
    if (IS_ERR(cls)) { cdev_del(&cdev_); unregister_chrdev_region(devno,1); return PTR_ERR(cls); }
    devnode=device_create(cls,NULL,devno,NULL,DEV_NAME "%d",0);
    if (IS_ERR(devnode)) { class_destroy(cls); cdev_del(&cdev_); unregister_chrdev_region(devno,1); return PTR_ERR(devnode); }
    timer_setup(&tick, tick_fn, 0); mod_timer(&tick, jiffies + HZ/rate_hz);
    pr_info("[lidar] ready: /dev/%s0\n", DEV_NAME);
    return 0;
}
static void __exit lidar_exit(void){
    del_timer_sync(&tick);
    device_destroy(cls,devno); class_destroy(cls);
    cdev_del(&cdev_); unregister_chrdev_region(devno,1);
}
module_init(lidar_init); module_exit(lidar_exit);
MODULE_LICENSE("GPL");
