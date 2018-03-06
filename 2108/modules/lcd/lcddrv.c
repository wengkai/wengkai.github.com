#include "../include/hardware.h"
#include <linux/delay.h>
#include <linux/sched.h>

#include "lcd.h"

//  LCD control pins at PD
#define LCD_RS	0x02
#define LCD_RW	0x04
#define LCD_EN	0x08

static inline void setEnable(int b);
static inline void setRS(int b);
static inline void setRW(int b);
static void sendData(int RS,int data,int isWait);
static inline void sendByte(int RS,int data);
static unsigned char readData();
static unsigned char readByte();

void lcdInit()
{
	char msg[] = "Welcome!";
	int i;
	char ch;

	//  used pins in PD is output
	//IO_PDDDR = IO_PDDDR & 0xe1;
	clps_writeb(clps_readb(PDDDR)&0xe1,PDDDR);
	//  all pins in PB is output
	//IO_PBDDR = 0xff;
	clps_writeb(0xff,PBDDR);

	setEnable(0);
	setRW(1);
	setRS(1);
	
	//  test if inited
	//sendByte(0,0x80);
	sendByte(1,0x5a);
	//sendByte(0,0x80);
	ch = readByte();
	printk("<1>ch=0x%x\n",ch);
	if ( (ch&0x0f) != 0x00 ) {
		current->state = TASK_INTERRUPTIBLE;
		schedule_timeout(50*HZ/1000);
		sendData(0,0x20,0);
	}
	
	sendData(0,0x20,0);
	sendData(0,0x80,0);
	current->state = TASK_INTERRUPTIBLE;
	schedule_timeout(6*HZ/1000);

	//  display on
	sendByte(0,0x0f);
	//  display clear
	sendByte(0,0x01);
	//  entry mode set
	sendByte(0,0x06);
	
	// the 1st line
	for ( i=0; i<8; ++i ) {
		sendByte(1,msg[i]);
	}

	// the 2nd line	
	sendByte(0,0xc0);	//  set cursor
	sendByte(1,'>');
}

void lcdClear()
{
	sendByte(0,0x01);
}

void lcdHome()
{
	sendByte(0,0x02);
}

void lcdMode(int mode)
{
	sendByte(0,(mode&0x07)|0x08);
}

void lcdCursor(int cursor)
{
	sendByte(0,(cursor&0x7f)|0x80);
}

void lcdPutcmd(char cmd)
{
	sendByte(0,cmd);
}

void lcdPutchar(char ch)
{
	sendByte(1,ch);
}

void lcdWrite(const char* text,int n)
{
	int i;
	
	for ( i=0; i<n; ++i )
		sendByte(1,text[i]); 
}

void lcdLight(int onoff)
{
	//IO_PDDDR = IO_PDDDR & ~(LCD_LIGHT);	//  PDDDR:'0'-->output
	clps_writeb(clps_readb(PDDDR) & ~(LCD_LIGHT), PDDDR);
	if ( onoff ) {	//  on
		//IO_PDDR = IO_PDDR & ~LCD_LIGHT;
		clps_writeb(clps_readb(PDDR) & ~LCD_LIGHT, PDDR);
	} else {	//  off
		//IO_PDDR = IO_PDDR | LCD_LIGHT;
		clps_writeb(clps_readb(PDDR) | LCD_LIGHT, PDDR);
	}
}

//  Inner functions
//  -----------------------------------------------------------------------

void setEnable(int b)
{
	if ( b )
		//IO_PDDR = IO_PDDR | LCD_EN;
		clps_writeb(clps_readb(PDDR)|LCD_EN,PDDR);
	else
		//IO_PDDR = IO_PDDR & ~LCD_EN;
		clps_writeb(clps_readb(PDDR)&~LCD_EN,PDDR);
}

void setRS(int b)
{
	if ( b )
		//IO_PDDR = IO_PDDR | LCD_RS;
		clps_writeb(clps_readb(PDDR)|LCD_RS,PDDR);
	else
		//IO_PDDR = IO_PDDR & ~LCD_RS;
		clps_writeb(clps_readb(PDDR)&~LCD_RS,PDDR);
}

void setRW(int b)
{
	if ( b )
		//IO_PDDR = IO_PDDR | LCD_RW;
		clps_writeb(clps_readb(PDDR)|LCD_RW,PDDR);
	else
		//IO_PDDR = IO_PDDR & ~LCD_RW;
		clps_writeb(clps_readb(PDDR)&~LCD_RW,PDDR);
}

void sendData(int RS,int data,int isWait)
{
	if ( isWait ) {
		//IO_PBDDR = 0;			//  now read from PB
		clps_writeb(0,PBDDR);
		setRS(0);
		setRW(1);
		setEnable(1);
		//while ( IO_PBDR & 0x80 ) {	//  wait until DB7 low
		while ( clps_readb(PBDR) & 0x80 ) {
			udelay(40);
		}
		setEnable(0);
		setRW(0);
		//IO_PBDDR = 0xff;		//  set PB to output agn
		clps_writeb(0xff,PBDDR);
	}
	
	setRS(RS);
	//IO_PBDR = data;
	clps_writeb(data,PBDR);
	setRW(0);
	setEnable(1);				//  pull up LCD_EN
	current->state = TASK_INTERRUPTIBLE;	//  wait 6ms
	schedule_timeout(6*HZ/1000);
	setEnable(0);				//  pull LCD_EN down
	udelay(40);				//  friendly delay 40us
}

void sendByte(int RS,int data)
{
	sendData(RS,data&0xf0,1);		//  1st the high 4-bit
	sendData(RS,(data&0x0f)<<4,1);		//  then the lower
}

unsigned char readData()
{
	unsigned char ch;
	setRS(1);
	setRW(1);
	clps_writeb(0,PBDDR);
	setEnable(1);				//  pull up LCD_EN
	udelay(45);
	ch = clps_readb(PBDR);
	setEnable(0);
	setRW(0);
	clps_writeb(0xff,PBDDR);
	return ch;
}

unsigned char readByte()
{
	unsigned char ch;
	ch = readData() & 0xf0;		//  1st the high 4-bit
	//printk("<1>ch1=0x%x,",ch);
	ch |= (readData()>>4);		//  then the lower
	return ch;
}