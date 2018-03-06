#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main() 
{
	int fd;
	int state =-1;
	int ss;
	char ch;
	
	fd = open("/dev/hook",O_RDONLY|O_NONBLOCK);
	if ( fd <0 ) {
		printf("Can't open /dev/hook.\n");
		exit(-1);
	}
	while ( 1 ) {
		ss = read(fd,&ch,1);
		if ( ss != state ) {
			printf("%d\n",ss);
			state = ss;
		}
	}	
	close(fd);
	return 0;
}
