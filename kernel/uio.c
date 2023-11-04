
#include <linux/init.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/uio_driver.h>
#include <linux/of_irq.h>
#include <linux/interrupt.h>

static struct uio_info *_info = NULL;
static struct device *_dev = NULL;
static int _irq = 0;

module_param(_irq, int, S_IRUGO);

static void __close(struct device *dev)
{
    pr_info("Release uio device\n");
}

static irqreturn_t __interrupt_handler(int irq, struct uio_info *dev_info)
{
    pr_info("Interrupt handler.\n");
    return IRQ_HANDLED;
}

static int __init __module_init(void)
{
    return 0;
}

static void __exit __module_exit(void)
{
}

module_init(__module_init);
module_exit(__module_exit);

MODULE_AUTHOR("Cong. Nguyen Thanh");
MODULE_DESCRIPTION("UIO Driver");
MODULE_LICENSE("GPL v2");