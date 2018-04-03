#include <linux/module.h>
#include <linux/sched.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "scull.h"

/* Initialize major/minor but let the user have an argument */
static int scull_major = SCULL_MAJOR;
static int scull_minor = SCULL_MINOR;
module_param(scull_major, int, S_IRUGO);
module_param(scull_minor, int, S_IRUGO);

/* Initialize scull quantum and scull set */
static int scull_qset = SCULL_QSET;
static int scull_quantum = SCULL_QUANTUM;
module_param(scull_qset, int, S_IRUGO);
module_param(scull_quantum, int, S_IRUGO);

static int scull_nr_devs = 1;

struct scull_dev *scull_devices;

/* free data area */
static int scull_trim(struct scull_dev *dev)
{
    struct scull_qset *next, *dptr;
    int qset = dev->qset;
    int i;

    for (dptr = dev->data; dptr; dptr=next) {
        if (dptr->data) {
            for (i=0; i< qset; i++)
                kfree(dptr->data[i]);
            kfree(dptr->data);
            dptr->data = NULL;
        }
        next = dptr->next;
        kfree(dptr);
    }
    dev->size=0;
    dev->quantum = scull_quantum;
    dev->qset = scull_qset;
    dev->data = NULL;
    return 0;
}

/* open function */
static int scull_open(struct inode *inode, struct file *filp)
{
    struct scull_dev *dev; /* device information */

    dev = container_of(inode->i_cdev, struct scull_dev, cdev);
    filp->private_data = dev; /* for other methods */
    /* trim to 0 the length of the device if open was write-only */
    if ((filp->f_flags & O_ACCMODE) == O_WRONLY)
        scull_trim(dev);
    return 0;
}

/* close operation */
static int scull_release(struct inode *inode, struct file *flip)
{
    return 0;
}

/* find item index in qset list */
static struct scull_qset *scull_follow(struct scull_dev *dev, int index)
{
    struct scull_qset *qs = dev->data;

    /* If there is no qset just add one */
    if (!qs) {
        qs = dev->data = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
        if (!qs)
            return NULL;
        memset(qs, 0, sizeof(struct scull_qset));
    }

    while (index--) {
        if (!qs->next) {
            qs->next = kmalloc(sizeof(struct scull_qset), GFP_KERNEL);
            if (!qs->next)
                return NULL;
            memset(qs->next, 0 , sizeof(struct scull_qset));
        }
        qs = qs->next;
        continue;
    }
    return qs;
}

/* read function */
static ssize_t scull_read(struct file *flip, char __user *buf, size_t count, loff_t *f_pos)
{
    struct scull_dev *dev = flip->private_data;
    struct scull_qset *dptr;
    int quantum, qset;
    int itemsize;
    int item, s_pos, q_pos, rest;
    ssize_t retval = 0;

    /* DEBUG */
    printk(KERN_NOTICE "scull: read %ld size from %ld. size %ld.", (long)count, (long)*f_pos, (long)dev->size);

    /* TO DO
    if (douwn_interruptible(&dev->sem))
        return -ERESTARTSYS;*/
    quantum = dev->quantum;
    qset = dev -> qset;
    itemsize = quantum * qset;

    /* check if out of size or at end of device */
    if (*f_pos >= dev->size) {
        printk(KERN_WARNING "scull: End of device or out of device position\n");
        goto out;
    }
    if (*f_pos + count > dev->size) {
        count = dev->size -*f_pos;
        printk(KERN_WARNING "scull: Only %d left in device.\n", (int)count);
    }

    /*find listitem, qset index and offset in the qunatum */
    item = (long)*f_pos / itemsize;
    rest = (long)*f_pos % itemsize;
    s_pos = rest / quantum;
    q_pos = rest % quantum;

    /* follow the list up to the right position (defined elsewhere) */
    dptr = scull_follow(dev, item);

    if (dptr == NULL || !dptr->data || !dptr->data[s_pos])
        goto out;

    /* read only up to the end of the qunatum */
    if (count > quantum - q_pos)
        count = quantum -q_pos;

    if (copy_to_user(buf, dptr->data[s_pos] + q_pos, count)) {
        retval = -EFAULT;
        goto out;
    }
    *f_pos += count;
    retval = count;

    out:
        /* TO DO
        up(&dev->sem);*/
        return retval;
}

