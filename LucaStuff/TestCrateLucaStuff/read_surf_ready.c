#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include "surfDriver_ioctl.h"

#define TIMEOUT (100)

int main(int argc, char **argv) {
    struct surfDriverRead req;
    int sfd;
    unsigned char cmd;
    char filename[10];
    unsigned int Ready;
    
    int time=0;
    strcpy (filename,"/dev/");
    strcat(filename, argv[1]);


    // screw interrupts
    sfd = open(filename, O_RDWR | O_NONBLOCK);
    if (!sfd) {
	perror("open");
	exit(1);
    }
    do {
      	req.address=4;
	req.value = 0x00000000;
	
	if (ioctl(sfd, SURF_IOCREAD, &req)) {
	    perror("ioctl");
	    close(sfd);
	    exit(1);
	}
	Ready=req.value & 1;
	if(time>0) system("sleep 0.1");
	if((time%10)==0)printf("%x ", Ready);
	time+=1;
    } while(!Ready && time<TIMEOUT);
    close(sfd);
    exit(0);
}


    
