#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/malloc.h>
#include <linux/fs.h>
#include <linux/types.h>  /* size_t */
#include <linux/errno.h>  /* error codes */
#include <linux/fcntl.h>        /* O_ACCMODE */
#include <asm/uaccess.h>
#include "../include/hardware.h"

#include "flash_intel.h"

#define FLASH_MAJOR 123

int flashMajor = FLASH_MAJOR;

// fops
int flashOpen (struct inode *inode, struct file *filp);
int flashRelease (struct inode *inode, struct file *filp);
ssize_t flashRead (struct file* inode,char* buf, size_t count,loff_t*);
ssize_t flashWrite (struct file* filp,const char* buf,size_t count,loff_t* p);
loff_t flashLlseek (struct file *, loff_t, int);

struct file_operations flashFops = {
    NULL,
    flashLlseek,
    flashRead,
    flashWrite,
    NULL,       
    NULL,          // select
    NULL,	   // ioctl
    NULL,          // mmap
    flashOpen,
    NULL,
    flashRelease,
    // nothing more, fill with NULLs
};

extern void flashBusWrite(unsigned int addr,unsigned short value);
extern unsigned short flashBusRead(unsigned int addr);

int init_module(void)
{
	int result;

	if ( !flash_check_id() ) {
		printk(KERN_WARNING "Flash: can't find flash\n");
		return -1;
	}
		
	/* Register your major, and accept a dynamic number */
	result = register_chrdev(flashMajor, "flash", &flashFops);
	if (result < 0) {
        	printk(KERN_WARNING "Flash: can't get major %d\n",flashMajor);
        	return result;
    	}
	if ( flashMajor == 0 )		/* dynamic */
    		flashMajor = result; 
	
	return 0;
}

void cleanup_module(void)
{
	unregister_chrdev(flashMajor, "flash");	
}

//  char device driver functions
//  ----------------------------------------------------------------------------

/* Open and close */
int flashOpen (struct inode *inode, struct file *filp)
{
	filp->f_pos = 0;
	MOD_INC_USE_COUNT;
	return 0;          /* success */
}

int flashRelease (struct inode *inode, struct file *filp)
{
	MOD_DEC_USE_COUNT;
	return 0;
}

/* Data management: write */
ssize_t flashRead (struct file* filp,char* buf,size_t count,loff_t* p )
{
	copy_to_user(buf,(char*)(FLASH_BASE+(unsigned long)filp->f_pos),count);
	return count;
}

ssize_t flashWrite (struct file* filp,const char* buf,size_t count,loff_t* p )
{
	unsigned char* kbuf;
	printk("<1>write to 0x%lx with %d\n",(unsigned long)filp->f_pos,count);
		
	if ( count <0 )
		return -1;
	if ( count ==0 )
		return 0;
		
	if ( filp->f_pos < 56*1024 )
		return -1;
	if ( filp->f_pos < 64*1024 ) {
		if ( filp->f_pos % (8*1024) )
			return -1;
		if ( count > 8*1024 )
			return -1;
	} else {
		if ( filp->f_pos % (64*1024) )
			return -1;
		if ( count > 64*1024 )
			return -1;
	}
	kbuf = (unsigned char*)kmalloc(count,GFP_KERNEL);
	if ( kbuf == NULL ) {
		printk(KERN_WARNING "Flash:not enough memory.\n");
		return -1;
	}
	copy_from_user(kbuf,buf,count);
	flash_unlock((unsigned long)(FLASH_BASE+(unsigned long)filp->f_pos));
	//printk("<1>Unlocked\n");
	flash_erase((unsigned long)(FLASH_BASE+(unsigned long)filp->f_pos));
	//printk("<1>Erased\n");
	flash_program((unsigned long)(FLASH_BASE+(unsigned long)filp->f_pos),
		count,(short*)kbuf);
	//printk("<1>Written\n");
	flash_lock((unsigned long)(FLASH_BASE+(unsigned long)filp->f_pos));
	//printk("<1>Locked\n");
	kfree(kbuf);
	filp->f_pos += count;
	return count;
}

loff_t flashLlseek(struct file *filp, loff_t off, int whence)
{
    loff_t newpos;

    //printk("<1>seek to %d\n",(int)off);
    switch(whence) {
      case 0: /* SEEK_SET */
        newpos = off;
        break;

      case 1: /* SEEK_CUR */
        newpos = filp->f_pos + off;
        break;

      case 2: /* SEEK_END */
        newpos = 0x200000 + off;
        break;

      default: /* can't happen */
        return -EINVAL;
    }
    if (newpos<0) return -EINVAL;
    filp->f_pos = newpos;
    return newpos;
}