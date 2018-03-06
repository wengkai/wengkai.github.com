#include <stdio.h>

int main()
{
	FILE* fp;
	char ca[2];
	int state = 0;
	
	printf("Hello\n");
	fp = fopen("/dev/hook","rb");
	
	while (1) {
		if ( fread(ca,1,2,fp) ) {
			if( !state ) {
				printf("off-hook\n");
				state = 1;
			}
		} else {
			if ( state ) {
				printf("on-hook\n");
				state = 0;
			}
		}
	}
	
	return 0;	
}