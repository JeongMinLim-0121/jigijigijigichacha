#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/timer.h>
#include "../common/include/forklift.h"

#define DEV_NAME "motor"
static dev_t devno; static struct cdev cdev_;
static struct class *cls; static struct device *devnode;

static int speed; /* 0~100 */
static int dir;   /* 0 stop, 1 fwd, 2 rev */
static struct timer_list tick;

static void tick_fn(struct timer_list *t) {
    /* 가상 구동: 단순히 주기 유지 (여기선 아무것도 안함) */
    if (dir != 0) mod_timer(&tick, jiffies + HZ/10);
}

static long motor_ioctl(struct file *f, unsigned int cmd, unsigned long arg){
    int v; int st[2];
    switch(cmd){
        case IOCTL_MOTOR_SET_SPEED:
            if (copy_from_user(&v,(void __user*)arg,sizeof(int))) return -EFAULT;
            if (v<0) v=0; if (v>100) v=100; speed=v; return 0;
        case IOCTL_MOTOR_SET_DIR:
            if (copy_from_user(&v,(void __user*)arg,sizeof(int))) return -EFAULT;
            if (v<0||v>2) return -EINVAL; dir=v;
            if (dir) mod_timer(&tick, jiffies + HZ/10); else del_timer_sync(&tick);
            return 0;
        case IOCTL_MOTOR_GET_STATE:
            st[0]=dir; st[1]=speed;
            if (copy_to_user((void __user*)arg, st, sizeof(st))) return -EFAULT;
            return 0;
    }
    return -ENOTTY;
}

static ssize_t motor_read(struct file *f, char __user *buf, size_t len, loff_t *o){
    int st[2]={dir,speed};
    if (len < sizeof(st)) return -EINVAL;
    if (copy_to_user(buf, st, sizeof(st))) return -EFAULT;
    return sizeof(st);
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = motor_ioctl,
    .read = motor_read,
};

static int __init motor_init(void){
    int ret;
    if ((ret=alloc_chrdev_region(&devno,0,1,DEV_NAME))) return ret;
    cdev_init(&cdev_, &fops); if((ret=cdev_add(&cdev_,devno,1))) return ret;
    cls = class_create(THIS_MODULE, DEV_NAME);
    devnode = device_create(cls, NULL, devno, NULL, DEV_NAME "%d", 0);
    timer_setup(&tick, tick_fn, 0);
    pr_info("[motor] ready: /dev/%s0\n", DEV_NAME);
    return 0;
}
static void __exit motor_exit(void){
    del_timer_sync(&tick);
    device_destroy(cls, devno); class_destroy(cls);
    cdev_del(&cdev_); unregister_chrdev_region(devno,1);
}
module_init(motor_init); module_exit(motor_exit);
MODULE_LICENSE("GPL");
