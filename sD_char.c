/*
 *
 * surfDriver: sD_char.c
 *
 * surfDriver character interface.
 *
 * PSA v1.0 03/21/08
 */
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/ioctl.h>
#include <linux/fcntl.h>
#include <linux/cdev.h>
#include <asm/msr.h>
#include <asm/uaccess.h>
#include <linux/delay.h>
#include "surfDriver.h"

static int surfDriver_major;

static int surfDriver_open(struct inode *iptr, struct file *filp);
static int surfDriver_release(struct inode *iptr, struct file *filp);
static ssize_t surfDriver_read(struct file *filp, char __user *buff,
			size_t count, loff_t *offp);
static ssize_t surfDriver_write(struct file *filp, const char __user *buff,
			size_t count, loff_t *offp);
static long surfDriver_ioctl(struct file *filp,
		      unsigned int cmd, unsigned long arg);
int surfDriver_loaddac(struct surfDriver_dev *devp, unsigned int *buffer, unsigned int length);
void surfDriver_setmode(struct surfDriver_dev *devp, 
			enum surfDriver_mode mode);
int surfDriver_isdataready(struct surfDriver_dev *devp);
int surfDriver_housekeepingRequest(struct surfDriver_dev *devp,
				    struct surfHousekeepingRequest *reqp);

static struct file_operations surfDriver_fops = {
  .owner = THIS_MODULE,
  .read = surfDriver_read,
  .write = surfDriver_write,
  .unlocked_ioctl = surfDriver_ioctl,
  .open = surfDriver_open,
  .release = surfDriver_release,
};

void __exit surfDriver_releasemajor(void) {
   dev_t dev;
   if (!surfDriver_major) return;
   dev = MKDEV(surfDriver_major, 0);
   unregister_chrdev_region(dev, MAX_SURFS);
}


int __init surfDriver_getmajor(void) {
  int result;
  dev_t dev;

  if (surfDriver_major) {
    dev = MKDEV(surfDriver_major, 0);
    result = register_chrdev_region(dev, MAX_SURFS, "surfDriver");
  } else {
    result = alloc_chrdev_region(&dev, 0, MAX_SURFS, "surfDriver");
    surfDriver_major = MAJOR(dev);
  }
  if (result < 0) {
    printk(KERN_WARNING "surfDriver: can't get major %d\n",
	   surfDriver_major);
    return result;
  }
  DEBUG("surfDriver: got major number %d\n", surfDriver_major);
  return 0;
}

int surfDriver_registerSurfCharacterDevice(struct surfDriver_dev *dev,
				   int index)
{
  int err=0;
  dev_t devno = MKDEV(surfDriver_major, index);

  DEBUG("surfDriver_registerSurfCharacterDevice: adding surf%d\n",
	index);
  cdev_init(&dev->cdev, &surfDriver_fops);
  dev->cdev.owner = THIS_MODULE;
  //  dev->cdev.ops = &surfDriver_fops;
  dev->devno = devno;
  err = cdev_add(&dev->cdev, devno, 1);
  if (err)
    printk(KERN_NOTICE "surfDriver: error %d adding surf%d", err, index);

  return err;
}

void surfDriver_unregisterSurfCharacterDevice(struct surfDriver_dev *dev) {
  cdev_del(&dev->cdev);
}

/*
 *  THE FOPS FUNCTIONS
 *
 *  These are the file operations (fops) functions which lock the SURF:
 *     surfDriver_ioctl
 *     surfDriver_read
 *     surfDriver_write
 *  These do not:
 *     surfDriver_open
 *     surfDriver_release
 */
static int surfDriver_open(struct inode *iptr, struct file *filp)
{
  /*
   * Get the surfDriver_dev pointer by bouncing out
   * of the cdev structure.
   */
  struct surfDriver_dev *devp = container_of(iptr->i_cdev,
					     struct surfDriver_dev,
					     cdev);
  /*
   * Store it for easier access.
   */
  filp->private_data = devp;
  /*
   * Do we need to do anything here?
   */ 
  DEBUG("surfDriver_open: /dev/surf%d opened\n",
	MINOR(iptr->i_rdev) & 0xF);
  return 0;
}
static int surfDriver_release(struct inode *inode, struct file *filp)
{
  /*
   * uh, I don't care
   */
  return 0;
}

/*
 *  THE READ FUNCTION: All bow down before it. Question not what it does.
 */
