#include <linux/kernel.h>
#include <linux/module.h>
#include <asm/uaccess.h>
#include <asm/i387.h>

#include "surfDriver.h"
#include "pci9030.h"

unsigned int *surfDriver_datatransfer_integer(struct surfDriver_dev *devp,
					      unsigned int *buffer,
					      unsigned int count);
unsigned int *surfDriver_datatransfer_quad_cl(struct surfDriver_dev *devp,
					      unsigned int *buffer,
					      unsigned int count);
unsigned int *surfDriver_datatransfer_cl(struct surfDriver_dev *devp,
					 unsigned int *buffer,
					 unsigned int count);

int surfDriver_user_datatransfer(struct surfDriver_dev *devp,
				 char __user *buff,
				 unsigned int count) 
{
   unsigned int *buffer;
   int mode;

   preempt_disable();
   buffer = kmalloc(count+256, GFP_KERNEL);
   if (!buffer) return -EFAULT;
   buffer[0] = readl(&devp->surf->buffer_status);
   buffer[1] = readl(&devp->surf->event_id);

   mode = surfDriver_readoutmode();
   if (mode == 1)
     surfDriver_datatransfer_quad_cl(devp, &buffer[2], count - 8);
   else if (mode == 2)
     surfDriver_datatransfer_integer(devp, &buffer[2], count - 8);
   else
     surfDriver_datatransfer_cl(devp, &buffer[2], count - 8);

   devp->surf->clear = SURF_CLEAR_EVT;
   if (copy_to_user(buff, buffer, count)) {
      kfree(buffer);
      preempt_enable();
      return -EFAULT;
   }
   kfree(buffer);
   preempt_enable();
   return 0;
}

// sD_dt_integer just copies the data one integer at a time.
// Actually, not really, because the CPU is 64 bit. It pulls them
// in 2 at a time. But really the point is "no movntdqa loop".
unsigned int *surfDriver_datatransfer_integer(struct surfDriver_dev *devp,
					      unsigned int *buffer,
					      unsigned int count) {
  unsigned int *p;
  unsigned int *bufp;
  int i;
  p = (unsigned int *) devp->lab;
  bufp = (unsigned int *) buffer;
  for (i=0;i<count;i+=4) {
    // Do we need to reset the window?
    if (!(i % 256)) {
      // reset the window
      unsigned int to_write;
      writel((i>>2) | (1<<31), &devp->surf->lab_counter);
      // mfence
      mb();      
      // reset pointer, too
      p = (unsigned char *) devp->lab;
    }
    *bufp = readl(p);
    bufp++;
    p++;
  }
}
  
// sD_dt_quad_cl transfers the data by using ALL of the XMM registers.
// 'quad' because we move 4 cachelines at once.
// I have no idea if this is faster or not. It might thrash the shit
// out of the PCI bridges, considering they're not the world's smartest
// devices.
unsigned int *surfDriver_datatransfer_quad_cl(struct surfDriver_dev *devp,
					      unsigned int *buffer,
					      unsigned int count) {
  int i;
  int count_lp;
  unsigned int my_cnt;
  volatile unsigned char *p;
  unsigned char *txbuf;
  unsigned char *bufp;
  // Enable bursting and BTERM#.
  devp->plx->las0brd = 
    PCI9030_BRD_PREFETCH_COUNTER |
    PCI9030_BRD_PREFETCH_16 |
    PCI9030_BRD_BURST_EN |
    PCI9030_BRD_NBTERM_EN |
    PCI9030_BRD_NREADY_EN |
    PCI9030_BRD_BUS_32;
  // Set up our pointers to the LAB space, to
  // the large buffer, and to the temporary buffer.
  bufp = buffer;
  count_lp = 0;

  while (count_lp < count)
    {
      unsigned int to_write;

      // initialize the from (p) pointer to beginning of lab space
      p = devp->lab;
      // initialize the to (txbuf) pointer to the temporary buffer
      txbuf = devp->txbuf;
      // Figure out where to set the LAB page register to.
      // Find the number of integers we've read (divide bytes by 4)
      my_cnt = count_lp >> 2;
      // Set the top bit to 1 to enable bursting
      to_write = my_cnt | (1<<31);

      // Lock the FPU. Realistically we should just do this at the
      // beginning and end, but I don't know if memcpy wants the FPU too.
      kernel_fpu_begin();
      // End Lock the FPU.
      
      // Set the page register to our target.
      writel(to_write, &devp->surf->lab_counter);
      // Barrier.
      mb();
      // Copy 4 cachelines via movntdqa.
      __asm__ __volatile__(
			     "movntdqa (%0), %%xmm0\n"
			     "movntdqa 16(%0), %%xmm1\n"
			     "movntdqa 32(%0), %%xmm2\n"
			     "movntdqa 48(%0), %%xmm3\n"
			     "movntdqa 64(%0), %%xmm4\n"
			     "movntdqa 80(%0), %%xmm5\n"
			     "movntdqa 96(%0), %%xmm6\n"
			     "movntdqa 112(%0), %%xmm7\n"
			     "movntdqa 128(%0), %%xmm8\n"
			     "movntdqa 144(%0), %%xmm9\n"
			     "movntdqa 160(%0), %%xmm10\n"
			     "movntdqa 176(%0), %%xmm11\n"
			     "movntdqa 192(%0), %%xmm12\n"
			     "movntdqa 208(%0), %%xmm13\n"
			     "movntdqa 224(%0), %%xmm14\n"
			     "movntdqa 240(%0), %%xmm15\n"	       
			     "movdqa   %%xmm0, (%1)\n"
			     "movdqa   %%xmm1, 16(%1)\n"
			     "movdqa   %%xmm2, 32(%1)\n"
			     "movdqa   %%xmm3, 48(%1)\n"
			     "movdqa   %%xmm4, 64(%1)\n"
			     "movdqa   %%xmm5, 80(%1)\n"
			     "movdqa   %%xmm6, 96(%1)\n"
			     "movdqa   %%xmm7, 112(%1)\n"
			     "movdqa   %%xmm8, 128(%1)\n"
			     "movdqa   %%xmm9, 144(%1)\n"
			     "movdqa   %%xmm10, 160(%1)\n"
			     "movdqa   %%xmm11, 176(%1)\n"
			     "movdqa   %%xmm12, 192(%1)\n"
			     "movdqa   %%xmm13, 208(%1)\n"
			     "movdqa   %%xmm14, 224(%1)\n"
			     "movdqa   %%xmm15, 240(%1)\n"
			     :
			     : "r" (p), "r" (txbuf)
			     : "memory");
      // Unlock the FPU.
      kernel_fpu_end();
      // Barrier.
      mb();
      // Copy to the full-size buffer.
      memcpy(bufp, txbuf, 256);
      // Increment the full-size buffer pointer.
      bufp += 256;
      // Increment the number of bytes read.
      count_lp += 256;
    }
  // Reset the local bus timings to single-access only.
  devp->plx->las0brd = 
     PCI9030_BRD_PREFETCH_COUNTER |
     PCI9030_BRD_NBTERM_EN |
     PCI9030_BRD_NREADY_EN |
     PCI9030_BRD_BUS_32;
  DEBUG("surfDriver_datatransfer: %8.8x %8.8x %8.8x %8.8x\n",
	buffer[0], buffer[1], buffer[2], buffer[3]);

  // Done.
  return buffer;
}

