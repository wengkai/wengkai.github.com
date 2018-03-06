#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/malloc.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <asm/arch/irqs.h>
#include <linux/types.h>  /* size_t */
#include <linux/errno.h>  /* error codes */
#include <linux/fcntl.h>  /* O_ACCMODE */
#include <asm/system.h>   /* cli(), *_flags */
#include <asm/uaccess.h>
#include <asm/fcntl.h>
#include <linux/poll.h>

#include "../include/hardware.h"
#include "codec.h"

int codecMajor = CODEC_MAJOR;

static unsigned char* codecRXBuf  =0;
static unsigned char* codecTXBuf  =0;
static volatile unsigned int codecRXLoc    =0;	//  location for FIFO write to buf
static volatile unsigned int codecTXLoc	  =0;	//  location for write to FIFO
static volatile unsigned int codecWriteLoc =0;	//  location for write to buffer
static volatile int isTXFull  =0;
static volatile int isTXEmpty =1;

//static struct wait_queue* wq = NULL;
DECLARE_WAIT_QUEUE_HEAD (mwq);

int codecInit();
void codecClose();

void codecInterrupt(int irq, void* dev_id, struct pt_regs* regs);

// fops
int codecOpen (struct inode *inode, struct file *filp);
int codecRelease (struct inode *inode, struct file *filp);
ssize_t codecRead (struct file* filp, char* buf, size_t count,loff_t*);
ssize_t codecWrite (struct file *filp, const char *buf, size_t count, loff_t * p);
unsigned int codecPoll (struct file *, struct poll_table_struct *);
int codecIoctl(struct inode *, struct file *, unsigned int, unsigned long);

struct file_operations codecFops = {
    NULL,
    NULL,
    codecRead,
    codecWrite,
    NULL,       
    codecPoll,    // was select
    codecIoctl,	   // ioctl
    NULL,          // mmap
    codecOpen,
    NULL,
    codecRelease,
    // nothing more, fill with NULLs
};

//  module functions
//  ------------------------------------------------------------------------

int init_module(void)
{
	int result;

	//printk("<1>Installing codec module.\n");
		
	if ( codecInit() <0 ) {
		codecClose();
		return -1;
	}

	/* Register your major, and accept a dynamic number */
	result = register_chrdev(codecMajor, "codec", &codecFops);
	if (result < 0) {
        	printk(KERN_WARNING "Codec: can't get major %d\n",codecMajor);
        	return result;
    	}
	if ( codecMajor == 0 )		/* dynamic */
    		codecMajor = result; 
    	
	//  register interrupt routine	
	if ( request_irq(IRQ_CSINT,&codecInterrupt,SA_INTERRUPT,
		"codec",NULL) != 0 ) {
		codecClose();
		printk(KERN_WARNING "Codec:Error to install IRQ routine.\n");
		return -1;
	}

	/*//  for debug
	{
	int i;
	IO_SYSCON = IO_SYSCON | (CDENTX | CDENRX);
	for ( i=0; i<FIFO_SIZE; ++i )
		IO_CODR = BLANK_CODE;	
	}*/
	
	return 0;
}

void cleanup_module(void)
{
	codecClose();
	
	unregister_chrdev(codecMajor, "codec");	
	
	free_irq(IRQ_CSINT,NULL);
}

//  codec functions
//  --------------------------------------------------------------------------

int codecInit()
{
	//printk("<1>Codec initing.\n");
	
	//  alloc buffer
	codecRXBuf = (unsigned char*)kmalloc(BUFFER_SIZE,GFP_KERNEL);
	codecTXBuf = (unsigned char*)kmalloc(BUFFER_SIZE,GFP_KERNEL);
	if ( codecRXBuf == NULL || codecTXBuf == NULL ) {
		printk(KERN_WARNING "Codec:not enough memory.\n");
		return -1;
	}

	codecRXLoc = codecTXLoc =0;
	codecWriteLoc =0;
	isTXFull  =0;
	isTXEmpty =1;
	
	//  select CODEC
	//IO_SYSCON2 = IO_SYSCON2 | SERSEL;
	clps_writel(clps_readl(SYSCON2) | SERSEL, SYSCON2);
	//IO_SYSCON3 = IO_SYSCON3 & ~DAISEL;
	clps_writel(clps_readl(SYSCON3) & ~DAISEL, SYSCON3);
	
	return 0;
}

void codecClose()
{
	if ( codecRXBuf ) {
		kfree(codecRXBuf);
		codecRXBuf = 0;
	}
	if ( codecTXBuf ) {
		kfree(codecTXBuf);
		codecTXBuf = 0;
	}
	
	//  stop RX/TX, if there's any
	//IO_SYSCON = IO_SYSCON & ~(CDENTX | CDENRX);
	clps_writel(clps_readl(SYSCON1) & ~(CDENTX | CDENRX), SYSCON1);
}

//  interrupt routine
//  ---------------------------------------------------------------------------

