#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/types.h>  /* size_t */
#include <linux/errno.h>  /* error codes */
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <asm/uaccess.h>
#include "../include/hardware.h"

#define HOOK_MAJOR 125
#define HOOK_BIT 5
#define HOOK_MASK (1<<HOOK_BIT)

int hookMajor = HOOK_MAJOR;

// fops
int hookOpen (struct inode *inode, struct file *filp);
int hookRelease (struct inode *inode, struct file *filp);
ssize_t hookRead (struct file* inode,char* buf, size_t count,loff_t*);

struct file_operations hookFops = {
    NULL,
    NULL,
    hookRead,
    NULL,
    NULL,       
    NULL,          // select
    NULL,	   // ioctl
    NULL,          // mmap
    hookOpen,
    NULL,
    hookRelease,
    // nothing more, fill with NULLs
};

int init_module(void)
{
	int result;
	
	/* Register your major, and accept a dynamic number */
	result = register_chrdev(hookMajor, "hook", &hookFops);
	if (result < 0) {
        	printk(KERN_WARNING "Keypad: can't get major %d\n",hookMajor);
        	return result;
    	}
	if ( hookMajor == 0 )		/* dynamic */
    		hookMajor = result; 
	
	//  for port test
	//clps_writeb(0x00,PDDDR);
	//clps_writeb(0xff,PDDR);
	return 0;
}

void cleanup_module(void)
{
	//  for port test
	//clps_writeb(0x00,PDDDR);
	//clps_writeb(0x00,PDDR);
	unregister_chrdev(hookMajor, "hook");	
}

//  char device driver functions
//  ----------------------------------------------------------------------------

//static struct wait_queue* hookwq = NULL;

/* Open and close */
int hookOpen (struct inode *inode, struct file *filp)
{
	MOD_INC_USE_COUNT;
	//printk("<1>OPEN!");
	return 0;          /* success */
}

int hookRelease (struct inode *inode, struct file *filp)
{
	MOD_DEC_USE_COUNT;
	//printk("<1>CLOSE!");
	return 0;
}

/* Data management: read */
ssize_t hookRead (struct file* filp,char* buf,size_t count,loff_t* p )
{
	int ch;
	clps_writeb(clps_readb(PDDDR)|HOOK_MASK,PDDDR);
	ch = (clps_readb(PDDR)&HOOK_MASK)?1:0;
	if ( filp->f_flags & O_NONBLOCK ) {	// non-block mode
		return ch;
	}
	
	while ( ch == ((clps_readb(PDDR)&HOOK_MASK)?1:0) ) {
		current->state = TASK_INTERRUPTIBLE;
		schedule_timeout(10*HZ/1000);
	}
		
	return !ch;
}