// sD_dt_cl transfers the data 1 cacheline at a time.
unsigned int *surfDriver_datatransfer_cl(struct surfDriver_dev *devp,
					 unsigned int *buffer,
					 unsigned int count) {
  int i;
  int count_lp;
  unsigned int my_cnt;
  volatile unsigned char *p;
  unsigned char *txbuf;
  unsigned char *bufp;
  // Enable bursting and BTERM#.
  devp->plx->las0brd = 
    PCI9030_BRD_PREFETCH_COUNTER |
    PCI9030_BRD_PREFETCH_16 |
    PCI9030_BRD_BURST_EN |
    PCI9030_BRD_NBTERM_EN |
    PCI9030_BRD_NREADY_EN |
    PCI9030_BRD_BUS_32;
  // Set up our pointers to the LAB space, to
  // the large buffer, and to the temporary buffer.
  bufp = buffer;
  count_lp = 0;
  while (count_lp < count)
    {
      unsigned int to_write;

      // initialize the from (p) pointer to beginning of lab space
      p = devp->lab;
      // initialize the to (txbuf) pointer to beginning of temp buffer
      txbuf = devp->txbuf;
      // Figure out where to set the LAB page register to.
      // Find the number of integers we've read (divide bytes by 4)
      my_cnt = count_lp >> 2;
      // Set the top bit to enable bursting.
      to_write = my_cnt | (1<<31);
      // Lock the FPU.
      kernel_fpu_begin();
      // the LAB window is 256 bytes, or 4 cachelines long.
      for (i=0;i<4;i++) {
	// Set the LAB page register. This doesn't have to be
	// done every cacheline read, but we do it here
	// to see if it helps makes the bridges less stupid.
	// (update: probably not, but it also doesn't hurt).
	writel(my_cnt | (1<<31), &devp->surf->lab_counter);
	// barrier
	mb();
	// Transfer a cacheline via movntdqa.
	__asm__ __volatile__(
			     "movntdqa (%0), %%xmm0\n"
			     "movntdqa 16(%0), %%xmm1\n"
			     "movntdqa 32(%0), %%xmm2\n"
			     "movntdqa 48(%0), %%xmm3\n"
			     "movdqa   %%xmm0, (%1)\n"
			     "movdqa   %%xmm1, 16(%1)\n"
			     "movdqa   %%xmm2, 32(%1)\n"
			     "movdqa   %%xmm3, 48(%1)\n"
			     :
			     : "r" (p), "r" (txbuf)
			     : "memory");
	// Increment LAB space pointer to next block of 64 bytes.
       	p += 64;
	// Increment temporary buffer pointer to next block of 64 bytes.
	txbuf += 64;
      }
      // Unlock the FPU.
      kernel_fpu_end();
      // barrier.
      mb();
      // Copy the temporary buffer to the larger event buffer.
      memcpy(bufp, devp->txbuf, 256);
      // Increment the event buffer pointer.
      bufp += 256;
      // Increment the number of bytes written.
      count_lp += 256;
    }
  // Reset the local bus timing to single-cycle access.
  devp->plx->las0brd = 
     PCI9030_BRD_PREFETCH_COUNTER |
     PCI9030_BRD_NBTERM_EN |
     PCI9030_BRD_NREADY_EN |
     PCI9030_BRD_BUS_32;
  DEBUG("surfDriver_datatransfer: %8.8x %8.8x %8.8x %8.8x\n",
	buffer[0], buffer[1], buffer[2], buffer[3]);
   
  return buffer;
}

unsigned int *surfDriver_hktransfer(struct surfDriver_dev *devp,
				      unsigned int *buffer,
				      unsigned int count) {
  int i;
  volatile unsigned int *p;
  unsigned int *bufp;
  /*
   * Test braindead readout.
   */
  p = devp->hk;
  bufp = buffer;
  // Avoid worrying about prefetching here. Not worth it.
  for (i=0;i<count;i+=4) *bufp++ = *p;
  return buffer;
}


