#ifndef _HELLO_H_
#define _HELLO_H_

#include <linux/module.h>
#include <linux/init.h>
#include <asm/current.h>
#include <linux/sched.h>

#ifdef HELLO_DEBUG
	#ifdef __KERNEL__
		#define dbg(fmt, args...) \
			printk(KERN_DEBUG "hello: " fmt, ## args)
	#else
		#define dbg(fmt, args...) \
			fprintf(stderr, fmt, ## args)
	#endif
#else
	#define dbg(fmt, args...) \
			
#endif







#endif
