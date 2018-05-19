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

#ifndef _SCULL_H_
#include "scull.h"
#endif

static int dev_major = DEFAULT_DEV_MAJOR;
static int dev_minor = DEFAULT_DEV_MINOR;
static int dev_num   = DEFAULT_DEV_NUM;
static int dev_quantum = DEFAULT_DEV_QUANTUM;
static int dev_qset    = DEFAULT_DEV_QSET;
module_param(dev_major, int, S_IRUGO);
module_param(dev_minor, int, S_IRUGO);
module_param(dev_num  , int, S_IRUGO);
module_param(dev_quantum, int, S_IRUGO);
module_param(dev_qset   , int, S_IRUGO);
MODULE_PARM_DESC(dev_major, "device major");    // check modinfo
MODULE_PARM_DESC(dev_minor, "device minor");
MODULE_PARM_DESC(dev_num, "device numbers");
MODULE_PARM_DESC(dev_quantum, "device quantum");
MODULE_PARM_DESC(dev_qset, "device qset");

static int  scull_init(void);
static void scull_exit(void);
static int  scull_setup_cdev(struct scull_devices *, int);

struct scull_devices *scull_dev = NULL;

struct file_operations scull_fops = {
	.owner          = THIS_MODULE,
	.llseek         = NULL,
	.read           = scull_read,
	.write          = scull_write,
	.unlocked_ioctl = NULL,
	.open           = scull_open,
	.release        = NULL,//scull_release,

};


int scull_trim(struct scull_devices *s)
{
	int i;
	struct q_set *next, *dptr;

	// go through qset and free memory
	for(dptr = s->q; dptr != NULL; dptr = next)
	{
		if(dptr->data != NULL)
		{
			for(i = 0; i < s->qset; i++)
			{
				kfree(dptr->data[i]);
			}
			kfree(dptr->data);
			dptr->data = NULL;
		}

		// link to next
		next = dptr->next;
		kfree(dptr);
	}

	s->size    = 0;
	s->quantum = dev_quantum;
	s->qset    = dev_qset;
	s->q       = NULL;

	return S_OK;
}

int scull_open(struct inode *inode, struct file *filp)
{
	struct scull_devices *s;
	// get scull struct
	s = container_of(inode->i_cdev, struct scull_devices, cdev);
	// store into private
	filp->private_data = s;

	// trim to zero if open was write-only
	if((filp->f_flags & O_ACCMODE) == O_WRONLY)
	{
		if (down_interruptible(&s->sem))
			return -ERESTARTSYS;

		scull_trim(s);
		up(&s->sem);
	}

	return S_OK;
}

ssize_t scull_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = 0;

	return retval;
}

ssize_t scull_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
	ssize_t retval = 0;

	return retval;

}

static int scull_setup_cdev(struct scull_devices *s, int index)
{
	int ret;
	int devno = MKDEV(dev_major, dev_minor + index);

	// initialize character device
	cdev_init(&s->cdev, &scull_fops);
	s->cdev.owner = THIS_MODULE;
	s->cdev.ops   = &scull_fops;

	// add
	ret = cdev_add(&s->cdev, devno, 1);
	if(ret)
	{
		printk(KERN_NOTICE "add scull%d failed. %d \n", index, ret);
		return ret;
	}

	return S_OK;
}

static int scull_init(void)
{
	int i, ret;
	dev_t dev;

	// register character devices
	if(dev_major)
	{
		dev = MKDEV(dev_major, dev_minor);
		ret = register_chrdev_region(dev, dev_num, "scull");
	}
	else
	{
		ret = alloc_chrdev_region(&dev, dev_minor, dev_num, "scull");
		dev_major = MAJOR(dev);
	}

	if(ret < 0)
	{
		printk(KERN_WARNING "scull: register device. \n");
		return ret;
	}

	scull_dev = kmalloc(sizeof(struct scull_devices)*dev_num, GFP_KERNEL);
	if(scull_dev == NULL)
	{
		ret = -ENOMEM;
		goto NO_MEMORY;
	}

	memset(scull_dev, 0, sizeof(struct scull_devices)*dev_num);

	for(i = 0; i < dev_num; i++)
	{
		scull_dev[i].quantum = dev_quantum;
		scull_dev[i].qset    = dev_qset;
		sema_init(&scull_dev[i].sem, 1);
		ret = scull_setup_cdev(&scull_dev[i], i);
		if(ret != S_OK)
			goto SETUP_CDEV_FAIL;
	}


	return S_OK;

SETUP_CDEV_FAIL:
	kfree(scull_dev);
NO_MEMORY:
	scull_exit();
	return ret;
}

static void scull_exit(void)
{

}

module_init(scull_init);
module_exit(scull_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paul");
