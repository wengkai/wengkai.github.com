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
	
	buf = (char*)malloc(80);

	fi = open("/dev/codec",O_RDWR);
	//fo = open("/dev/codec",O_WRONLY);
	while ( 1 ) {
		read(fi,buf,80);
		write(fi,buf,80);
	}

	
	return 0;
}