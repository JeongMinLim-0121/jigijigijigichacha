#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/timer.h>
#include "../common/include/forklift.h"

#define DEV_NAME "power"
static dev_t devno; static struct cdev cdev_;
static struct class *cls; static struct device *devnode;

static struct fk_power_stat stat = { .mv = 12000, .ma = 500, .soc = 90 };
static struct timer_list tick;

static void tick_fn(struct timer_list *ti){
    /* 간단 소모 시뮬레이션 */
    if (stat.soc > 0) stat.soc--;
    if (stat.mv > 9000) stat.mv -= 10;
    mod_timer(&tick, jiffies + HZ*1);
}

static long pwr_ioctl(struct file *f, unsigned int cmd, unsigned long arg){
    if (cmd == IOCTL_PWR_GET_STAT){
        if (copy_to_user((void __user*)arg, &stat, sizeof(stat))) return -EFAULT;
        return 0;
    }
    return -ENOTTY;
}

static ssize_t pwr_read(struct file *f, char __user *b, size_t l, loff_t *o){
    if (l < sizeof(stat)) return -EINVAL;
    if (copy_to_user(b,&stat,sizeof(stat))) return -EFAULT;
    return sizeof(stat);
}

static const struct file_operations fops={
    .owner=THIS_MODULE,.unlocked_ioctl=pwr_ioctl,.read=pwr_read,
};

static int __init pwr_init(void){
    int ret;
    if((ret=alloc_chrdev_region(&devno,0,1,DEV_NAME))) return ret;
    cdev_init(&cdev_,&fops); if((ret=cdev_add(&cdev_,devno,1))) return ret;
    cls=class_create(THIS_MODULE,DEV_NAME);
    if (IS_ERR(cls)) { cdev_del(&cdev_); unregister_chrdev_region(devno,1); return PTR_ERR(cls); }
    devnode=device_create(cls,NULL,devno,NULL,DEV_NAME "%d",0);
    if (IS_ERR(devnode)) { class_destroy(cls); cdev_del(&cdev_); unregister_chrdev_region(devno,1); return PTR_ERR(devnode); }
    timer_setup(&tick, tick_fn, 0); mod_timer(&tick, jiffies + HZ*1);
    pr_info("[power] ready: /dev/%s0\n", DEV_NAME);
    return 0;
}
static void __exit pwr_exit(void){
    del_timer_sync(&tick);
    device_destroy(cls,devno); class_destroy(cls);
    cdev_del(&cdev_); unregister_chrdev_region(devno,1);
}
module_init(pwr_init); module_exit(pwr_exit);
MODULE_LICENSE("GPL");
