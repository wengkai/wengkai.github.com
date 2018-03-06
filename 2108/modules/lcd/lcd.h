
/*
 * lcd.h -- definitions for the LCD char module
 *
 *********/

#ifndef _LCD_HEAD_
#define _LCD_HEAD_

#define LCD_MAJOR 127

#define LCD_LIGHT (1<<4)

void lcdInit();

void lcdClear();
void lcdHome();
void lcdMode(int mode);
void lcdCursor(int cursor);
void lcdPutcmd(char cmd);
void lcdPutchar(char ch);
void lcdWrite(const char* text,int n);
void lcdLight(int onoff);

#endif