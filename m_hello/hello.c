#include <linux/module.h>
#include <linux/init.h>
#include <asm/current.h>
#include <linux/sched.h>

static int __init hello_int(void)
{
	printk("KERN_INFO hello %s: pid=%d \n", current->comm, current->pid);
	return 0;
}

static void __exit hello_exit(void)
{
	printk("KERN_INFO bye \n");
}

module_init(hello_int);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paul");
