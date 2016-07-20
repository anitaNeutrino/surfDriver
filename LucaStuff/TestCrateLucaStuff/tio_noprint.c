#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "turfioDriver_ioctl.h"

int main(int argc, char **argv) {
    struct turfioDriverRead req;
    int tiofd;
    unsigned char cmd;

    // screw interrupts
    tiofd = open("/dev/turfio", O_RDWR | O_NONBLOCK);
    if (!tiofd) {
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
	
	if (ioctl(tiofd, TURFIO_IOCREAD, &req)) {
	    perror("ioctl");
	    close(tiofd);
	    exit(1);
	}
    } else if (cmd == 'w') {
	req.address = strtoul(*argv, NULL, 0);
	argc--;
	argv++;
	if (!argc) exit(1);
	req.value = strtoul(*argv, NULL, 0);
	if (ioctl(tiofd, TURFIO_IOCWRITE, &req)) {
	    perror("ioctl");
	    close(tiofd);
	    exit(1);
	}
	req.value = 0x0;
	if (ioctl(tiofd, TURFIO_IOCREAD, &req)) {
	    perror("ioctl");
	    close(tiofd);
	    exit(1);
	}
    }
    close(tiofd);
    exit(0);
}


    
