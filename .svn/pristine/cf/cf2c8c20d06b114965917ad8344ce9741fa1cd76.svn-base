/*
 * surfDriver: sysfs junk. Yeah, whatever, /proc might be easier since I
 *             already know it, but sysfs is The Right Way To Do It
 *             and I might as well learn it.
 */
#include <linux/types.h>
#include <linux/list.h>
#include <linux/sysfs.h>
#include <linux/version.h>
#include "surfDriver.h"

// hackity hack hack for backwards compatibility
// doesn't matter for the SURFs, but this is just here for
// template-type reasons

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,6,13)
#define CLASS_SIMPLE class_simple
#define CLASS_SIMPLE_CREATE class_simple_create
#define CLASS_SIMPLE_DEVICE_ADD class_simple_device_add
#define CLASS_SIMPLE_DESTROY class_simple_destroy
#define CLASS_SIMPLE_DEVICE_REMOVE class_simple_device_remove
#else
#define CLASS_SIMPLE class
#define CLASS_SIMPLE_CREATE class_create
#define CLASS_SIMPLE_DEVICE_ADD class_device_create
#define CLASS_SIMPLE_DESTROY class_destroy
#define CLASS_SIMPLE_DEVICE_REMOVE(a) class_device_destroy(surfClass, a)
#endif

// Functions
ssize_t surfDriver_readReadoutMode(struct class *cls, char *buf);
ssize_t surfDriver_readDeviceSlot(struct class_device *clsDevice, char * buf);
ssize_t surfDriver_readBytesRead(struct class_device *clsDevice, char * buf);
ssize_t surfDriver_readBytesWritten(struct class_device *clsDevice, char * buf);
ssize_t surfDriver_readNCycles(struct class_device *clsDevice, char * buf);
ssize_t surfDriver_readNEvents(struct class_device *clsDevice, char * buf);
ssize_t surfDriver_readGPIOC(struct class_device *clsDevice, char * buf);
ssize_t surfDriver_clearStatistics(struct class_device *clsDevice, 
				   const char *buf,
				   size_t count);
ssize_t surfDriver_readPlxRegisters(struct class_device *clsDevice,
				    char *buf);

// Class definitions...

static struct CLASS_SIMPLE *surfClass;
CLASS_ATTR(readoutMode, S_IRUGO, surfDriver_readReadoutMode, NULL);
// Device definitions...
CLASS_DEVICE_ATTR(slot, S_IRUGO, surfDriver_readDeviceSlot, NULL);
CLASS_DEVICE_ATTR(bytesRead, S_IRUGO, surfDriver_readBytesRead, NULL);
CLASS_DEVICE_ATTR(bytesWritten, S_IRUGO, surfDriver_readBytesWritten, NULL);
CLASS_DEVICE_ATTR(ncycles, S_IRUGO, surfDriver_readNCycles, NULL);
CLASS_DEVICE_ATTR(nevents, S_IRUGO, surfDriver_readNEvents, NULL);
CLASS_DEVICE_ATTR(GPIO, S_IRUGO, surfDriver_readGPIOC, NULL);
CLASS_DEVICE_ATTR(clearStatistics, S_IWUGO, NULL, surfDriver_clearStatistics);
CLASS_DEVICE_ATTR(plxRegisters, S_IRUGO, surfDriver_readPlxRegisters, NULL);

// Class readout
ssize_t surfDriver_readReadoutMode(struct class *cls,
				   char *buf) {
  sprintf(buf, "%d\n", surfDriver_readoutmode());
  return strlen(buf);
}

// Device readout
ssize_t surfDriver_readDeviceSlot(struct class_device *clsDevice,
				  char * buf) {
  struct surfDriver_dev *devp = (struct surfDriver_dev *) clsDevice->class_data;  
  // no need to lock anything here, it's all the same
  sprintf(buf, "%d:%d.%d\n", devp->dev->bus->number, 
	  PCI_SLOT(devp->dev->devfn), 
	  PCI_FUNC(devp->dev->devfn));
  return strlen(buf);
}
ssize_t surfDriver_readBytesRead(struct class_device *clsDevice,
				 char *buf) {
  struct surfDriver_dev *devp = (struct surfDriver_dev *) clsDevice->class_data;
  if (down_interruptible(&devp->sema))
    return 0;
  sprintf(buf, "%d\n", devp->statistics.bytesRead);
  up(&devp->sema);
  return strlen(buf);
}
ssize_t surfDriver_readBytesWritten(struct class_device *clsDevice,
				    char *buf) {
  struct surfDriver_dev *devp = (struct surfDriver_dev *) clsDevice->class_data;
  if (down_interruptible(&devp->sema))
    return 0;
  sprintf(buf, "%d\n", devp->statistics.bytesWritten);
  up(&devp->sema);
  return strlen(buf);
} 
ssize_t surfDriver_readNCycles(struct class_device *clsDevice,
				    char *buf) {
  struct surfDriver_dev *devp = (struct surfDriver_dev *) clsDevice->class_data;
  if (down_interruptible(&devp->sema))
    return 0;
  sprintf(buf, "%d\n", devp->statistics.ncycles);
  up(&devp->sema);
  return strlen(buf);
} 
ssize_t surfDriver_readNEvents(struct class_device *clsDevice,
				    char *buf) {
  struct surfDriver_dev *devp = (struct surfDriver_dev *) clsDevice->class_data;
  if (down_interruptible(&devp->sema))
    return 0;
  sprintf(buf, "%d\n", devp->statistics.nevents);
  up(&devp->sema);
  return strlen(buf);
} 
ssize_t surfDriver_readGPIOC(struct class_device *clsDevice,
			     char *buf) {
  struct surfDriver_dev *devp = (struct surfDriver_dev *) clsDevice->class_data;
  unsigned int gpioval;

  if (down_interruptible(&devp->sema))
    return 0;
  gpioval = plx9030_gpioc(devp->plxBase, NULL);
  up(&devp->sema);
  sprintf(buf, "%11.11o\n", gpioval);
  return strlen(buf);
}

