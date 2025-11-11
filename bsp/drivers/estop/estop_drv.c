#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include "../common/include/forklift.h"

#define DEV_NAME "estop"
static dev_t devno; static struct cdev cdev_;
static struct class *cls; static struct device *devnode;
static int estop; /* 0:해제 1:비상정지 */

static long estop_ioctl(struct file *f, unsigned int cmd, unsigned long arg){
    switch(cmd){
        case IOCTL_ESTOP_TRIP:  estop=1; return 0;
        case IOCTL_ESTOP_CLEAR: estop=0; return 0;
        case IOCTL_ESTOP_GET:
            if(copy_to_user((void __user*)arg,&estop,sizeof(estop))) return -EFAULT;
            return 0;
    }
    return -ENOTTY;
}
static ssize_t estop_read(struct file *f, char __user *b, size_t l, loff_t *o){
    if (l<sizeof(int)) return -EINVAL;
    if (copy_to_user(b,&estop,sizeof(int))) return -EFAULT;
    return sizeof(int);
}
static const struct file_operations fops={
    .owner=THIS_MODULE,.unlocked_ioctl=estop_ioctl,.read=estop_read,
};
static int __init estop_init(void){
    int ret;
    if((ret=alloc_chrdev_region(&devno,0,1,DEV_NAME))) return ret;
    cdev_init(&cdev_,&fops); if((ret=cdev_add(&cdev_,devno,1))) return ret;
    cls=class_create(THIS_MODULE,DEV_NAME);
    devnode=device_create(cls,NULL,devno,NULL,DEV_NAME "%d",0);
    pr_info("[estop] ready: /dev/%s0\n", DEV_NAME);
    return 0;
}
static void __exit estop_exit(void){
    device_destroy(cls,devno); class_destroy(cls);
    cdev_del(&cdev_); unregister_chrdev_region(devno,1);
}
module_init(estop_init); module_exit(estop_exit);
MODULE_LICENSE("GPL");
