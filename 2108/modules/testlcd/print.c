#include <stdio.h>
#include <stdlib.h>

#define	LCD_DEV	"/dev/lcd"

/*key range from ' ' to ' '+96 */
FILE*	fp;
char*	text="void";

void lcd_reset( FILE* fp )
{
		fputc('\x1b',fp);
		fputc('\x01',fp);
}

void lcd_home( FILE* fp )
{
		fputc('\x1b',fp);
		fputc('\x02',fp);
}

void lcd_cursor_hide( FILE* fp )
{
		fputc('\x1b',fp);
		fputc('\x0c',fp);
}

void lcd_cursor_flash( FILE* fp )
{
		fputc('\x1b',fp);
		fputc('\x0d',fp);
}

void lcd_cursor_on( FILE* fp )
{
		fputc('\x1b',fp);
		fputc('\x0e',fp);
}

void lcd_goto( int x, int y, FILE* fp )
{
	int	c,d;

	c=(x==0?0x80:0xc0);
	d=y&0x0f;
	fputc('\x1b',fp);
	fputc(c|d,fp);
}

void lcd_puts(  char* str, FILE* fp )
{
	int i;
	for ( i=0;i<strlen(str);i++ ) 
			fputc(str[i],fp);
}

int main( int argc, char* argv[] )
{
		int i;
	fp = fopen( LCD_DEV, "w" );
	if ( !fp ) {
		perror("fopen");
		return 1;
	}

	/* reset, cursor to home */
	lcd_reset(fp);

	lcd_puts(text,fp);

	lcd_goto(1,2,fp);

	lcd_puts(text,fp);
	lcd_cursor_flash(fp);

	lcd_goto(0,10,fp);
	lcd_home(fp);

	lcd_goto(1,12,fp);
	lcd_home(fp);
			/*
	for ( i = ' '; i < ' '+96; i++ )
			fputc(i,fp);
			*/
	/* cursor to home */
	fclose(fp);
}