static ssize_t surfDriver_read(struct file *filp, char __user *buff,
			size_t count, loff_t *offp) {
  int isdataready = 0;
  struct surfDriver_dev *devp = filp->private_data;
  unsigned char *buffer;
  unsigned long long before, after;
  int ret;
   
  /* What mode are we in? */
  switch(devp->mode) {
  case surf_DataRead:
  case surf_DataWrite:
    DEBUG("surfDriver_read (surf_DataRead): %d bytes\n", count);
    /* Get the device. */
    if (down_interruptible(&devp->sema))
      return -ERESTARTSYS;
    /* OK, we've got it. */
    isdataready = surfDriver_isdataready(devp);
    DEBUG("surfDriver_read (surf_DataRead): isdataready: %d\n", isdataready);

    // we want to read data. Holy crap, are you serious?
    if (!isdataready) {
      // no data
      up(&devp->sema);
      // Hopefully you opened me O_NONBLOCK, because blocking on this guy
      // suuuuucks without interrupts.
      if (filp->f_flags & O_NONBLOCK) { // smart move, bucko
	return -EAGAIN;
      }
      // Wow, you don't listen, do you?
      // Okay, then, time to poll, poll, poll...
      while (!isdataready) {
	msleep(10); // 10 ms
	if (down_interruptible(&devp->sema))
	  return -ERESTARTSYS;
	isdataready=surfDriver_isdataready(devp);
	if (!isdataready) up(&devp->sema);
      }
    }
    rdtscll(before); 
    if (ret = surfDriver_user_datatransfer(devp, buff, count)) 
       {
	  up(&devp->sema);
	  return ret;
       }
    up(&devp->sema);
    rdtscll(after);
    devp->statistics.nevents++;
    devp->statistics.ncycles = after-before;
    break;
  case surf_HkRead:
  case surf_HkWrite:
    DEBUG("surfDriver_read (surf_HkRead): %d bytes\n", count);
    /* Get the device. */
    if (down_interruptible(&devp->sema))
      return -ERESTARTSYS;
    /* OK, we've got it. */
    buffer = kmalloc(count, GFP_KERNEL);
    memset(buffer, 0, count);
    surfDriver_hktransfer(devp, buffer, count);
    // we're done with the SURF
    up(&devp->sema);
    if (copy_to_user(buff, buffer, count)) {
      kfree(buffer);
      return -EFAULT;
    }
    kfree(buffer);
    break;
  }
  devp->statistics.bytesRead += count;
  return count;
}
      
static ssize_t surfDriver_write(struct file *filp, const char __user *buff,
			 size_t count, loff_t *offp) {
  struct surfDriver_dev *devp = filp->private_data;
  unsigned int *buffer;

  DEBUG("surfDriver_write: %d bytes\n", count);

  // got the device
  buffer = kmalloc(count, GFP_KERNEL);
  if (copy_from_user(buffer, buff, count)) {
    kfree(buffer);
    return -EFAULT;
  }
  if (down_interruptible(&devp->sema)) {
    kfree(buffer);
    return -ERESTARTSYS;
  }
  surfDriver_loaddac(devp, buffer, count);
  up(&devp->sema);
  kfree(buffer);
  devp->statistics.bytesWritten += count;
  return count;
}