#define PLX9030_REGISTER_SIZE 0x78
ssize_t surfDriver_readPlxRegisters(struct class_device *clsDevice,
				    char *buf) {
    struct surfDriver_dev *devp = (struct surfDriver_dev *) clsDevice->class_data;
    static char myBuffer[16];
    int i;
    volatile unsigned int *plxBase = devp->plxBase;
    buf[0] = 0x0; // null terminate
    if (down_interruptible(&devp->sema))
	return 0;
    for (i=0;i<(PLX9030_REGISTER_SIZE>>2);i++) {
	sprintf(myBuffer, "%2.2X: %8.8X\n", i*sizeof(int), *(plxBase+i));
	strcat(buf, myBuffer);
    }
    up(&devp->sema);
    return strlen(buf);
}
// Class writeto (clear)
ssize_t surfDriver_clearStatistics(struct class_device *clsDevice,
				   const char *buf,
				   size_t count) {
  struct surfDriver_dev *devp = (struct surfDriver_dev *) clsDevice->class_data;
  if (count == 0) return 0;
  if (*buf != '1')
    return 0;
  if (down_interruptible(&devp->sema))
    return 0;
  devp->statistics.nevents = 0;
  devp->statistics.bytesWritten = 0;
  devp->statistics.bytesRead = 0;
  up(&devp->sema);
  return count;
}



int surfDriver_registerSurfClass(void) {
  surfClass = CLASS_SIMPLE_CREATE(THIS_MODULE,"surf");
  if (IS_ERR(surfClass)) {
    printk(KERN_ERR "surfDriver: unable to create sysfs SURF class entry\n");
    surfClass = 0x0;
    return -1;
  }
  class_create_file(surfClass, &class_attr_readoutMode);
  return 0;
}

void surfDriver_unregisterSurfClass(void) {
  if (surfClass==0x0) return;
  class_remove_file(surfClass, &class_attr_readoutMode);
  CLASS_SIMPLE_DESTROY(surfClass);
  surfClass = 0;
}

void surfDriver_removeSurfDevice(struct surfDriver_dev *devp) {
    if (devp->surfDev == 0x0) {
	printk(KERN_ERR "surfDriver: no surf class device for devno %d\n",
	       devp->devno);
	return;
    }
  class_device_remove_file(devp->surfDev, &class_device_attr_slot);
  class_device_remove_file(devp->surfDev, &class_device_attr_bytesRead);
  class_device_remove_file(devp->surfDev, &class_device_attr_bytesWritten);
  class_device_remove_file(devp->surfDev, &class_device_attr_ncycles);
  class_device_remove_file(devp->surfDev, &class_device_attr_nevents);
  class_device_remove_file(devp->surfDev, &class_device_attr_GPIO);
  class_device_remove_file(devp->surfDev, &class_device_attr_clearStatistics);
  class_device_remove_file(devp->surfDev, &class_device_attr_plxRegisters);
  CLASS_SIMPLE_DEVICE_REMOVE(devp->devno);
}

int surfDriver_addSurfDevice(struct surfDriver_dev *devp) {
  unsigned char bus = devp->dev->bus->number;
  unsigned char slot = PCI_SLOT(devp->dev->devfn);
  int err = 0;
  int ret;
  /*
   * Add /sys/class/surf/ entry.
   * devp->dev->dev is the struct device for this SURF.
   */
  if (surfClass==0x0) {
     printk(KERN_ERR "surfDriver: surfClass is null?\n");
     return -1;
  }
  devp->surfDev = CLASS_SIMPLE_DEVICE_ADD(surfClass,
					  (struct class_device *) NULL,
					  devp->devno,
					  &devp->dev->dev,
					  "%2.2d:%2.2d",
					  bus, slot);  
  if (IS_ERR(devp->surfDev)) {
    printk(KERN_ERR "surfDriver: unable to add sysfs entry for surf at %2.2d:%2.2d\n", bus, slot);
    devp->surfDev = 0x0;
    return -1;
  }
  devp->surfDev->class_data = (void *) devp;
  if (ret=class_device_create_file(devp->surfDev, &class_device_attr_slot))
      err = ret;
  if (ret=class_device_create_file(devp->surfDev, &class_device_attr_bytesRead))
      err = ret;
  if (ret=class_device_create_file(devp->surfDev, &class_device_attr_bytesWritten))
      err = ret;
  if (ret=class_device_create_file(devp->surfDev, &class_device_attr_ncycles))
      err = ret;
  if (ret=class_device_create_file(devp->surfDev, &class_device_attr_nevents))
      err = ret;
  if (ret=class_device_create_file(devp->surfDev, &class_device_attr_GPIO))
      err = ret;
  if (ret=class_device_create_file(devp->surfDev, &class_device_attr_clearStatistics))
      err = ret;
  if (ret=class_device_create_file(devp->surfDev, &class_device_attr_plxRegisters))
      err = ret;
  if (err)
      printk(KERN_ERR "surfDriver: error %d adding sysfs entry, gonna go boom!\n", err);
  return err;
}
