// ledkey_drv.c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/version.h>
#include "../common/include/forklift.h"

#define DEV_NAME "ledkey"

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

static u8 led_state;  /* bit0~7 LED */
static u8 key_state;  /* bit0~7 KEY (사용자가 write로 흉내낼 수도 있음) */

static ssize_t lk_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    u8 v = key_state;
    if (len < sizeof(v)) return -EINVAL;
    if (copy_to_user(buf, &v, sizeof(v))) return -EFAULT;
    return sizeof(v);
}

static ssize_t lk_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    u8 v;
    if (len < sizeof(v)) return -EINVAL;
    if (copy_from_user(&v, buf, sizeof(v))) return -EFAULT;
    led_state = v;
    return sizeof(v);
}

static long lk_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
#ifdef IOCTL_LED_SET
    if (cmd == IOCTL_LED_SET) {
        if (copy_from_user(&led_state, (void __user*)arg, sizeof(led_state))) return -EFAULT;
        return 0;
    }
#endif
#ifdef IOCTL_LED_GET
    if (cmd == IOCTL_LED_GET) {
        if (copy_to_user((void __user*)arg, &led_state, sizeof(led_state))) return -EFAULT;
        return 0;
    }
#endif
#ifdef IOCTL_KEY_GET
    if (cmd == IOCTL_KEY_GET) {
        if (copy_to_user((void __user*)arg, &key_state, sizeof(key_state))) return -EFAULT;
        return 0;
    }
#endif
    return -ENOTTY;
}

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .read           = lk_read,
    .write          = lk_write,
    .unlocked_ioctl = lk_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = lk_ioctl,
#endif
};

static int __init lk_init(void)
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

    pr_info("[ledkey] ready: /dev/%s0\n", DEV_NAME);
    return 0;

err_class:
    class_destroy(cls);
err_cdev:
    cdev_del(&cdev_);
err_chr:
    unregister_chrdev_region(devno, 1);
    return ret;
}

static void __exit lk_exit(void)
{
    device_destroy(cls, devno);
    class_destroy(cls);
    cdev_del(&cdev_);
    unregister_chrdev_region(devno, 1);
    pr_info("[ledkey] removed\n");
}

module_init(lk_init);
module_exit(lk_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("ijm");