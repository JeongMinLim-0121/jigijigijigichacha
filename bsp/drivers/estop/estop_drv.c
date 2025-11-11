// estop_drv.c
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/version.h>
#include "../common/include/forklift.h"   // IOCTL_ESTOP_* 과 fk_* 정의

#define DEV_NAME "estop"

static dev_t devno;
static struct cdev cdev_;
static struct class *cls;
static struct device *devnode;

static int estop; /* 0: 해제, 1: 비상정지 */

/* 커널 버전에 따른 class_create 시그니처 차이 처리 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
  #define CLASS_CREATE(name) class_create(name)
#else
  #define CLASS_CREATE(name) CLASS_CREATE(name)
#endif

/* ---------- file ops ---------- */

static long estop_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    switch (cmd) {
    case IOCTL_ESTOP_TRIP:     /* 비상정지 ON */
        estop = 1;
        return 0;
    case IOCTL_ESTOP_CLEAR:    /* 비상정지 해제 */
        estop = 0;
        return 0;
    case IOCTL_ESTOP_GET:      /* 현재 상태 읽기 */
        if (copy_to_user((void __user *)arg, &estop, sizeof(estop)))
            return -EFAULT;
        return 0;
    default:
        return -ENOTTY;
    }
}

static ssize_t estop_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
    if (len < sizeof(int))
        return -EINVAL;
    if (copy_to_user(buf, &estop, sizeof(int)))
        return -EFAULT;
    return sizeof(int);
}

static const struct file_operations fops = {
    .owner           = THIS_MODULE,
    .unlocked_ioctl  = estop_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl    = estop_ioctl,
#endif
    .read            = estop_read,
};

/* ---------- init / exit ---------- */

static int __init estop_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&devno, 0, 1, DEV_NAME);
    if (ret)
        return ret;

    cdev_init(&cdev_, &fops);
    ret = cdev_add(&cdev_, devno, 1);
    if (ret)
        goto err_chr;

    cls = CLASS_CREATE(DEV_NAME);
    if (IS_ERR(cls)) {
        ret = PTR_ERR(cls);
        goto err_cdev;
    }

    devnode = device_create(cls, NULL, devno, NULL, DEV_NAME "%d", 0);
    if (IS_ERR(devnode)) {
        ret = PTR_ERR(devnode);
        goto err_class;
    }

    pr_info("[estop] ready: /dev/%s0 (init ok)\n", DEV_NAME);
    return 0;

err_class:
    class_destroy(cls);
err_cdev:
    cdev_del(&cdev_);
err_chr:
    unregister_chrdev_region(devno, 1);
    return ret;
}

static void __exit estop_exit(void)
{
    device_destroy(cls, devno);
    class_destroy(cls);
    cdev_del(&cdev_);
    unregister_chrdev_region(devno, 1);
    pr_info("[estop] removed\n");
}

module_init(estop_init);
module_exit(estop_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("ijm");
MODULE_DESCRIPTION("Emergency Stop char device (/dev/estop0)");
