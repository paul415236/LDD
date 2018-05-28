#ifndef _SCULL_H_
#define _SCULL_H_

#include <linux/ioctl.h> /* needed for the _IOW etc stuff used later */

#define S_OK	(0)
#define S_NG	(-1)

#define DEFAULT_DEV_MAJOR	(0)
#define DEFAULT_DEV_MINOR	(0)
#define DEFAULT_DEV_NUM		(4)
#define DEFAULT_DEV_QUANTUM	(4096) // 4k bytes
#define DEFAULT_DEV_QSET	(1000)

#ifdef SCULL_DEBUG
    #define dbg(fmt, args...) \
        printk(KERN_DEBUG "%s(%d): " fmt, __func__, __LINE__, ## args)
#else
    #define dbg(fmt, args...)
#endif


struct q_set{
	void **data;
	struct q_set *next;
};

struct scull_devices{
	struct q_set *q;
	int quantum;
	int qset;
	unsigned long size;
	unsigned int  access_key;
	struct semaphore sem;
	struct cdev cdev;
};

int     scull_open(struct inode *inode, struct file *file);
int     scull_trim(struct scull_devices *dev);
int     scull_release(struct inode *inode, struct file *filp);
loff_t  scull_llseek(struct file *filp, loff_t off, int whence);
ssize_t scull_read(struct file *filp, char __user *buf, size_t count,loff_t *f_pos);
ssize_t scull_write(struct file *filp, const char __user *buf, size_t count,loff_t *f_pos);

int     scull_read_procmem(struct seq_file *s, void *v);
int     scullmem_proc_open(struct inode *inode, struct file *file);

int     scullseq_proc_open(struct inode *inode, struct file *file);
void*   scull_seq_start(struct seq_file *s, loff_t *pos);
void*   scull_seq_next(struct seq_file *s, void *v, loff_t *pos);
void    scull_seq_stop(struct seq_file *s, void *v);
int     scull_seq_show(struct seq_file *s, void *v);


#endif // _SCULL_H_
