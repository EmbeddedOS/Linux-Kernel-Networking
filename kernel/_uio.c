
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/uio_driver.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>


#define UIO_DEV_NAME    "__uio_device"

static struct uio_info *__info = NULL;
static struct device *__dev = NULL;
static int __irq = 10;

module_param(__irq, int, S_IRUGO);

static void __close(struct device *dev)
{
    pr_info("Release uio device.\n");
}

static irqreturn_t __interrupt_handler(int irq, struct uio_info *dev_info)
{
    pr_info("Interrupt handler.\n");
    return IRQ_HANDLED;
}

static int __init __module_init(void)
{
    /* 1. Initialize device. */
    __dev = kzalloc(sizeof(struct device), GFP_KERNEL);
    dev_set_name(__dev, UIO_DEV_NAME);
    __dev->release = __close;
    device_register(__dev);

    __info = kzalloc(sizeof(struct uio_info), GFP_KERNEL);
    __info->name = UIO_DEV_NAME;
    __info->version = "0.0.1";
    __info->irq = __irq;
    __info->irq_flags = IRQF_SHARED;
    __info->handler = __interrupt_handler;

    /* 2. Register a new userspace IO device. */
    if (uio_register_device(__dev, __info) < 0) {
        device_unregister(__dev);
        kfree(__dev);
        kfree(__info);
        pr_info("Failing to register uio device\n");
        return -1;
    }

    pr_info("Registered UIO handler for IRQ=%d\n", __irq);

    return 0;
}

static void __exit __module_exit(void)
{
    uio_unregister_device(__info);
    device_unregister(__dev);
    pr_info("Un-Registered UIO handler for IRQ=%d\n", __irq);
    kfree(__info);
    kfree(__dev);
}

module_init(__module_init);
module_exit(__module_exit);

MODULE_AUTHOR("Cong. Nguyen Thanh");
MODULE_DESCRIPTION("UIO Driver");
MODULE_LICENSE("GPL v2");