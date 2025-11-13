// motor_drv.c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/version.h>
#include "../common/include/forklift.h"

#define DEV_NAME "motor"

#ifndef CLASS_CREATE
# if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
#  define CLASS_CREATE(name) class_create(name)
# else
#  define CLASS_CREATE(name) class_create(THIS_MODULE, name)
# endif
#endif

static dev_t devno;
static struct cdev cdev_;
static struct class *cls;
static struct device *devnode;

static int speed; /* -1000 ~ +1000 가정 */

static ssize_t motor_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    if (len < sizeof(speed)) return -EINVAL;
    if (copy_to_user(buf, &speed, sizeof(speed))) return -EFAULT;
    return sizeof(speed);
}

static ssize_t motor_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    int v;
    if (len < sizeof(v)) return -EINVAL;
    if (copy_from_user(&v, buf, sizeof(v))) return -EFAULT;
    if (v > 1000) v = 1000;
    if (v < -1000) v = -1000;
    speed = v;
    return sizeof(v);
}

static long motor_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
#ifdef IOCTL_MOTOR_SET_SPEED
    if (cmd == IOCTL_MOTOR_SET_SPEED) {
        int v;
        if (copy_from_user(&v, (void __user*)arg, sizeof(v))) return -EFAULT;
        if (v > 1000) v = 1000;
        if (v < -1000) v = -1000;
        speed = v;
        return 0;
    }
#endif
#ifdef IOCTL_MOTOR_GET_SPEED
    if (cmd == IOCTL_MOTOR_GET_SPEED) {
        if (copy_to_user((void __user*)arg, &speed, sizeof(speed))) return -EFAULT;
        return 0;
    }
#endif
    return -ENOTTY;
}

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .read           = motor_read,
    .write          = motor_write,
    .unlocked_ioctl = motor_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = motor_ioctl,
#endif
};

static int __init motor_init(void)
{
    int ret = alloc_chrdev_region(&devno, 0, 1, DEV_NAME);
    if (ret) return ret;

    cdev_init(&cdev_, &fops);
    ret = cdev_add(&cdev_, devno, 1);
    if (ret) goto err_chr;

    cls = CLASS_CREATE(DEV_NAME);
    if (IS_ERR(cls)) { ret = PTR_ERR(cls); goto err_cdev; }

    devnode = device_create(cls, NULL, devno, NULL, DEV_NAME "%d", 0);
    if (IS_ERR(devnode)) { ret = PTR_ERR(devnode); goto err_class; }

    pr_info("[motor] ready: /dev/%s0\n", DEV_NAME);
    return 0;

err_class:
    class_destroy(cls);
err_cdev:
    cdev_del(&cdev_);
err_chr:
    unregister_chrdev_region(devno, 1);
    return ret;
}

static void __exit motor_exit(void)
{
    device_destroy(cls, devno);
    class_destroy(cls);
    cdev_del(&cdev_);
    unregister_chrdev_region(devno, 1);
    pr_info("[motor] removed\n");
}

module_init(motor_init);
module_exit(motor_exit);
MODULE_LICENSE("GPL");
