#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "surfDriver_ioctl.h"

int main(int argc, char **argv) {
    struct surfDriverRead req;
    int sfd;
    unsigned char cmd;
    short val;

    // screw interrupts
    sfd = open("/dev/surf0", O_RDWR | O_NONBLOCK);
    if (!sfd) {
	perror("open");
	exit(1);
    }

	if (ioctl(sfd, SURF_IOCGETSLOT, &val)) {
	    perror("ioctl");
	    close(sfd);
	    exit(1);
	}
    printf("0x%8.8X\n", val);
    close(sfd);
    exit(0);
}


    
