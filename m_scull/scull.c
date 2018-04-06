#ifndef _SCULL_H_
#include <scull.h>
#endif

static char *name = "Paul";
static int old = 18;
module_param(name, charp, S_IRUGO);
module_param(old, int, S_IRUGO);
// parmaters shall be noted at /sys/module/hello/parameters

// description of parameters
MODULE_PARM_DESC(name, "your name");    // check modinfo
MODULE_PARM_DESC(old, "your age");

static int __init hello_int(void)
{
	printk("KERN_INFO %s: pid=%d \n", current->comm, current->pid);
	printk("Hello %s(%d) \n", name, old);
	return 0;
}

static void __exit hello_exit(void)
{
	printk("KERN_INFO bye %s \n", name);
}

module_init(hello_int);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paul");
