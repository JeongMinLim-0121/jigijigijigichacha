#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/timer.h>
#include <linux/version.h>   // ★ 커널 버전 매크로
#include "../common/include/forklift.h"

#define DEV_NAME "encoder"

static dev_t devno;
static struct cdev cdev_;
static struct class *cls;
static struct device *devnode;

static unsigned int enc_speed; /* 가상 속도 단위 */
static unsigned int ticks;
static struct timer_list tick;

/* 커널 버전에 따라 class_create 매크로 정리 */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,4,0)
  #define CLASS_CREATE(name) class_create(name)
#else
  #define CLASS_CREATE(name) CLASS_CREATE(name)
#endif

static void tick_fn(struct timer_list *t)
{
    ticks += enc_speed; /* 속도 비례 증가 (단위 임의) */
    mod_timer(&tick, jiffies + HZ/10);
}

static long enc_ioctl(struct file *f, unsigned int cmd, unsigned long arg)
{
    struct fk_enc_stat st;

    switch (cmd) {
    case IOCTL_ENC_SET_SPEED:
        if (copy_from_user(&enc_speed, (void __user *)arg, sizeof(enc_speed)))
            return -EFAULT;
        if (!timer_pending(&tick))
            mod_timer(&tick, jiffies + HZ/10);
        return 0;

    case IOCTL_ENC_GET_STAT:
        st.ticks = ticks;
        st.speed = enc_speed;
        if (copy_to_user((void __user *)arg, &st, sizeof(st)))
            return -EFAULT;
        return 0;
    }

    return -ENOTTY;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = enc_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = enc_ioctl, /* 간단히 동일 처리 (필요 시 분리) */
#endif
};

static int __init enc_init(void)
{
    int ret;

    ret = alloc_chrdev_region(&devno, 0, 1, DEV_NAME);
    if (ret)
        return ret;

    cdev_init(&cdev_, &fops);
    ret = cdev_add(&cdev_, devno, 1);
    if (ret)
        goto err_chrdev;

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

    timer_setup(&tick, tick_fn, 0);

    pr_info("[encoder] ready: /dev/%s0\n", DEV_NAME);
    return 0;

err_class:
    class_destroy(cls);
err_cdev:
    cdev_del(&cdev_);
err_chrdev:
    unregister_chrdev_region(devno, 1);
    return ret;
}

static void __exit enc_exit(void)
{
    del_timer_sync(&tick);
    device_destroy(cls, devno);
    class_destroy(cls);
    cdev_del(&cdev_);
    unregister_chrdev_region(devno, 1);
}

module_init(enc_init);
module_exit(enc_exit);
MODULE_LICENSE("GPL");
