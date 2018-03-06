#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <asm/arch/irqs.h>
#include <linux/types.h>  /* size_t */
#include <linux/errno.h>  /* error codes */
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <asm/system.h>   /* cli(), *_flags */
#include <asm/uaccess.h>
#include <asm/fcntl.h>
#include <linux/wait.h>
#include <linux/poll.h>

#include "keypad.h"

int keypadMajor = KEYPAD_MAJOR;

// fops
int keypadOpen (struct inode *inode, struct file *filp);
int keypadRelease (struct inode *inode, struct file *filp);
ssize_t keypadRead (struct file* inode,char* buf, size_t count,loff_t*);
unsigned int keypadPoll (struct file *, struct poll_table_struct *);

struct file_operations keypadFops = {
    NULL,
    NULL,
    keypadRead,
    NULL,
    NULL,       
    keypadPoll,    // was select
    NULL,	   // ioctl
    NULL,          // mmap
    keypadOpen,
    NULL,
    keypadRelease,
    // nothing more, fill with NULLs
};

int init_module(void)
{
	int result;
	
	if ( keypadInit() <0 )
		return -1;

	/* Register your major, and accept a dynamic number */
	result = register_chrdev(keypadMajor, "keypad", &keypadFops);
	if (result < 0) {
        	printk(KERN_WARNING "Keypad: can't get major %d\n",keypadMajor);
        	return result;
    	}
	if ( keypadMajor == 0 )		/* dynamic */
    		keypadMajor = result; 
    	
	//  register interrupt routine	
	initInterrupt();
	if ( request_irq(IRQ_KBDINT,&keypadInterrupt,SA_INTERRUPT,
		"keypad",NULL) != 0 ) {
		keypadClose();
		printk(KERN_WARNING "Keypad:Error to install IRQ routine.\n");
		return -1;
	}
	
	return 0;
}

void cleanup_module(void)
{
	keypadClose();
	
	unregister_chrdev(keypadMajor, "keypad");	
	
	free_irq(IRQ_KBDINT,NULL);
}

//  char device driver functions
//  ----------------------------------------------------------------------------

DECLARE_WAIT_QUEUE_HEAD (mwq);

/* Open and close */
int keypadOpen (struct inode *inode, struct file *filp)
{
	filp->f_pos = (unsigned long)keypadBufLoc;
	MOD_INC_USE_COUNT;
	return 0;          /* success */
}

int keypadRelease (struct inode *inode, struct file *filp)
{
	MOD_DEC_USE_COUNT;
	return 0;
}

/* Data management: write */
ssize_t keypadRead (struct file* filp,char* buf,size_t count,loff_t* p )
{
	int loc = (int)filp->f_pos;
	int size = 0;
	
	while ( size == 0 ) {
		if ( loc == keypadBufLoc ) {	//  no new key to read
			if ( filp->f_flags & O_NONBLOCK ) {	// non-block mode
				return -EAGAIN;
			}
			interruptible_sleep_on(&mwq);	//  wait for a key press
			if ( loc == keypadBufLoc )  	//  wake up by other
				break;
		}
		if ( loc > keypadBufLoc ) {	//  read behide write
			size = min(count,BUFFER_SIZE-loc);
			copy_to_user(buf,keypadBuffer+loc,size);
			loc += size;
			if ( loc >= BUFFER_SIZE )
				loc = 0;
			filp->f_pos = loc;
			count -= size;
		}
		if ( count ) {	
			size = min(count,keypadBufLoc-loc);
			copy_to_user(buf,keypadBuffer+loc,size);
			filp->f_pos += size;
			/*{
				int i;
				for ( i=0; i<size; ++i )
					printk("<1>%c",keypadBuffer[loc+i]);
				printk("<1>\n");
			}*/
		} 
	} 
    	return size;
}

unsigned int keypadPoll (struct file *filp, struct poll_table_struct *table)
{
	if ( filp->f_pos != keypadBufLoc ) 	//  readable
		return 1;
	poll_wait(filp, &mwq, table);
	return 0;
}