/* write function */
static ssize_t scull_write(struct file *flip, const char __user *buf, size_t count, loff_t *f_pos)
{
    struct scull_dev *dev = flip->private_data;
    struct scull_qset *dptr;
    int quantum = dev->quantum, qset = dev->qset;
    int itemsize = quantum * qset;
    int item, s_pos, q_pos, rest;
    ssize_t retval = -ENOMEM;

    /* TO DO
    if (down_interruptible(&dev->sem))
        return -ERESTARTSYS;*/

    /* find listitem , qset index and offset in quantum */
    item = (long)*f_pos / itemsize;
    rest = (long)*f_pos % itemsize;
    s_pos = rest / quantum;
    q_pos = rest % quantum;

    /* Follow the list up to the right position */
    dptr = scull_follow(dev, item);
    if (dptr == NULL)
        goto out;

    if (!dptr->data) {
        /* alloc and init qset */
        dptr->data = kmalloc(qset * sizeof(char*), GFP_KERNEL);
        if (!dptr->data)
            goto out;
        memset(dptr->data, 0, qset * sizeof(char*));
    }
    if (!dptr->data[s_pos]) {
        dptr->data[s_pos] = kmalloc(quantum, GFP_KERNEL);
        if (!dptr->data[s_pos])
            goto out;
    }

    /* Only write up to the end of the quantum */
    if (count > quantum - q_pos)
        count = quantum - q_pos;

    if (copy_from_user(dptr->data[s_pos]+q_pos, buf, count)) {
        retval = -EFAULT;
        goto out;
    }
    *f_pos += count;
    retval = count;

    /* update the size */
    if (dev->size < *f_pos)
        dev->size = *f_pos;

    /* DEBUG */
    printk(KERN_NOTICE "scull: wrote %ld size from %ld. size %ld.", (long)count, (long)*f_pos, (long)dev->size);

    out:
        /* TO DO
        up(&dev->sem);*/
        return retval;
}

/* File operations structure */
static struct file_operations scull_fops = {
    .owner = THIS_MODULE, /* used to prevent unload while operations are used */
    .open = scull_open,
    .release = scull_release,
    .read = scull_read,
    .write = scull_write,
};

/* cdev registration function */
static void scull_setup_cdev(struct scull_dev *dev, int index)
{
    int err;
    int devno = MKDEV(scull_major, scull_minor + index);

    cdev_init(&dev->cdev, &scull_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &scull_fops;
    err = cdev_add(&dev->cdev, devno, 1);
    if (err)
        printk(KERN_NOTICE "scull: Error %d while adding scull cde %d", err, index);
}

/* exit function */
static void __exit scull_exit(void)
{
    int i;
    dev_t devno = MKDEV(scull_major, scull_minor);

    /* remove char devices */
    if (scull_devices) {
        for (i=0; i<scull_nr_devs; i++) {
            scull_trim(scull_devices + i);
            cdev_del(&scull_devices[i].cdev);
        }
        kfree(scull_devices);
    }

    /* We are sure register suceeded because module fails at init if otherwise */
    unregister_chrdev_region(devno, scull_nr_devs);
    printk(KERN_INFO "scull: Unloaded module.\n");
}

/* init function */
static int __init scull_init(void)
{
    int result, i;
    dev_t dev = 0;

    /* Allocate major and minor */
    if (scull_major) {
        dev = MKDEV(scull_major, scull_minor);
        result = register_chrdev_region(dev, scull_nr_devs, "scull");
    } else {
        result = alloc_chrdev_region(&dev, scull_minor, scull_nr_devs, "scull");
        scull_major = MAJOR(dev);
    }
    if (result < 0)
        printk(KERN_WARNING "scull: Can't get major %d.\n", scull_major);
    else
        printk(KERN_INFO "scull: Got major %d.\n", scull_major);

    /* allocate devices */
    scull_devices = kmalloc(scull_nr_devs * sizeof(struct scull_dev), GFP_KERNEL);
    if (!scull_devices) {
        result = -ENOMEM;
        goto fail;
    }
    memset(scull_devices, 0, scull_nr_devs * sizeof(struct scull_dev));

    /* init each device */
    for (i=0; i<scull_nr_devs; i++) {
        scull_devices[i].quantum = scull_quantum;
        scull_devices[i].qset = scull_qset;
        scull_setup_cdev(&scull_devices[i], i);
    }

    printk(KERN_NOTICE "scull: Initialized %d scull devices.\n", scull_nr_devs);
    return 0;

    fail:
        scull_exit();
        return result;
}

module_init(scull_init);
module_exit(scull_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Andrei Gherzan <andrei@gherzan.ro>");

