#ifndef _SCULL_H_
#include <scull.h>
#endif

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/proc_fs.h>
#include <linux/fcntl.h>	/* O_ACCMODE */
#include <linux/seq_file.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>	/* copy_*_user */

#include <asm/current.h>

static int dev_major = DEFAULT_DEV_MAJOR;
static int dev_minor = DEFAULT_DEV_MINOR;
static int dev_num   = DEFAULT_DEV_NUM;
module_param(dev_major, int, S_IRUGO);
module_param(dev_minor, int, S_IRUGO);
module_param(dev_num  , int, S_IRUGO);
MODULE_PARM_DESC(dev_major, "device major");    // check modinfo
MODULE_PARM_DESC(dev_minor, "device minor");
MODULE_PARM_DESC(dev_num, "device numbers");

struct file_operations scull_fops = {
	.owner = THIS_MODULE,
	.llseek = NULL,
	.read   = scull_read,
	.write  = scull_write,
	
};
static int __init scull_int(void)
{
	printk("KERN_INFO init scull. pid=%d \n", current->pid);
	return 0;
}

static void __exit scull_exit(void)
{
	printk("KERN_INFO exit scull. \n");
}


module_init(scull_int);
module_exit(scull_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paul");