static long surfDriver_ioctl(struct file *filp,
			     unsigned int cmd, unsigned long arg)
{
  int err = 0;
  unsigned short retval;
  struct surfHousekeepingRequest req;
  struct surfDriverRead rd;  
  /*
   * Get surfDriver_dev pointer.
   */
  struct surfDriver_dev *devp = filp->private_data;
  struct inode *iptr = filp->f_path.dentry->d_inode;

  if (_IOC_TYPE(cmd) != SURF_IOC_MAGIC) return -ENOTTY;
  if (_IOC_NR(cmd) > SURF_IOC_MAXNR) return -ENOTTY;
  if (_IOC_DIR(cmd) & _IOC_READ)
      err = !access_ok(VERIFY_WRITE, (void __user *) arg, _IOC_SIZE(cmd));
  if (err) return -EFAULT;

  if (!devp) {
    printk(KERN_ERR "surfDriver: no surfDriver_dev pointer?\n");
    return -EFAULT;
  }
  DEBUG("surfDriver: ioctl\n");
  /*
   * Get exclusive access to the device.
   */
  if (down_interruptible(&devp->sema))
    return -ERESTARTSYS;
  /* Here is where we switch reading modes (HK/DATA)
     and also clear the event */
  switch (cmd) {
      case SURF_IOCHK:
	  // switch to housekeeping (read) mode.
	  // We ALWAYS switch back to 'read' mode. Only the "write"
	  // functions use the 'write' modes.
	  DEBUG("surfDriver: surf%d put in HK mode\n",
		iminor(iptr) & 0xF);
	  surfDriver_setmode(devp, surf_HkRead);
	  break;
      case SURF_IOCDATA:
	  // switch to data (read) mode
	  // See above
	  DEBUG("surfDriver: surf%d put in Data mode\n",
		iminor(iptr) & 0xF);
	  surfDriver_setmode(devp, surf_DataRead);
	  break;
      case SURF_IOCCLEARALL:
	  DEBUG("surfDriver: Clr_All surf%d\n",
		iminor(iptr) & 0xF);
          devp->surf->clear = 0x1;
	  break;
      case SURF_IOCCLEARHK:
	  // clear housekeeping (toggle Clr_HK GPIO)
	  DEBUG("surfDriver: Clr_HK surf%d\n",
		iminor(iptr) & 0xF);
	  // FIXME
	  // Write to housekeeping counter.
          devp->surf->hk_counter = 0;
          break;
      case SURF_IOCCLEAREVENT:
	  // clear event (toggle Clr_Event GPIO)
	  DEBUG("surfDriver: Clr_Event surf%d\n",
		iminor(iptr) & 0xF);
          devp->surf->clear = 0x2;
	  break;
      case SURF_IOCEVENTREADY:
	  // return 0 if no data, 1 if data is available
	  err = surfDriver_isdataready(devp);
	  break;
      case SURF_IOCENABLEINT:
	  surfDriver_enableint(devp);
	  break;
      case SURF_IOCDISABLEINT:
	  surfDriver_disableint(devp);
	  break;
      case SURF_IOCQUERYINT:
	  err = surfDriver_queryint(devp);
	  break;
      case SURF_IOCGETSLOT:
	  retval = ((devp->dev->bus->number) << 8) | (devp->dev->devfn);
	  err = __put_user(retval, (unsigned short __user *) arg);
	  break;
      case SURF_IOCHKREAD:
	  // Copy the pointer from the user...
	  if (copy_from_user(&req, 
			     (struct surfHousekeepingRequest __user *) arg,
			     sizeof(struct surfHousekeepingRequest))) {
	      return -EFAULT;
	  }
	  err = surfDriver_housekeepingRequest(devp, &req);
	  break;
      case SURF_IOCREAD:
	  if (copy_from_user(&rd,
			     (struct surfDriverRead __user *) arg,
			     sizeof(struct surfDriverRead))) {
	      return -EFAULT;
	  }
	  err = surfDriverRead(devp, &rd);
	  if (copy_to_user((struct surfDriverRead __user *) arg,
			   &rd, sizeof(struct surfDriverRead))) {
	      return -EFAULT;
	  }
	  break;
      case SURF_IOCWRITE:
	  if (copy_from_user(&rd,
			     (struct surfDriverRead __user *) arg,
			     sizeof(struct surfDriverRead))) {
	      return -EFAULT;
	  }
	  err = surfDriverWrite(devp, &rd);
	  if (copy_to_user((struct surfDriverRead __user *) arg,
			   &rd, sizeof(struct surfDriverRead))) {
	      return -EFAULT;
	  }
	  break;
      default:
	  err=-ENOTTY;
	  break;
  }
  up(&devp->sema);
  return err;
}

/*
 * Loads the DACs. Does not lock the SURF (only done in fops functions!)
 */
int surfDriver_loaddac(struct surfDriver_dev *devp, 
		       unsigned int *buffer, 
		       unsigned int length) {
  int i;
  unsigned int *p;
  volatile unsigned int *ioaddr;
  enum surfDriver_mode mode;
  // All we do for loading the DACs is switch to Hk mode, switch to
  // Write mode (Hk isn't really needed, but meh), and do I/O writes in a loop.
  // Length is determined by the number of bytes written to the device.

  // We then toggle DACLOAD at the end.
  
  if (!devp) { // erwha?
    printk(KERN_ERR "surfDriver_loaddac: devp is null, Patrick can't write software.\n");
    return -EFAULT;
  }
  if (length < sizeof(unsigned int)*32) 
     {
	memcpy(devp->hk, buffer, length);
     }   
   else
     {
	unsigned int temp;
	memcpy(devp->hk, buffer, sizeof(unsigned int)*32);
	if (length >= sizeof(unsigned int)*34) 
	  {	     
	     temp = buffer[32] & 0xFFFF;
	     temp |= ((buffer[33] & 0xFFFF) << 16);
	     DEBUG("surfDriver_loaddac: writing %8.8x to short_mask\n", temp);
	     devp->surf->shortmask = temp;
	  }	
     }   
  // And write update DAC flag.
  devp->surf->clear = SURF_UPDATE_DAC;
   
   /*
  // save mode...
  mode = devp->mode;
  // put the SURF in Data Writing mode...
  surfDriver_setmode(devp, surf_HkWrite);

  p = buffer;
  ioaddr = devp->las0Base;

  // and write the data
  for (i=0;i<length;i+=4) {
// don't need this anymore...
//    DEBUG("0x%X = 0x%X\n", ioaddr, *p);
    *ioaddr++ = *p++;
  }
  
  // toggle DACLOAD...
  plx9030_toggle(devp->plxBase, surfGPIO_Load_DACs);

  // and put the SURF back where it was
  surfDriver_setmode(devp, mode);
  */  
  return 0;
}

