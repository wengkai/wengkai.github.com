#include <linux/delay.h>
#include <linux/sched.h>
#include <asm/arch/irqs.h>
#include <linux/interrupt.h>
#include <linux/tqueue.h>

#include "../include/hardware.h"
#include "keypad.h"

static struct tq_struct keypad_task; /* 0 by now, filled by init_module code */

int keypad_bh_key = 0;

void keypad_bottom_half(void* unused)
{
	unsigned int col;
	unsigned int padr;
	unsigned int row = 1;
	
	//printk("<1>In bottom half\n");
	// disable keypad interrupt
	clps_writeb(clps_readb(INTMR2) & 0xfffe, INTMR2);
	
	for ( col=8; col<11; ++col ) {
		clps_writel((clps_readl(SYSCON1) & 0xfffffff0) | col, SYSCON1);
		udelay(5);	//  important! wait for stable
		padr = clps_readb(PADR);
		if ( padr ) {	//  there is a key pressed,find out which row
			//  disable the next interrupt
			clps_writel((clps_readl(SYSCON1) & 0xfffffff0) | 0x1,
				SYSCON1);
			while ( (padr >>=1) >0 )
				++ row;
			keypadPressed(((col-7)<<4)|row);
			break;
		}
	}
	
	if ( col == 11 ) {
		//  re-charge COLs
		clps_writel(clps_readl(SYSCON1) & 0xfffffff0,SYSCON1);
	}

	// clear interrupts
	clps_writel(0x0,KBDEOI);
	// enable keypad interrupt again
	clps_writeb((clps_readb(INTMR2) & 0xfffe) | 0x1, INTMR2);
}

void keypadInterrupt(int irq, void* dev_id, struct pt_regs* regs)
{
	// disable keypad interrupt
	clps_writeb(clps_readb(INTMR2) & 0xfffe, INTMR2);
	// discharge all columns
	clps_writel((clps_readl(SYSCON1) & 0xfffffff0)|0x01, SYSCON1);
	
	//printk("<1>Interrupted!\n");
	
	queue_task(&keypad_task, &tq_immediate);
	mark_bh(IMMEDIATE_BH);
	
	//  clear interrupt
	clps_writel(0x0,KBDEOI);
}

void initInterrupt()
{
	keypad_task.routine = keypad_bottom_half;
	keypad_task.data = NULL; /* unused */
}