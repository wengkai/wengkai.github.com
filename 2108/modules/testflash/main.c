#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <malloc.h>

main(int argc, char* argv[])
{
	char buf[15] = "This is a test";
	char rb[15];
	int i;
	int fi,fo;
	
	buf[14] = 0;
	fo = open("/dev/flash",O_WRONLY);
	lseek(fo,64*1024,SEEK_SET);
	if ( write(fo,&(buf[0]),15) != 15 ) {
		printf("Error in write\n");
		exit(-1);
	}
	printf("written!\n");
	close(fo);
	
	fi = open("/dev/flash",O_RDONLY);
	lseek(fi,56*1024,SEEK_SET);
	if ( read(fi,&(rb[0]),15) != 15 ) {
		printf("Error in read\n");
		exit(-1);
	}
	printf("%s\n",rb);
	for ( i=0; i<15; ++i )
		printf("%02x ",rb[i]);
	printf("\n");
	close(fi);
	
	return 0;
}