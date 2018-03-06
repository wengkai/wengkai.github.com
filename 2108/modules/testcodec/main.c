#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>

main(int argc, char* argv[])
{
	char* buf;
	int i;
	int fi,fo;
	
	buf = (char*)malloc(32*1024);
	for ( i=0; i<32*1024; ++i )
		buf[i] = 0x00;
	fi = open(argv[1],O_RDONLY);
	read(fi,buf,32768);
	close(fi);
	
	fo = open("/dev/codec",O_WRONLY);
	//for ( i=0; i<1024; ++i )
	write(fo,buf,32768);

	getchar();		
	close(fo);
	free(buf);
	
	return 0;
}