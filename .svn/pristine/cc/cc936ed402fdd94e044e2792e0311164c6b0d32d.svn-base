#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include "surfDriver_ioctl.h"

unsigned char *DAC_labels[32] = 
{  

"NC",  //A0
"NC",  //A1
"NC",  //A2
"NC",  //A3
"NC",  //A4
"NC",  //A5
"NC",  //A6
"NC",  //A7
"0VT", //A8
"0HT", //A9
"NC",  //AA
"NC",  //AB
"0VM", //AC
"0HM", //AD
"0VB", //AE
"0HB", //AF
"1VT", //B0
"1HT", //B1
"NC", //B2
"NC", //B3
"1VM", //B4
"1HM", //B5
"1VB", //B6
"1HB", //B7
"NC", //B8
"NC", //B9
"NC", //BA
"NC", //BB
"NC", //BC
"NC", //BD
"NC", //BE
"NC"};//BF
 
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
   sfd = open("/dev/09:09", O_RDWR | O_NONBLOCK);
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
   printf("SURF Housekeeping Read: Header %4.4x \nRef Counts: %5d\n", (buffer[0] >> 16) & 0xFFFF, (buffer[16]>>16) & 0xFFFF);
   for (i=0;i<32;i++) 
     {
	printf("%s: %5d\n", DAC_labels[i], (buffer[i+32] & 0xFFFF));
	if (!((i+1)%8)) printf("\n");
	else printf(" ");
     }
   for (i=0;i<32;i++) 
     {
	printf("%s: %5d\n", scaler_labels[i], (buffer[i] & 0xFFFF));
	if (!((i+1)%8)) printf("\n");
	else printf(" ");
     }
   close(sfd);
   return 0;
}


    
