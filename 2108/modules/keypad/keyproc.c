#include <linux/kernel.h>
#include <linux/malloc.h>

#include "../include/hardware.h"
#include "keypad.h"

//#define _LINUX_2_2_1_

//  save the scancode of the key pressed to be check in keypadSchedule()
static unsigned int pressedKey;

//  buffer to save the pressed key queue
char* keypadBuffer =0;
int keypadBufLoc;

static void keypadNewKey(unsigned int scanCode);
static void keypadSchedule();
static void keypadAddTimer();

static void keypadAddBuffer(char ch);

int keypadInit()
{
	pressedKey = 0;
	keypadBuffer = NULL;
	keypadBufLoc = 0;
	
	keypadBuffer = (char*)kmalloc(BUFFER_SIZE,GFP_KERNEL);
	if ( keypadBuffer == NULL ) {
		printk(KERN_WARNING "Keypad: Not enough memory.\n");
		return -1;
	}
	
	//  All column high to wait a key interrupt
	//IO_SYSCON = (IO_SYSCON & 0xfffffff0);	
	clps_writel(clps_readl(SYSCON1) & 0xfffffff0, SYSCON1);
	
	return 0;
}

void keypadClose()
{
	if ( keypadBuffer != NULL ) {
		kfree(keypadBuffer);
		keypadBuffer = NULL;
	}
}

//  functions
//  -----------------------------------------------------------------------

void keypadPressed(unsigned int scanCode)
{
	//printk("<1>key 0x%x pressed\n",scanCode);
	if ( scanCode != pressedKey ) {
		pressedKey = scanCode;
		keypadNewKey(scanCode);	//  get keycode and put it into buffer
		keypadAddTimer();	//  a timer to check if the key released
	}
}

void keypadNewKey(unsigned int scanCode)
{
	struct KeyTable {
		unsigned scanCode;
		char keyCode;
	};
	static struct KeyTable keyTable[NR_KEYS] = {
#ifdef _LINUX_2_2_1_
		{0x21,'*'},
		{0x22,'7'},
		{0x23,'4'},
		{0x24,'1'},
		{0x25,'L'},
		{0x26,'U'},
		{0x31,'0'},
		{0x32,'8'},
		{0x33,'5'},
		{0x34,'2'},
		{0x35,'D'},
		{0x36,'R'},
		{0x51,'#'},
		{0x52,'9'},
		{0x53,'6'},
		{0x54,'3'},
		{0x55,'C'},
		{0x56,'O'},	
#else		
		{0x16,'*'},
		{0x15,'7'},
		{0x14,'4'},
		{0x13,'1'},
		{0x12,'L'},
		{0x11,'U'},
		{0x26,'0'},
		{0x25,'8'},
		{0x24,'5'},
		{0x23,'2'},
		{0x22,'D'},
		{0x21,'R'},
		{0x36,'#'},
		{0x35,'9'},
		{0x34,'6'},
		{0x33,'3'},
		{0x32,'C'},
		{0x31,'O'},
#endif
	};
	int i;
	//printk("<1>new key 0x%x\n",scanCode);
	for ( i = 0; i<NR_KEYS; ++i ) {
		if ( scanCode == keyTable[i].scanCode ) {
			keypadAddBuffer(keyTable[i].keyCode);
		}
	} 
}

//  timer functions
//  -----------------------------------------------------------------------

//  timer routine
void keypadSchedule()
{
	if ( pressedKey == 0 )	//  was cleared somewhere
		return;

	//  disable kbdint temp.
	//IO_INTMR2 = IO_INTMR2 & 0xfffe;
	clps_writeb(clps_readb(INTMR2) & 0xfffe, INTMR2);
		
	//  check if the key released
	//IO_SYSCON = (IO_SYSCON & 0xfffffff0) | (((pressedKey & 0xf0)>>4)+7);
	clps_writel( (clps_readl(SYSCON1) & 0xfffffff0) | 
		(((pressedKey & 0xf0)>>4)+7), SYSCON1);
	//if ( IO_PADR ) {
	if ( clps_readb(PADR) ) {
		unsigned char mask = 1 << ((pressedKey&0x0f)-1);
		//if ( IO_PADR & mask ) {	//  still pressed
		if ( clps_readb(PADR) & mask ) {
			//  re-queue
			keypadAddTimer();
			return;
		} else
			pressedKey = 0;
	} else
		pressedKey = 0;
	
	//  re-charge COLs
	//IO_SYSCON = IO_SYSCON & 0xfffffff0;
	clps_writel(clps_readl(SYSCON1) & 0xfffffff0, SYSCON1);
	//  clear KBD interrupt
	//IO_KBDEOI =  0x0;
	clps_writeb(0x0,KBDEOI);
	//  enable KBD interrupt
	//IO_INTMR2 = (IO_INTMR2 & 0xfffe) | 0x1;
	clps_writeb((clps_readb(INTMR2) & 0xfffe) | 0x1, INTMR2);
}

void keypadAddTimer()
{
	static struct timer_list keypadTimer;
	
	init_timer(&keypadTimer);
	keypadTimer.function = &keypadSchedule;
	keypadTimer.expires = RELEASE_DELAY;	
	add_timer(&keypadTimer);
}

//  buffer functions
//  ------------------------------------------------------------------------
void keypadAddBuffer(char ch)
{
	//printk("<1>Key %c inputed.\n",ch);
	keypadBuffer[keypadBufLoc++] = ch;
	if ( keypadBufLoc == BUFFER_SIZE )	//  wrap back
		keypadBufLoc = 0;
	wake_up_interruptible(&mwq);		//  wake up waited read() thread
}

