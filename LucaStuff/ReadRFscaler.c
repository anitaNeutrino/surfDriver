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
   int scaler_no;
   char filename[20];
    // screw interrupts
   strcpy (filename,"/dev/");
   strcat(filename, argv[1]);
   scaler_no=atoi(argv[2]);

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
   //printf("RF top 16 bit ADC value: %d\n", buffer[64+8+scaler_no] & 0xffff);
//   printf("RF top 16 bit ADC value: %d\n", buffer[64+scaler_no] & 0xffff);
   printf("High value: %d\n", buffer[64+scaler_no] & 0xffff);
   close(sfd);
   return 0;
}


    
