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


ssize_t slot_show(struct device *clsDevice, struct device_attribute *attr, char * buf);
static DEVICE_ATTR_RO(slot);

ssize_t bytesRead_show(struct device *clsDevice, struct device_attribute *attr, char * buf);
static DEVICE_ATTR_RO(bytesRead);

ssize_t bytesWritten_show(struct device *clsDevice, struct device_attribute *attr, char * buf);
static DEVICE_ATTR_RO(bytesWritten);

ssize_t ncycles_show(struct device *clsDevice, struct device_attribute *attr, char * buf);
static DEVICE_ATTR_RO(ncycles);

ssize_t nevents_show(struct device *clsDevice, struct device_attribute *attr, char * buf);
static DEVICE_ATTR_RO(nevents);

ssize_t gpio_show(struct device *clsDevice, struct device_attribute *attr, char * buf);
static DEVICE_ATTR_RO(gpio);

ssize_t clearStatistics_store(struct device *clsDevice, 
			      struct device_attribute *attr,
			      const char *buf,
			      size_t count);
static DEVICE_ATTR_WO(clearStatistics);

ssize_t plxRegisters_show(struct device *clsDevice,
			  struct device_attribute *attr,
			  char *buf);
static DEVICE_ATTR_RO(plxRegisters);
// Class definitions...
static struct attribute *surf_attrs[] = {
  &dev_attr_slot.attr,
  &dev_attr_bytesRead.attr,
  &dev_attr_bytesWritten.attr,
  &dev_attr_ncycles.attr,
  &dev_attr_nevents.attr,
  &dev_attr_gpio.attr,
  &dev_attr_clearStatistics.attr,
  &dev_attr_plxRegisters.attr,
  NULL,
};
ATTRIBUTE_GROUPS(surf);

static struct class_attribute surf_class_attrs[] = {
  __ATTR_NULL,
};

static struct class surfClass = {
  .name = "surf",
  .owner = THIS_MODULE,
  .class_attrs = surf_class_attrs,
  .dev_groups = surf_groups,
};

// Device readout
ssize_t slot_show(struct device *clsDevice,
		  struct device_attribute *attr,
		  char * buf) {
  struct surfDriver_dev *devp = dev_get_drvdata(clsDevice);
  // no need to lock anything here, it's all the same
  sprintf(buf, "%d:%d.%d\n", devp->dev->bus->number, 
	  PCI_SLOT(devp->dev->devfn), 
	  PCI_FUNC(devp->dev->devfn));
  return strlen(buf);
}
ssize_t bytesRead_show(struct device *clsDevice,
		       struct device_attribute *attr,
		       char *buf) {
  struct surfDriver_dev *devp = dev_get_drvdata(clsDevice);
  if (down_interruptible(&devp->sema))
    return 0;
  sprintf(buf, "%d\n", devp->statistics.bytesRead);
  up(&devp->sema);
  return strlen(buf);
}
ssize_t bytesWritten_show(struct device *clsDevice,
			  struct device_attribute *attr,
			  char *buf) {
  struct surfDriver_dev *devp = dev_get_drvdata(clsDevice);
  if (down_interruptible(&devp->sema))
    return 0;
  sprintf(buf, "%d\n", devp->statistics.bytesWritten);
  up(&devp->sema);
  return strlen(buf);
} 
ssize_t ncycles_show(struct device *clsDevice,
		     struct device_attribute *attr,
		     char *buf) {
  struct surfDriver_dev *devp = dev_get_drvdata(clsDevice);
  if (down_interruptible(&devp->sema))
    return 0;
  sprintf(buf, "%d\n", devp->statistics.ncycles);
  up(&devp->sema);
  return strlen(buf);
} 
ssize_t nevents_show(struct device *clsDevice,
		     struct device_attribute *attr,
		     char *buf) {
  struct surfDriver_dev *devp = dev_get_drvdata(clsDevice);
  if (down_interruptible(&devp->sema))
    return 0;
  sprintf(buf, "%d\n", devp->statistics.nevents);
  up(&devp->sema);
  return strlen(buf);
} 
ssize_t gpio_show(struct device *clsDevice,
		  struct device_attribute *attr,
		  char *buf) {
  struct surfDriver_dev *devp = dev_get_drvdata(clsDevice);
  unsigned int gpioval;

  if (down_interruptible(&devp->sema))
    return 0;
  gpioval = devp->plx->gpioc;
  up(&devp->sema);
  sprintf(buf, "%11.11o\n", gpioval);
  return strlen(buf);
}

#define PLX9030_REGISTER_SIZE 0x78
ssize_t plxRegisters_show(struct device *clsDevice,
			  struct device_attribute *attr,
			  char *buf) {
  struct surfDriver_dev *devp = dev_get_drvdata(clsDevice);
    static char myBuffer[16];
    int i;
    volatile unsigned int *plxBase = (unsigned int *) devp->plx;
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
ssize_t clearStatistics_store(struct device *clsDevice,
			     struct device_attribute *attr,
				   const char *buf,
				   size_t count) {
  struct surfDriver_dev *devp = dev_get_drvdata(clsDevice);
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
  int status;
  status = class_register(&surfClass);
  if (status < 0) {
    printk(KERN_ERR "surfDriver: unable to create sysfs SURF class entry\n");
    return status;
  }
  // Don't think this is necessary anymore.
  //  class_create_file(surfClass, &class_attr_readoutMode);
  return 0;
}

void surfDriver_unregisterSurfClass(void) {
  class_unregister(&surfClass);
}

void surfDriver_removeSurfDevice(struct surfDriver_dev *devp) {
  if (devp->surfDev == 0x0) {
    printk(KERN_ERR "surfDriver: no surf class device for devno %d\n",
	   devp->devno);
    return;
  }
  device_destroy(&surfClass, devp->devno);
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
  devp->surfDev = device_create(&surfClass,
				(struct device *) NULL,
				devp->devno,
				devp,
				"%2.2d:%2.2d",
				bus, slot);  
  if (IS_ERR(devp->surfDev)) {
    printk(KERN_ERR "surfDriver: unable to add sysfs entry for surf at %2.2d:%2.2d\n", bus, slot);
    devp->surfDev = 0x0;
    return -1;
  }
  // I think this is all automatic now.
  /*
  if (ret=device_create_file(devp->surfDev, &device_attr_slot))
      err = ret;
  if (ret=device_create_file(devp->surfDev, &device_attr_bytesRead))
      err = ret;
  if (ret=device_create_file(devp->surfDev, &device_attr_bytesWritten))
      err = ret;
  if (ret=device_create_file(devp->surfDev, &device_attr_ncycles))
      err = ret;
  if (ret=device_create_file(devp->surfDev, &device_attr_nevents))
      err = ret;
  if (ret=device_create_file(devp->surfDev, &device_attr_GPIO))
      err = ret;
  if (ret=device_create_file(devp->surfDev, &device_attr_clearStatistics))
      err = ret;
  if (ret=device_create_file(devp->surfDev, &device_attr_plxRegisters))
      err = ret;
  if (err)
      printk(KERN_ERR "surfDriver: error %d adding sysfs entry, gonna go boom!\n", err);
  */

  return err;
}
