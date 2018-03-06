/*
 * Copyright (c) 2000 Blue Mug, Inc.  All Rights Reserved.
 *
 * Commands to program the Intel 28F320B3 flash chips on the Cirrus
 * Logic EDB7211 evaluation board.  These are 32 Mbit "bottom boot"
 * (boot and param blocks at low end of address space) flash chips.
 *
 * The EDB7211 has two 8Mbyte flash banks, each using two flash chips.
 * The flash chips are 16 bits wide; one occupies the 16 high lines on
 * the data bus, the other the low lines.  So when we write commands
 * to the flash, we have to duplicate each command across each half of
 * the data bus:
 *
 * 	*addr = 0x00FF00FF;
 *
 * This is seen as a word-wide write of 0x00FF by each 16-bit flash
 * chip.  Such an arrangement should be familiar to embedded
 * developers who have worked with flash in 32-bit systems.
 *
 * Two of the boot blocks in each flash are write protected.  We don't
 * provide any way of circumventing that protection here.  From a
 * quick look at the EDB7211 schematics, it looks like the write
 * protect line is held at logic high, so the blocks are always
 * writeable on this board.
 *
 * Even in other configurations (e.g. WP line wired to a GPIO output)
 * I expect that I'll want to avoid automatic disabling of write
 * protect here, since the consequences of writing garbage into the
 * boot blocks can be grave.  In the infrequent case in which you're
 * writing over the boot loader itself, it's not too onerous to have
 * to fire up the command line and do a little register prodding.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include "flash_intel.h"

typedef unsigned long addr_t;
typedef unsigned long size_t;
typedef unsigned long word_t;

union errdata {
	const char *msg;
	word_t word;
} errdata;

//#define FL_WORD(addr) *((volatile unsigned short*)(addr))

/* command user interface (CUI) */
#define CUI_CHECK_ID		0x0090
#define CUI_READ_ARRAY		0x00FF
#define CUI_READ_IDENTIFIER	0x0090
#define CUI_READ_STATUS		0x0070
#define CUI_CLEAR_STATUS	0x0050
#define CUI_PROGRAM		0x0040
#define CUI_BLOCK_ERASE		0x0020
#define CUI_PROG_ERASE_SUSPEND	0x00B0
#define CUI_PROG_ERASE_RESUME	0x00D0
#define CUI_BLOCK_LOCK		0x0060
#define CUI_BLOCK_LOCK_OFF	0x00D0
#define CUI_BLOCK_LOCK_ON	0x0001

/* status register bits */
#define SR7_WSMS	(1<<7)	/* Write State Machine Status */
#define SR6_ESS		(1<<6)	/* Erase-Suspend Status */
#define SR5_ES		(1<<5)	/* Erase Status */
#define SR4_PS		(1<<4)	/* Program Status */
#define SR3_VPPS	(1<<3)	/* V_PP (program voltage) Status */
#define SR2_PSS		(1<<2)	/* Program Suspend Status */
#define SR1_BLLK	(1<<1)	/* Block Lock Status */
/* bit 0 is reserved */

#define SR_MASK (SR7_WSMS|SR6_ESS|SR5_ES|SR4_PS|SR3_VPPS|SR2_PSS|SR1_BLLK)
#define SR_ERASE_ERR (SR5_ES|SR4_PS|SR3_VPPS|SR1_BLLK)
#define SR_BOTH_MASK (SM_MASK)
#define SR_BOTH_WSMS (SR7_WSMS)
#define SR_BOTH_ERASE_ERR (SR_ERASE_ERR)

/* handy flash functions */
static void flash_report_error(unsigned char status);
static word_t flash_status_wait(addr_t addr);
static int flash_status_full_check(addr_t addr);

static void flashBusWrite(addr_t addr,unsigned short value)
{
	__asm("strh	r1, [r0, #0]");
}

static unsigned short flashBusRead(addr_t addr)
{
	__asm("ldrh	r0, [r0, #0]");
}

static void flash_report_error(unsigned char status)
{
	if (status & SR3_VPPS)
		printk("VPPRange");
	else if ((status & SR5_ES) && (status & SR4_PS))
		printk("CommandSequence");
	else if (status & SR5_ES)
		printk("BlockErase");
	else if (status & SR1_BLLK)
		printk("LockedBlock");
	else
		printk("No Error");
	printk("\n");
}

/*
 * Loop until both write state machines complete.
 */
