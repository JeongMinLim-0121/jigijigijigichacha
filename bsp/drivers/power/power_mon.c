// power_mon.c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/version.h>
#include "../common/include/forklift.h"

#define DEV_NAME "power"

#ifndef CLASS_CREATE
# if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
#  define CLASS_CREATE(name) class_create(name)
# else
#  define CLASS_CREATE(name) class_create(THIS_MODULE, name)
# endif
#endif

struct power_stat {
    u16 mv;  /* millivolt */
    u16 ma;  /* milliamp */
    u16 pct; /* battery % */
};

static dev_t devno;
static struct cdev cdev_;
static struct class *cls;
static struct device *devnode;

static struct power_stat ps;
static struct timer_list sim_t;

static void sim_tick(struct timer_list *t)
{
    /* 간단한 배터리 소모 시뮬레이터 */
    if (ps.pct > 0) ps.pct--;
    if (ps.mv > 3300) ps.mv -= 2;
    if (ps.ma > 100)  ps.ma -= 1;
    mod_timer(&sim_t, jiffies + HZ); /* 1Hz */
}

static ssize_t pwr_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    if (len < sizeof(ps)) return -EINVAL;
    if (copy_to_user(buf, &ps, sizeof(ps))) return -EFAULT;
    return sizeof(ps);
}

static long pwr_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
#ifdef IOCTL_POWER_GET
    if (cmd == IOCTL_POWER_GET) {
        if (copy_to_user((void __user*)arg, &ps, sizeof(ps))) return -EFAULT;
        return 0;
    }
#endif
#ifdef IOCTL_POWER_SET
    if (cmd == IOCTL_POWER_SET) {
        if (copy_from_user(&ps, (void __user*)arg, sizeof(ps))) return -EFAULT;
        return 0;
    }
#endif
    return -ENOTTY;
}

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .read           = pwr_read,
    .unlocked_ioctl = pwr_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = pwr_ioctl,
#endif
};

static int __init pwr_init(void)
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

    /* 초기값 */
    ps.mv = 12400;  /* 12.4V 가정 */
    ps.ma = 1500;   /* 1.5A */
    ps.pct = 100;

    timer_setup(&sim_t, sim_tick, 0);
    mod_timer(&sim_t, jiffies + HZ);

    pr_info("[power] ready: /dev/%s0\n", DEV_NAME);
    return 0;

err_class:
    class_destroy(cls);
err_cdev:
    cdev_del(&cdev_);
err_chr:
    unregister_chrdev_region(devno, 1);
    return ret;
}

static void __exit pwr_exit(void)
{
    del_timer_sync(&sim_t);
    device_destroy(cls, devno);
    class_destroy(cls);
    cdev_del(&cdev_);
    unregister_chrdev_region(devno, 1);
    pr_info("[power] removed\n");
}

module_init(pwr_init);
module_exit(pwr_exit);
MODULE_LICENSE("GPL");
   