int surfDriverRead(struct surfDriver_dev *devp,
		   struct surfDriverRead *readp) {
   if (readp->address < SURF_REGISTER_SIZE) 
     {
	unsigned int *p = (unsigned int *) devp->surf;
	p += readp->address;
	readp->value = *p;
     }
   else if (readp->address >= (SURF_HOUSEKEEPING_BASE>>2) &&
	    readp->address < ((SURF_HOUSEKEEPING_BASE>>2)+(SURF_HOUSEKEEPING_SIZE>>2)))
     {
	unsigned int *p = (unsigned int *) devp->hk;
	p += readp->address - (SURF_HOUSEKEEPING_SIZE>>2);
	readp->value = *p;
     }
   
   return 0;
}

int surfDriverWrite(struct surfDriver_dev *devp,
		   struct surfDriverRead *readp) {
   if (readp->address < SURF_REGISTER_SIZE) 
     {
	unsigned int *p = (unsigned int *) devp->surf;
	p += readp->address;
	*p = readp->value;
     }
   else if (readp->address >= (SURF_HOUSEKEEPING_BASE>>2) &&
	    readp->address < ((SURF_HOUSEKEEPING_BASE>>2)+(SURF_HOUSEKEEPING_SIZE>>2)))
     {
	unsigned int *p = (unsigned int *) devp->hk;
	p += readp->address - (SURF_HOUSEKEEPING_SIZE>>2);
	*p = readp->value;
     }   
   return 0;
}

int surfDriver_housekeepingRequest(struct surfDriver_dev *devp,
				    struct surfHousekeepingRequest *reqp) {
    unsigned int *buffer;
    enum surfDriver_mode mode;

    DEBUG("surfDriver_housekeepingRequest: %d bytes\n",
	   reqp->length);
    buffer = (unsigned int *) kmalloc(reqp->length, GFP_KERNEL);
    if (!buffer) return -EFAULT;
    devp->surf->hk_counter = 0;
    /*
    mode = devp->mode;
    // make sure address pointer is reset...
    plx9030_toggle(devp->plxBase, surfGPIO_Clr_HK);
    surfDriver_setmode(devp, surf_HkRead);
    surfDriver_hktransfer(devp, buffer, reqp->length);
    // reset address pointer
    plx9030_toggle(devp->plxBase, surfGPIO_Clr_HK);
    surfDriver_setmode(devp, mode);
    */
    surfDriver_hktransfer(devp, buffer, reqp->length);
    if (copy_to_user(reqp->address, buffer, reqp->length)) {
	return -EFAULT;
    }
    return 0;
}


/*
 * Sets modes.
 * Does not lock the SURF (only in fops functions!)
 */
void surfDriver_setmode(struct surfDriver_dev *devp, 
			enum surfDriver_mode mode) {
  enum plxGPIO gpios[2] = {surfGPIO_RD_notWR, surfGPIO_DAT_nHK};
  unsigned char val[2];
  volatile unsigned int *plxBase;

  if (!devp) { // erwha?
    printk(KERN_ERR "surfDriver_setmode: devp is null, Patrick can't write software.\n");
    return;
  }
  // This is all we need to do now. HK/data/register mode is
  // autoselected based on CS2/CS3.
  devp->mode = mode;
  /*
  plxBase = devp->plxBase;
  if (devp->mode == mode) return; // nothing to do 
  switch (mode) {
  case surf_DataRead:
    val[0] = 1; // RD_notWr
    val[1] = 1; // DAT_nHK
    break;
  case surf_DataWrite:
    val[0] = 0; // RD_notWr
    val[1] = 1; // DAT_nHK
    break;
  case surf_HkRead:
    val[0] = 1; // RD_notWr
    val[1] = 0; // DAT_nHK
    break;
  case surf_HkWrite:
    val[0] = 0; // RD_notWr
    val[1] = 0; // DAT_nHK
    break;
  }

  plx9030_setgpios(plxBase, gpios, val, 2);
  devp->mode = mode;
  */
}

int surfDriver_isdataready(struct surfDriver_dev *devp) {
   if (devp->surf->buffer_status & 0x1) return surfGPIO_ED_Ready;
   else return 0;
}

void surfDriver_enableint(struct surfDriver_dev *devp) {
  devp->plx->gpioc |= surfGPIO_intEnable;
}

void surfDriver_disableint(struct surfDriver_dev *devp) {
  devp->plx->gpioc &= ~surfGPIO_intEnable;
}

int surfDriver_queryint(struct surfDriver_dev *devp) {
  return ((devp->plx->gpioc & surfGPIO_intEnable) == surfGPIO_intEnable);
}

