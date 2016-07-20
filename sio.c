#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "surfDriver_ioctl.h"

int main(int argc, char **argv) {
    struct surfDriverRead req;
    int sfd;
    unsigned char cmd;

    // screw interrupts
    sfd = open("/dev/surf0", O_RDWR | O_NONBLOCK);
    if (!sfd) {
	perror("open");
	exit(1);
    }
    argc--;
    argv++;
    if (!argc) exit(1);
    cmd = **argv;
    
    argc--;
    argv++;
    if (!argc) exit(1);
    if (cmd == 'r') {
      	req.address=strtoul(*argv, NULL, 0);
	req.value = 0x00000000;
	
	if (ioctl(sfd, SURF_IOCREAD, &req)) {
	    perror("ioctl");
	    close(sfd);
	    exit(1);
	}
    } else if (cmd == 'w') {
	req.address = strtoul(*argv, NULL, 0);
	argc--;
	argv++;
	if (!argc) exit(1);
	req.value = strtoul(*argv, NULL, 0);
	if (ioctl(sfd, SURF_IOCWRITE, &req)) {
	    perror("ioctl");
	    close(sfd);
	    exit(1);
	}
	req.value = 0x0;
	if (ioctl(sfd, SURF_IOCREAD, &req)) {
	    perror("ioctl");
	    close(sfd);
	    exit(1);
	}
    }
    printf("0x%8.8X: 0x%8.8X\n", req.address, req.value);
    close(sfd);
    exit(0);
}


    
