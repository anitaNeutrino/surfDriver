#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <string.h>
#include "surfDriver_ioctl.h"

   
int main(int argc, char **argv) {
   struct surfHousekeepingRequest req;
//   unsigned int buffer[96];
   uint32_t buffer[96];
   int sfd;
   int i;
   char filename[20];
    // screw interrupts
   strcpy (filename,"/dev/");
   strcat(filename, argv[1]);
   sfd = open(filename, O_RDWR | O_NONBLOCK);
//   sfd = open("/dev/04:11", O_RDWR | O_NONBLOCK);
   if (!sfd) {
      perror("open");
      exit(1);
   }
   req.length = sizeof(uint32_t)*96;
   req.address = buffer;
   if (ioctl(sfd, SURF_IOCHKREAD, &req)) 
     {
	perror("ioctl");
	close(sfd);
	exit(1);
     }
   printf("SURF Housekeeping Read: Full 96\n");
   for (i=0;i<96;i++) 
     {
	printf("%x\n",  buffer[i]);
	if (!((i+1)%32)) printf("\n");
	else printf(" ");
     }
   close(sfd);
   return 0;
}


    
