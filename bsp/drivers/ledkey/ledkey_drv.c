#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include "../common/include/forklift.h"

#define DEV_NAME "ledkey"
static dev_t devno; static struct cdev cdev_;
static struct class *cls; static struct device *devnode;

static unsigned char leds; /* 출력상태 */
static unsigned char keys; /* 입력상태(가상) */

static long lk_ioctl(struct file *f, unsigned int cmd, unsigned long arg){
    switch(cmd){
        case IOCTL_LED_SET:
            if(copy_from_user(&leds,(void __user*)arg,sizeof(leds))) return -EFAULT;
            return 0;
        case IOCTL_LED_GET:
            if(copy_to_user((void __user*)arg,&leds,sizeof(leds))) return -EFAULT;
            return 0;
        case IOCTL_KEY_GET:
            if(copy_to_user((void __user*)arg,&keys,sizeof(keys))) return -EFAULT;
            return 0;
    }
    return -ENOTTY;
}

/* 간단히 write로 키상태 주입 가능하게 */
static ssize_t lk_write(struct file *f, const char __user *buf, size_t len, loff_t *o){
    if (len < 1) return -EINVAL;
    if (copy_from_user(&keys, buf, 1)) return -EFAULT;
    return 1;
}

static const struct file_operations fops={
    .owner=THIS_MODULE,.unlocked_ioctl=lk_ioctl,.write=lk_write,
};

static int __init lk_init(void){
    int ret;
    if((ret=alloc_chrdev_region(&devno,0,1,DEV_NAME))) return ret;
    cdev_init(&cdev_,&fops); if((ret=cdev_add(&cdev_,devno,1))) return ret;
    cls=class_create(THIS_MODULE,DEV_NAME);
    devnode=device_create(cls,NULL,devno,NULL,DEV_NAME "%d",0);
    pr_info("[ledkey] ready: /dev/%s0\n", DEV_NAME);
    return 0;
}
static void __exit lk_exit(void){
    device_destroy(cls,devno); class_destroy(cls);
    cdev_del(&cdev_); unregister_chrdev_region(devno,1);
}
module_init(lk_init); module_exit(lk_exit);
MODULE_LICENSE("GPL");
