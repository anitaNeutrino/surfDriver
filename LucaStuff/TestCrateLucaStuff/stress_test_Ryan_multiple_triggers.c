#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <stdint.h>

#include "surfDriver_ioctl.h"
#include "turfioDriver_ioctl.h"

#define TURF_EVENT_DATA_SIZE 256
#define SURF_EVENT_DATA_SIZE 1172

#define TIMEOUT (100)

int main(int argc, char **argv) {
    struct surfDriverRead req;
    int sfd;
    int tiofd;
    unsigned char cmd;
    char filename[10];
    unsigned int Ready;
    int i;   
    int int_time=0;
    long u_time=0;
    long u_time_sec=0;
    struct timespec tp;
    int retVal = 0;
    int count = 0;

    unsigned char turfBuf[TURF_EVENT_DATA_SIZE];
    uint32_t eventBuf[SURF_EVENT_DATA_SIZE];


    strcpy (filename,"/dev/");
    strcat(filename, argv[1]);


    // screw interrupts
    sfd = open(filename, O_RDWR | O_NONBLOCK);
    tiofd = open("/dev/turfio", O_RDWR | O_NONBLOCK);
    if (!sfd) {
	perror("open");
	exit(1);
    }
    if (!tiofd) {
	perror("open");
	exit(1);
    }

/*
./tio_noprint w 8 1 
./read_surf_ready 09:09
./sio_generic_noprint 09:09 w 6 2 
./sio_generic_noprint 09:09 w 6 2 
./tio_noprint w 0xc 2 
./tio_noprint w 0xc 2 
./tio_printdec r 11 
./sio_generic_printdec 09:09 r 5
*/

for(i=0;i<10000;i++){
// tio w 8 1
	req.address = 8;
	req.value = 1;
	if (ioctl(tiofd, TURFIO_IOCWRITE, &req)) {
	    perror("ioctl");
	    close(tiofd);
	    exit(1);
	}

// tio w 8 1 - two triggers!
	req.address = 8;
	req.value = 1;
	if (ioctl(tiofd, TURFIO_IOCWRITE, &req)) {
	    perror("ioctl");
	    close(tiofd);
	    exit(1);
	}

// tio w 8 1 - three triggers!
	req.address = 8;
	req.value = 1;
	if (ioctl(tiofd, TURFIO_IOCWRITE, &req)) {
	    perror("ioctl");
	    close(tiofd);
	    exit(1);
	}


// wait for ready
	int_time = 0;
	clock_gettime(CLOCK_REALTIME, &tp);
	u_time=tp.tv_nsec;
	u_time_sec=tp.tv_sec;
//	printf("Time start: %lu\n", u_time);
    do {
      	req.address=4;
	req.value = 0x00000000;
	
	if (ioctl(sfd, SURF_IOCREAD, &req)) {
	    perror("ioctl");
	    close(sfd);
	    exit(1);
	}
	Ready=req.value & 1;
//	if(int_time>0) system("sleep 0.01");
//	if((int_time%10)==1)printf("	Not ready until: %d\n", int_time);
	int_time+=1;
    } while(!Ready && int_time<TIMEOUT);



//First Readout
    retVal=ioctl(sfd,SURF_IOCDATA);
   count = read(sfd, eventBuf, SURF_EVENT_DATA_SIZE*sizeof(uint32_t));
  if (count < 0) {
    fprintf(stderr,"Error reading TURF data from TURFIO ");
  }
  count = read(tiofd, turfBuf, TURF_EVENT_DATA_SIZE*sizeof(char));
  if (count < 0) {
    fprintf(stderr,"Error reading TURF data from TURFIO ");
  }

//Second Readout
    retVal=ioctl(sfd,SURF_IOCDATA);
   count = read(sfd, eventBuf, SURF_EVENT_DATA_SIZE*sizeof(uint32_t));
  if (count < 0) {
    fprintf(stderr,"Error reading TURF data from TURFIO ");
  }
  count = read(tiofd, turfBuf, TURF_EVENT_DATA_SIZE*sizeof(char));
  if (count < 0) {
    fprintf(stderr,"Error reading TURF data from TURFIO ");
  }

// sio w 6 2
//	req.address = 6;
//	req.value = 2;
//	if (ioctl(sfd, SURF_IOCWRITE, &req)) {
//	    perror("ioctl");
//	    close(sfd);
//	    exit(1);
//	}

// sio w 6 2
//	req.address = 6;
//	req.value = 2;
//	if (ioctl(sfd, SURF_IOCWRITE, &req)) {
//	    perror("ioctl");
//	    close(sfd);
//	    exit(1);
//	}

// tio w 0xC 2
	req.address = 12;
	req.value = 2;
	if (ioctl(tiofd, TURFIO_IOCWRITE, &req)) {
	    perror("ioctl");
	    close(tiofd);
	    exit(1);
	}

// tio w 0xC 2
	req.address = 12;
	req.value = 2;
	if (ioctl(tiofd, TURFIO_IOCWRITE, &req)) {
	    perror("ioctl");
	    close(tiofd);
	    exit(1);
	}
	clock_gettime(CLOCK_REALTIME, &tp);
	/*if(tp.tv_nsec> u_time)
		u_time=tp.tv_nsec-u_time;
	else
		u_time=u_time- tp.tv_nsec;*/
	u_time_sec=tp.tv_sec-u_time_sec;
	u_time = 1000000000 * u_time_sec + tp.tv_nsec-u_time;
// tio r 11
      	req.address=11;
	req.value = 0x00000000;
	
	if (ioctl(tiofd, TURFIO_IOCREAD, &req)) {
	    perror("ioctl");
	    close(tiofd);
	    exit(1);
	}
	printf("Iteration %d : %d - %d at time %d - u time: %ld %ld\n", i, req.value, Ready, int_time, u_time_sec, u_time);
}
    close(sfd);
    exit(0);
}


    




    
    
