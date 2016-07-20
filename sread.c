#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "surfDriver_ioctl.h"

int main(int argc, char **argv) {
    struct surfDriverRead req;
    int sfd;
   unsigned char cmd;
   int err;
   unsigned int data[1172];
   int chan = 0;
   unsigned int headerWord;
    // screw interrupts
    sfd = open("/dev/surf0", O_RDWR | O_NONBLOCK);
    if (!sfd) {
	perror("open");
	exit(1);
    }
   err = ioctl(sfd, SURF_IOCEVENTREADY);
   printf("ioctl returned %d\n", err);
   err = ioctl(sfd, SURF_IOCDATA);
   err = read(sfd, data, 1172*sizeof(int)); 
   printf("First word: %8.8x\n", data[0]);
   printf("Second word: %8.8x\n", data[1]);
   headerWord = data[0];
   printf("Ryan would say:\n");
   printf("SURF 0 (%d), CHIP %d, CHN %d, Header: %x\n",
	  ((headerWord&0xf0000)>>16),((headerWord&0x00c00000)>> 22),chan,headerWord);
   close(sfd);
   return 0;
}


    
