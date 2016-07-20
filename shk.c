#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "surfDriver_ioctl.h"

unsigned char *scaler_labels[32] = 
{
   "  T0V",
   "  M0V",
   "  B0V",
   "  T1V",
   "  M1V",
   "  B1V",
   "  T0H",
   "  M0H",
   "  B0H",
   "  T1H",
   "  M1H",
   "  B1H",
   "L1 0V",
   "L1 1V",
   "L1 0H",
   "L1 1H",
   "M T0V",
   "M M0V",
   "M B0V",
   "M T1V",
   "M M1V",
   "M B1V",
   "M T0H",
   "M M0H",
   "M B0H",
   "M T1H",
   "M M1H",
   "M B1H",
   "ML10V",
   "ML11V",
   "ML10H",
   "ML11H"
};
   
int main(int argc, char **argv) {
   struct surfHousekeepingRequest req;
   unsigned int buffer[96];
   int sfd;
   int i;
    // screw interrupts
   sfd = open("/dev/surf0", O_RDWR | O_NONBLOCK);
   if (!sfd) {
      perror("open");
      exit(1);
   }
   req.length = sizeof(unsigned int)*96;
   req.address = buffer;
   if (ioctl(sfd, SURF_IOCHKREAD, &req)) 
     {
	perror("ioctl");
	close(sfd);
	exit(1);
     }
   printf("SURF Housekeeping Read: Header %4.4x, Ref Counts: %5d\n", (buffer[0] >> 16) & 0xFFFF, (buffer[16]>>16) & 0xFFFF);
   for (i=0;i<32;i++) 
     {
	printf("%s: %5d", scaler_labels[i], (buffer[i] & 0xFFFF));
	if (!((i+1)%8)) printf("\n");
	else printf(" ");
     }
   close(sfd);
   return 0;
}


    
