/*
 * module.c -- the LCD module for IPPhone
 *
 * by Eric Kai Weng 2001.2.19
 * last modified    2001.4.4
 * wengkai@cs.zju.edu.cn
 *********/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>     /* everything... */
#include <linux/types.h>  /* size_t */

#include "../include/hardware.h"

#include "lcd.h"

//  major number of the char. device
int lcd_major = LCD_MAJOR;

//  file operation functions
int lcd_open (struct inode *inode, struct file *filp);
int lcd_release (struct inode *inode, struct file *filp);
ssize_t lcd_write (struct file *filp, const char *buf, size_t count, loff_t * p);
int lcd_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);

struct file_operations lcd_fops = {
    NULL,
    NULL,
    NULL,
    lcd_write,
    NULL,          /* readdir */
    NULL,          /* select */
    lcd_ioctl,	   /*lcd_ioctl,*/
    NULL,          /* mmap */
    lcd_open,
    NULL,
    lcd_release,
                   /* nothing more, fill with NULLs */
};

int init_module(void)
{
	int result;

	/* Register your major, and accept a dynamic number */
	result = register_chrdev(lcd_major, "lcd", &lcd_fops);
	if (result < 0) {
        	printk(KERN_WARNING "lcd: can't get major %d\n",lcd_major);
        	return result;
	}
	if (lcd_major == 0)		/* dynamic */
    		lcd_major = result; 
    	
	lcdInit();
	lcdLight(0);

	return 0;
}

void cleanup_module(void)
{
	lcdLight(0);
	unregister_chrdev(lcd_major, "lcd");
}

//  Char device driver functions
//  ----------------------------------------------------------------------

/* Open and close */
int lcd_open (struct inode *inode, struct file *filp)
{
	//IO_PBDDR = 0xff;
	clps_writeb(0xff,PBDDR);
	MOD_INC_USE_COUNT;

	return 0;          /* success */
}

int lcd_release (struct inode *inode, struct file *filp)
{
	MOD_DEC_USE_COUNT;
	
	return 0;
}

/* Data management: write */
ssize_t lcd_write (struct file *filp, const char *buf, size_t count, loff_t * p)
{
	static char buffer[32] = {
		32,32,32,32,32,32,32,32,
		32,32,32,32,32,32,32,32,
		32,32,32,32,32,32,32,32,
		32,32,32,32,32,32,32,32,
	};
	static int pos =17;	//  We display "Hello!\n>" in lcdInit
				//  So the 1st pos user can disply is here
	int i,j;
	char cmd;
	
	for ( i=0; i<count; ++i ) {
		if ( buf[i] == 27 ) {  		//  Esc
			if ( i == count-1 )	//  Invalid data
				break;
			cmd = buf[++i];
			lcdPutcmd(cmd);
			if ( cmd == 0x01 ) {		//  clear
				for ( j=0; j<16; ++j )
					buffer[j] = ' ';
				pos = 0;
			} else if ( cmd == 0x02 ) {	//  home
				pos = 0;
			} else if ( cmd & 0x80 ) {	//  set cursor
				pos = cmd & 0x0f;
				if ( cmd & 0xC0 )	//  second line
					pos += 16;
			}
		} else if ( buf[i] == 10 ) {  		//  return
			if ( pos > 16 ) {
				//  scroll up one line
				for ( j=0; j<16; ++j )
					buffer[j] = buffer[j+16];
				for ( j=16; j<32; ++j )	//  clear 2nd line buffer
					buffer[j] = ' ';
				lcdClear();		//  clear the screen
				lcdWrite(buffer,16);	//  re-write the new 1st line
			}
			pos = 16;
			lcdCursor(0xC0);		//  now write from the 2nd line
		} else {				//  common char. to be displayed 
			buffer[pos++] = buf[i];
			lcdPutchar(buf[i]);
			if ( pos == 16 ) {		//  new line
				lcdCursor(0xC0);
			} else if ( pos == 32 ) {
				//  scroll up one line
				for ( j=0; j<16; ++j )
					buffer[j] = buffer[j+16];
				for ( j=16; j<32; ++j )
					buffer[j] = ' ';
				lcdClear();
				lcdWrite(buffer,16);
				pos = 16;
				lcdCursor(0xC0);
			}
		}
	}
    
	return count;
}

int lcd_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
{
	if ( cmd == 0x21400599 ) {
		lcdLight(arg);
		return 0;
	}
	return -EINVAL;
}