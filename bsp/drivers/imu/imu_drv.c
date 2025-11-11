// imu_drv.c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/version.h>
#include "../common/include/forklift.h"

#define DEV_NAME "imu"

#ifndef CLASS_CREATE
# if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
#  define CLASS_CREATE(name) class_create(name)
# else
#  define CLASS_CREATE(name) class_create(THIS_MODULE, name)
# endif
#endif

struct imu_frame {
    int ax, ay, az;   /* mg */
    int gx, gy, gz;   /* mdps */
};

static dev_t devno;
static struct cdev cdev_;
static struct class *cls;
static struct device *devnode;

static struct imu_frame last;
static struct timer_list sim_t;

static void sim_tick(struct timer_list *t)
{
    /* 간단한 시뮬레이션: 값이 시간에 따라 살짝 변함 */
    last.ax = (last.ax + 7) % 2000 - 1000;
    last.ay = (last.ay + 5) % 2000 - 1000;
    last.az = 1000; /* 중력가속도 근사 */
    last.gx = (last.gx + 13) % 500 - 250;
    last.gy = (last.gy + 11) % 500 - 250;
    last.gz = (last.gz + 17) % 500 - 250;
    mod_timer(&sim_t, jiffies + HZ/20); /* 50Hz */
}

static ssize_t imu_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    if (len < sizeof(last)) return -EINVAL;
    if (copy_to_user(buf, &last, sizeof(last))) return -EFAULT;
    return sizeof(last);
}

static ssize_t imu_write(struct file *f, const char __user *buf, size_t len, loff_t *off)
{
    if (len < sizeof(last)) return -EINVAL;
    if (copy_from_user(&last, buf, sizeof(last))) return -EFAULT;
    return sizeof(last);
}

static long imu_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
#ifdef IOCTL_IMU_GET
    if (cmd == IOCTL_IMU_GET) {
        if (copy_to_user((void __user*)arg, &last, sizeof(last))) return -EFAULT;
        return 0;
    }
#endif
#ifdef IOCTL_IMU_SET
    if (cmd == IOCTL_IMU_SET) {
        if (copy_from_user(&last, (void __user*)arg, sizeof(last))) return -EFAULT;
        return 0;
    }
#endif
    return -ENOTTY;
}

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .read           = imu_read,
    .write          = imu_write,
    .unlocked_ioctl = imu_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = imu_ioctl,
#endif
};

static int __init imu_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&devno, 0, 1, DEV_NAME);
    if (ret) return ret;

    cdev_init(&cdev_, &fops);
    ret = cdev_add(&cdev_, devno, 1);
    if (ret) goto err_chr;

    cls = CLASS_CREATE(DEV_NAME);
    if (IS_ERR(cls)) { ret = PTR_ERR(cls); goto err_cdev; }

    devnode = device_create(cls, NULL, devno, NULL, DEV_NAME "%d", 0);
    if (IS_ERR(devnode)) { ret = PTR_ERR(devnode); goto err_class; }

    timer_setup(&sim_t, sim_tick, 0);
    mod_timer(&sim_t, jiffies + HZ/20);

    pr_info("[imu] ready: /dev/%s0\n", DEV_NAME);
    return 0;

err_class:
    class_destroy(cls);
err_cdev:
    cdev_del(&cdev_);
err_chr:
    unregister_chrdev_region(devno, 1);
    return ret;
}

static void __exit imu_exit(void)
{
    del_timer_sync(&sim_t);
    device_destroy(cls, devno);
    class_destroy(cls);
    cdev_del(&cdev_);
    unregister_chrdev_region(devno, 1);
    pr_info("[imu] removed\n");
}

module_init(imu_init);
module_exit(imu_exit);
MODULE_LICENSE("GPL");