static word_t flash_status_wait(addr_t addr)
{
	word_t status;
	do {
		//FL_WORD(addr) = CUI_READ_STATUS;
		//status = FL_WORD(addr);
		status = flashBusRead(addr);
	} while ((status & SR_BOTH_WSMS) != SR_BOTH_WSMS);
	return status;
}

/*
 * Loop until the Write State machine is ready, then do a full error
 * check.  Clear status and leave the flash in Read Array mode; return
 * 0 for no error, -1 for error.
 */
static int flash_status_full_check(addr_t addr)
{
	word_t status;

	status = flash_status_wait(addr) & SR_BOTH_ERASE_ERR;
	if (status) {
		flash_report_error(status&SR_MASK);
	}
	//FL_WORD(addr) = CUI_CLEAR_STATUS;
	//FL_WORD(addr) = CUI_READ_ARRAY;
	flashBusWrite(addr,CUI_CLEAR_STATUS);
	flashBusWrite(addr,CUI_READ_ARRAY);
	//hprintf("status=%x\n",status);
	return status ? -1 : 0;
}

int flash_check_id()
{
	unsigned short id;
	flashBusWrite(FLASH_BASE,CUI_CHECK_ID);
	id = flashBusRead(FLASH_BASE);
	flashBusWrite(FLASH_BASE,CUI_READ_ARRAY);
	if ( id == 0x89 )
		return 1;
	return 0;
}

/*
 * Program the contents of the download buffer to flash at the given
 * address.  Size is also specified; we shouldn't have to track usage
 * of the download buffer, since the host side can easily do that.
 *
 * We write words without checking status between each; we only go
 * through the full status check procedure once, when the full buffer
 * has been written.
 *
 * Alignment problems are errors here; we don't automatically correct
 * them because in the context of this command they signify bugs, and
 * we want to be extra careful when writing flash.
 */
int flash_program(addr_t addr,size_t size,short* buf)
{
	short *ptr;

	//if (addr & UNALIGNED_MASK) return -H_EALIGN;
	//if (size & UNALIGNED_MASK) return -H_EALIGN;

	size >>= 1;

	for (ptr = buf; size--; addr += sizeof(short)) {
		//hprintf("writting %x to %x\n",*ptr,addr);
		
		//FL_WORD(addr) = CUI_PROGRAM;
		//FL_WORD(addr) = *ptr++;
		flashBusWrite(addr,CUI_PROGRAM);
		flashBusWrite(addr,*ptr++);
		flash_status_wait(addr);
	}
	return flash_status_full_check(addr);
}

/*
 * Erase a flash block.  Single argument is the first address in the
 * block (actually, it can be anywhere in the block, but the host-side
 * hermit downloader gives the first address).
 */
int flash_erase(addr_t addr)
{
	addr &= ~(addr_t)3;	// word align

	//printk("<1>erasing %lx\n",addr);
	//FL_WORD(addr) = CUI_BLOCK_ERASE;
	//FL_WORD(addr) = CUI_PROG_ERASE_RESUME;
	flashBusWrite(addr,CUI_BLOCK_ERASE);
	flashBusWrite(addr,CUI_PROG_ERASE_RESUME);
	return flash_status_full_check(addr);
}

void flash_unlock(addr_t addr)
{
	int delay;

	printk("<1>Unlocking %lx\n",addr);	
	flashBusWrite(addr,CUI_READ_ARRAY);
	for ( delay = 10000; delay; --delay );
	flashBusWrite(addr,CUI_BLOCK_LOCK);
	for ( delay = 10000; delay; --delay );
	flashBusWrite(addr,CUI_BLOCK_LOCK_OFF);
	for ( delay = 10000; delay; --delay );
	flashBusWrite(addr,CUI_READ_ARRAY);
	for ( delay = 10000; delay; --delay );
	//flash_status_full_check(addr);
}

void flash_lock(addr_t addr)
{
	int delay;

	printk("<1>Locking %lx\n",addr);	
	flashBusWrite(addr,CUI_READ_ARRAY);
	for ( delay = 10000; delay; --delay );
	flashBusWrite(addr,CUI_BLOCK_LOCK);
	for ( delay = 10000; delay; --delay );
	flashBusWrite(addr,CUI_BLOCK_LOCK_ON);
	for ( delay = 10000; delay; --delay );
	flashBusWrite(addr,CUI_READ_ARRAY);
	for ( delay = 10000; delay; --delay );
	//flash_status_full_check(addr);
}