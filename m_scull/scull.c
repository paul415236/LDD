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

struct scull_devices *scull_dev = NULL;
struct file_operations scull_fops = {
	.owner          = THIS_MODULE,
	.llseek         = scull_llseek,
	.read           = scull_read,
	.write          = scull_write,
	.unlocked_ioctl = NULL,
	.open           = scull_open,
	.release        = scull_release,

};

static void __exit scull_exit(void);
static int  __init scull_init(void);
struct q_set      *scull_follow(struct scull_devices *s, int n);
static int         scull_setup_cdev(struct scull_devices *, int);

int scull_trim(struct scull_devices *s)
{
	int i;
	struct q_set *next, *dptr;

	// go through qset and free memory
	for(dptr = s->q; dptr; dptr = next)
	{
		if(dptr->data)
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

        dbg("open with write only mode. \n");
		scull_trim(s);
		up(&s->sem);
	}

	dbg("open success \n");

	return S_OK;
}

struct q_set *scull_follow(struct scull_devices *s, int n)
{
    struct q_set *dptr = s->q;

    if(dptr == NULL)
    {
        // LDD allocates memory for set in this case, but i think we should not do it..
        // no! we should do it if it's writting
        dptr = s->q = kmalloc(sizeof(struct q_set), GFP_KERNEL);
		if (dptr == NULL)
			return NULL;  /* Never mind */

		memset(dptr, 0, sizeof(struct q_set));
    }

    while(n--)
    {
        if(!dptr->next)
        {
            dptr->next = kmalloc(sizeof(struct q_set), GFP_KERNEL);
			if (dptr->next == NULL)
				return NULL;  /* Never mind */

			memset(dptr->next, 0, sizeof(struct q_set));
        }

        dptr = dptr->next;
    }

    return dptr;
}

ssize_t scull_read(struct file *filp,
                char __user *buf,
                size_t count,
                loff_t *f_pos)
{
    struct scull_devices *s = filp->private_data;
    struct q_set *dptr;
    int setsize = s->qset*s->quantum;
    int item, s_pos, q_pos, rest;
	ssize_t ret = 0;

	if (down_interruptible(&s->sem))
		return -ERESTARTSYS;

    if(*f_pos >= s->size)
    {
        dbg("err. read size \n");
        goto OUT;
    }

    if((*f_pos + count) > s->size)
        count = s->size - *f_pos;

    // search read point
    item  = (long)*f_pos / setsize;     // how many sets we should cross
    rest  = (long)*f_pos % setsize;     // how big the size we should cross at the rest set
    s_pos = rest / s->quantum;          // how many quantum we should cross at the rest set
    q_pos = rest % s->quantum;          // how many size w should cross at the rest quantum

    // then we follow up the list according to the offset we just cauculated
    dptr = scull_follow(s, item);
    if(dptr == NULL || dptr->data == NULL || dptr->data[s_pos] == NULL)
    {
        dbg("not found \n");
        goto OUT;
    }

    // read quantum by qunatum one time
    if(count > (s->quantum - q_pos))
        count = s->quantum - q_pos;

    dbg("count = %d \n", (int)count);

    if(copy_to_user(buf, dptr->data[s_pos] + q_pos, count))
    {
        ret = -EFAULT;
        goto OUT;
    }

    *f_pos += count;
    ret = count;

OUT:
    up(&s->sem);


	return ret;
}

ssize_t scull_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    struct scull_devices *s = filp->private_data;
    struct q_set *dptr;
    int setsize = s->qset*s->quantum;
    int item, s_pos, q_pos, rest;
	ssize_t ret = 0;

	if (down_interruptible(&s->sem))
		return -ERESTARTSYS;

    // search read point
    item  = (long)*f_pos / setsize;     // how many sets we should cross
    rest  = (long)*f_pos % setsize;     // how big the size we should cross at the rest set
    s_pos = rest / s->quantum;          // how many quantum we should cross at the rest set
    q_pos = rest % s->quantum;          // how many size w should cross at the rest quantum

    // then we follow up the list according to the offset we just cauculated
    dptr = scull_follow(s, item);
    if (dptr == NULL)
		goto OUT;
	if (!dptr->data)
	{
		dptr->data = kmalloc(s->qset * sizeof(char *), GFP_KERNEL);
		if (!dptr->data)
			goto OUT;

		memset(dptr->data, 0, s->qset * sizeof(char *));
	}
	if (!dptr->data[s_pos])
	{
		dptr->data[s_pos] = kmalloc(s->quantum, GFP_KERNEL);
		if (!dptr->data[s_pos])
			goto OUT;
	}

    // read quantum by qunatum one time
    if(count > (s->quantum - q_pos))
        count = s->quantum - q_pos;

    dbg("count = %d \n", (int)count);

    if(copy_from_user(dptr->data[s_pos] + q_pos, buf, count))
    {
        ret = -EFAULT;
        goto OUT;
    }

    *f_pos += count;
    ret = count;

    if(s->size < *f_pos)
        s->size = *f_pos;

OUT:
    up(&s->sem);


    return ret;
}

loff_t scull_llseek(struct file *filp, loff_t off, int whence)
{
	struct scull_devices *dev = filp->private_data;
	loff_t newpos;

	switch(whence) {
	  case 0: /* SEEK_SET */
		newpos = off;
		break;

	  case 1: /* SEEK_CUR */
		newpos = filp->f_pos + off;
		break;

	  case 2: /* SEEK_END */
		newpos = dev->size + off;
		break;

	  default: /* can't happen */
		return -EINVAL;
	}
	if (newpos < 0) return -EINVAL;
	filp->f_pos = newpos;
	return newpos;
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

	dbg("add cdev %d \n", index);

	return S_OK;
}

static int __init scull_init(void)
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

static void __exit scull_exit(void)
{
    int i;
    dev_t devno = MKDEV(dev_major, dev_minor);

    // free memory
    if(scull_dev)
    {
        for(i = 0; i < dev_num; i++)
        {
            scull_trim(&scull_dev[i]);
            cdev_del(&scull_dev[i].cdev);
        }
        kfree(scull_dev);
    }

    // unreginster character device
    unregister_chrdev_region(devno, dev_num);
}

int scull_release(struct inode *inode, struct file *filp)
{
	return 0;
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Paul");
