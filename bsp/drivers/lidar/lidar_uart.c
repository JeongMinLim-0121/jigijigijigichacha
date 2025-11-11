// lidar_uart.c  (간단한 가상 LiDAR 값 제공; 실제 UART 연동은 추후 추가)
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/version.h>
#include "../common/include/forklift.h"

#define DEV_NAME "lidar"

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

static u16 distance_mm;             /* 마지막 측정값 */
static struct timer_list sim_t;     /* 시뮬레이터 */

static void sim_tick(struct timer_list *t)
{
    /* 200mm ~ 3000mm 사이를 왕복하는 파형 */
    static int dir = 1;
    if (dir) distance_mm += 50; else distance_mm -= 50;
    if (distance_mm > 3000) { distance_mm = 3000; dir = 0; }
    if (distance_mm < 200)  { distance_mm = 200;  dir = 1; }
    mod_timer(&sim_t, jiffies + HZ/20);
}

static ssize_t lidar_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    u16 v = distance_mm;
    if (len < sizeof(v)) return -EINVAL;
    if (copy_to_user(buf, &v, sizeof(v))) return -EFAULT;
    return sizeof(v);
}

static long lidar_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
#ifdef IOCTL_LIDAR_GET
    if (cmd == IOCTL_LIDAR_GET) {
        if (copy_to_user((void __user*)arg, &distance_mm, sizeof(distance_mm))) return -EFAULT;
        return 0;
    }
#endif
#ifdef IOCTL_LIDAR_SET
    if (cmd == IOCTL_LIDAR_SET) {
        if (copy_from_user(&distance_mm, (void __user*)arg, sizeof(distance_mm))) return -EFAULT;
        return 0;
    }
#endif
    return -ENOTTY;
}

static const struct file_operations fops = {
    .owner          = THIS_MODULE,
    .read           = lidar_read,
    .unlocked_ioctl = lidar_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl   = lidar_ioctl,
#endif
};

static int __init lidar_init(void)
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

    distance_mm = 1000;
    timer_setup(&sim_t, sim_tick, 0);
    mod_timer(&sim_t, jiffies + HZ/20);

    pr_info("[lidar] ready: /dev/%s0\n", DEV_NAME);
    return 0;

err_class:
    class_destroy(cls);
err_cdev:
    cdev_del(&cdev_);
err_chr:
    unregister_chrdev_region(devno, 1);
    return ret;
}

static void __exit lidar_exit(void)
{
    del_timer_sync(&sim_t);
    device_destroy(cls, devno);
    class_destroy(cls);
    cdev_del(&cdev_);
    unregister_chrdev_region(devno, 1);
    pr_info("[lidar] removed\n");
}

module_init(lidar_init);
module_exit(lidar_exit);
MODULE_LICENSE("GPL");