void codecInterrupt(int irq, void* dev_id, struct pt_regs* regs)
{
	int i;

	//printk("<1>TX=%x,Write=%x,isTXEmpty=%d\n",codecTXLoc,codecWriteLoc,
	//isTXEmpty);	
	//  fill 8-byte to/from FIFO
	for ( i =0; i<FIFO_SIZE/2; ++i ) {
		//codecRXBuf[codecRXLoc++] = IO_CODR;
		codecRXBuf[codecRXLoc++] = clps_readb(CODR);
		if ( isTXEmpty )
			//IO_CODR = BLANK_CODE;
			clps_writeb(BLANK_CODE,CODR);
		else {
			//IO_CODR = codecTXBuf[codecTXLoc++];
			clps_writeb(codecTXBuf[codecTXLoc++],CODR);
			if ( codecTXLoc == codecWriteLoc )
				isTXEmpty =1;
                }
		//IO_CODR = BLANK_CODE;
		//IO_CODR = codecRXBuf[codecRXLoc-1];
	}
	if ( codecRXLoc == BUFFER_SIZE )	//  wrap back
		codecRXLoc = 0;
	if ( codecTXLoc == BUFFER_SIZE )	//  wrap back
		codecTXLoc = 0;
	
	if ( codecTXLoc == codecWriteLoc )
		isTXEmpty =1;
	isTXFull =0;
	
	wake_up_interruptible(&mwq);		//  wake up waited read() thread	
	
	//  clear interrupt status
	//IO_COEOI = 0x0;
	clps_writeb(0x0,COEOI);
}

//  char device driver functions
//  ----------------------------------------------------------------------------

/* Open and close */
int codecOpen (struct inode *inode, struct file *filp)
{
	int i;
	
	//printk("<1>codec opening.\n");
	
	filp->f_pos = codecRXLoc;
	isTXFull  =0;
	isTXEmpty =1;
	codecTXLoc =0;
	codecWriteLoc =0;

	for ( i=0; i<BUFFER_SIZE; ++i )
		codecRXBuf[i] = codecTXBuf[i] = BLANK_CODE;
	
	//printk("<1>We are to start RX/TX now.\n");
	
	//  start RX/TX tendamly
	//IO_SYSCON = IO_SYSCON | (CDENTX | CDENRX);
	clps_writel(clps_readl(SYSCON1) | (CDENTX | CDENRX), SYSCON1);

	//printk("<1>Wo've got here.\n");
	
	//  fill in TX FIFO with blank
	for ( i=0; i<FIFO_SIZE; ++i )
		//IO_CODR = BLANK_CODE;
		clps_writeb(BLANK_CODE,CODR);
	
	//printk("<1>Blank code full fiiled into FIFO.\n");
	
	MOD_INC_USE_COUNT;
	//printk("<1>Codec opened.\n");
	return 0;          /* success */
}

int codecRelease (struct inode *inode, struct file *filp)
{
	//  stop RX/TX
	//IO_SYSCON = IO_SYSCON & ~(CDENTX | CDENRX);
	clps_writel(clps_readl(SYSCON1) & ~(CDENTX | CDENRX), SYSCON1);

	MOD_DEC_USE_COUNT;
	return 0;
}

/* Data management: read/write */
ssize_t codecRead (struct file* filp,char* buf,size_t count,loff_t* p )
{
	int loc = (int)filp->f_pos;
	int size = 0;
	int totalSize =0;
		
	while ( count ) {
		if ( loc == codecRXLoc ) {	//  no new code to read
			if ( filp->f_flags & O_NONBLOCK ) {	// non-block mode
				return -EAGAIN;
			}
			interruptible_sleep_on(&mwq);	//  wait for code
			if ( loc == codecRXLoc )  	//  wake up by other
				break;
		}
		if ( loc > codecRXLoc ) {	//  read behide write
			size = min(count,BUFFER_SIZE-loc);
			copy_to_user(buf+totalSize,codecRXBuf+loc,size);
			loc += size;
			if ( loc >= BUFFER_SIZE )
				loc = 0;
			filp->f_pos = loc;
			count -= size;
			totalSize += size;
		}
		if ( count ) {
			size = min(count,codecRXLoc-loc);
			copy_to_user(buf+totalSize,codecRXBuf+loc,size);
			loc += size;
			filp->f_pos = loc;
			count -= size;
			totalSize += size;
		} 
	} 

    	return totalSize;
}

ssize_t codecWrite (struct file* filp,const char* buf,size_t count,loff_t* p )
{
	int size = 0;
	int totalSize=0;
	
	//printk("<1>Writing to codec.\n");
	
	while ( count ) {
		if ( isTXFull ) {
			if ( filp->f_flags & O_NONBLOCK ) {	// non-block mode
				return -EAGAIN;
			}
			interruptible_sleep_on(&mwq);	//  wait for space
			if ( isTXFull )  	//  wake up by other
				break;
		}
		if ( codecWriteLoc >= codecTXLoc ) {
			size = min(count,BUFFER_SIZE-codecWriteLoc);
			copy_from_user(codecTXBuf+codecWriteLoc,buf+totalSize,size);
			//printk("<1>1copy to 0x%x while txloc=%x.\n",
			//	codecWriteLoc,codecTXLoc);
			isTXEmpty =0;
			codecWriteLoc += size;
			if ( codecWriteLoc >= BUFFER_SIZE )
				codecWriteLoc = 0;
			count -= size;
			totalSize += size;
		}
		if ( count ) {
			size = min(count,codecTXLoc-codecWriteLoc);
			copy_from_user(codecTXBuf+codecWriteLoc,buf+totalSize,size);
			//printk("<2>1copy to 0x%x while txloc=%x.\n",
			//	codecWriteLoc,codecTXLoc);
			isTXEmpty =0;
			codecWriteLoc += size;
			if ( codecWriteLoc == codecTXLoc ) {
				isTXFull =1;
			}
			count -=size;
			totalSize += size;
		} 
	} 
	
    	return totalSize;
}

unsigned int codecPoll (struct file *filp, struct poll_table_struct *table)
{
	if ( filp->f_pos != codecRXLoc ) 	//  readable
		return 1;
	poll_wait(filp, &mwq, table);
	return 0;
}

int codecIoctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	if ( cmd == 0x73880599 ) {
		isTXEmpty =1;
		codecTXLoc = codecWriteLoc;
		return 0;
	}
	return -EINVAL;
}