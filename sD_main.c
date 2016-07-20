/*
 *
 * surfDriver: sD_main.c
 *
 * Main module file. Contains startup/shutdown, driver registration, all
 * that jazz.
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <asm/i387.h>
#include "pci9030.h"
#include "surfDriver.h"

#define SURFDRIVER_VERSION "3.0-16-Sep-14"

typedef struct 
{
   unsigned int blah[8*4+2];
   unsigned long cr0;
}
 align_blah __attribute__((aligned(16)));

#define BLAH(x) ((unsigned int *)((((unsigned long)&(x)->blah)+15) & ~15))

static const struct pci_device_id pci_ids[] = {
  { PCI_VENDOR_ID_PLX, PCI_DEVICE_ID_PLX9030, PCI_ANY_ID, PCI_ANY_ID, },
  { 0,0,0,0 }
};

static int surfDriver_probe(struct pci_dev *dev, const struct pci_device_id *id);
static void surfDriver_remove(struct pci_dev *dev);

static struct pci_driver pci_driver = {
  .name = "surfDriver",
  .id_table = pci_ids,
  .probe = surfDriver_probe,
  .remove = surfDriver_remove,
};

// DEPRECATED
// default cache address: we don't set it up
unsigned int cacheaddr = 0x0;
unsigned int cacheReserved = 0;

// DEPRECATED
// This is where we set up the base prefetchable address space if set
// Every SURF gets 128K up from this, and the upstream bridges get set
// accordingly. This will be a minor nightmare.
module_param(cacheaddr, int, S_IRUSR);

// Set up the transfer mode.
int transfer_mode = 0;
module_param(transfer_mode, int, S_IRUGO | S_IWUSR);

// Exclusion list, if needed. Eventually (rather soon, hopefully)
// there will be a way in the driver (in surfDriver_probe) to tell
// whether the board is a SURF, TURF, or LOS. For now, however, we accept
// an exclusion list of bus:slot combinations (hexadecimal!)
// We translate these into shorts (bus << 8 | devfn) which get matched
// against in surfDriver_probe.
//
// Setup by default to exclude 0xA:0xA, 0x8:0xE. If the exclusion parameter is specified,
// this will be overwritten, so you need to specify ALL 9030 devices to exclude.
static char *exclude[MAX_SURFS];
unsigned int num_exclude=2;
static unsigned short exclude_devno[MAX_SURFS] = {
	(0xA << 8) | PCI_DEVFN(0xA, 0),
	(0x8 << 8) | PCI_DEVFN(0xE, 0),
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
module_param_array(exclude, charp, NULL, S_IRUSR);

#define SURF_PREFETCHABLE_SIZE    0x2000  // 8 kB
#define SURF_PREFETCHABLE_TOTAL 0x100000 //  1 MB
#define SURF_NONPREFETCH_SIZE   0x100000  // 1 MB

// Master SURF list.
struct surfDriver_dev surfList[MAX_SURFS];

static int __init surfDriver_init(void)
{
  int i;
  int result;
  dev_t dev;
  
  printk(KERN_INFO "surfDriver: Kernel driver for the Sampling Unit for RF (SURF)\n");
  printk(KERN_INFO "surfDriver: Author: Patrick Allison\n");
  printk(KERN_INFO "surfDriver: Version: %s\n", SURFDRIVER_VERSION);

  memset(surfList, 0, sizeof(struct surfDriver_dev)*MAX_SURFS);

  // Get the character device major number first,
  // and then the /sysfs class. These need to be done FIRST
  // before the PCI driver is registered, because the probe()
  // functions will be called inside pci_register_device.

  result = surfDriver_getmajor();
  if (result) {
    printk(KERN_ERR "surfDriver: error %d in surfDriver_getmajor\n", result);
    return result;
  }
  result = surfDriver_registerSurfClass();
  if (result) {
    printk(KERN_ERR "surfDriver: error %d in surfDriver_registerSurfClass\n",
	   result);
    return result;
  }

  // Exclusion list?
  for (i=0;i<MAX_SURFS;i++) {
      if (exclude[i] != NULL) {
	  // user is providing a custom exclusion list
	  // ditch the one we had
	  num_exclude = 0;
	  break;
      }
  }
  // It's kinda silly to reloop over them again,
  // but it works.
  for (i=0;i<MAX_SURFS;i++) {
      unsigned short bus, slot;
      if (exclude[i] != NULL) {
	  if (sscanf(exclude[i], "%hx:%hx", &bus, &slot) < 2) {
	      printk(KERN_ERR "surfDriver: did not understand exclusion list entry '%s'\n", exclude[i]);
	  } else {
	      DEBUG("surfDriver: excluding %s - bus %d, slot %d\n",
		exclude[i], bus, slot);	      
	      exclude_devno[num_exclude] = (bus << 8) | PCI_DEVFN(slot, 0);
	      num_exclude++;
	  }
      }
  }
  // Clear the rest of the exclusion list
  for (i=num_exclude;i<MAX_SURFS;i++) {
      exclude_devno[i] = 0;
  }
  
  result = pci_register_driver(&pci_driver);
  if (result) {
    printk("surfDriver: error %d in pci_register_driver\n", result);
    return result;
  }
   
  return 0;
}

int surfDriver_probe(struct pci_dev *dev, const struct pci_device_id *id) {
  unsigned long startAddress, endAddress;
  unsigned short busdev;

  int i;
  DEBUG("surfDriver: surfDriver_probe\n");
  
  DEBUG("surfDriver: surfDriver_probe %x:%x.%x\n", dev->bus->number, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn));

  // Check against exclusion list
  busdev = (dev->bus->number << 8) | (dev->devfn);
  for (i=0;i<num_exclude;i++) {
      if (busdev == exclude_devno[i]) // System said don't use this...
	  return -ENODEV;  // -ENODEV/-ENXIO won't put errors in syslog
  }

  // Find open SURF device
  for (i=0;i<MAX_SURFS;i++) {
    if (!surfList[i].inUse) break;
  }
  if (i == MAX_SURFS) {
    printk(KERN_ERR "surfDriver: this driver can only handle 16 SURF devices\n");
    return -1;
  }
  pci_enable_device(dev);

  // Copy PCI device pointer
  surfList[i].dev = dev;

  /*
   * Obtain the PLX base address. Located at BAR0.
   * We check that no other driver has claimed that area of memory
   * (hopefully...) and then we request it for ourselves. Only after
   * that do we ioremap it into kernel virtual address space, and then
   * it's ours for the life of the driver.
   */
  startAddress = pci_resource_start(dev, PLX9030_BAR_PLX);
  endAddress = pci_resource_end(dev, PLX9030_BAR_PLX);
  // length of the region is endAddress-startAddress+1
  if (check_mem_region(startAddress, endAddress-startAddress+1)) {
    printk(KERN_ERR "surfDriver: memory address region 0x%X-0x%X is in use?\n",
	   (unsigned int) startAddress, (unsigned int) endAddress);
    pci_disable_device(dev);
    return -1;
  }
  request_mem_region(startAddress, endAddress-startAddress+1, "surfDriver");
  surfList[i].plx = (struct pci9030_regs *) ioremap(startAddress, sizeof(struct pci9030_regs));
  DEBUG("surfDriver: plx (0x%X) at 0x%X\n", startAddress, (unsigned int) surfList[i].plx);

  /*
   * We're in slow readout mode.
   */
  // SURF base register.
  startAddress = pci_resource_start(dev, PLX9030_BAR_LAS0);
  endAddress = pci_resource_end(dev, PLX9030_BAR_LAS0);
  // Length is endAddress-startAddress+1
  if (check_mem_region(startAddress, endAddress-startAddress+1)) {
    printk(KERN_ERR "surfDriver: memory address region 0x%X-0x%X is in use?\n",
	   (unsigned int) startAddress, 
	   (unsigned int) endAddress);
    pci_disable_device(dev);
    return -1;
  }
  request_mem_region(startAddress, 
		     endAddress-startAddress+1, 
		     "surfDriver");
  // First map the SURF registers.
  surfList[i].surf = (struct surfDriver_regs *) ioremap(startAddress + SURF_REGISTER_BASE, SURF_REGISTER_SIZE);
  // Next the housekeeping.
  surfList[i].hk = ioremap(startAddress + SURF_HOUSEKEEPING_BASE, SURF_HOUSEKEEPING_SIZE);
  // Finally the LABs. Write-cacheable, so we can use movntqa.
  surfList[i].lab = ioremap_wc(startAddress + SURF_LAB_BASE, SURF_LAB_SIZE);

  // Set up the transfer buffer.
  surfList[i].txbuf = kmalloc(4096, GFP_KERNEL);

  printk("surfDriver: surf registers (%8.8x) at 0x%8.8X\n", 
	 startAddress + SURF_REGISTER_BASE,
	 surfList[i].surf);
  printk("surfDriver: HK space (%8.8x) at 0x%8.8x\n", 
	 startAddress + SURF_HOUSEKEEPING_BASE,
	 surfList[i].hk);
  printk("surfDriver: LAB space (%8.8x)at 0x%8.8x\n", 
	 startAddress + SURF_LAB_BASE,
	 surfList[i].lab);   
  // Set up the GPIOs
  surfDriver_setupGPIOs(&surfList[i]);
  // Set up the bus region descriptor (timing)
  surfDriver_setupTiming(&surfList[i], 0);

     {
	unsigned int id;
	unsigned int version;
	
	id = surfList[i].surf->ident;
	version = surfList[i].surf->version;
	printk("surfDriver: SURF%d is a \"%c%c%c%c\" v%d.%d.%d compiled %d/%d\n", 
	       i,
	       (id>>24)&0xFF,
	       (id>>16)&0xFF, 
	       (id>>8)& 0xFF, 
	       (id>>0)&0xFF, 

	       (version >> 12) & 0xF,
	       (version >> 8) & 0xF,
	       (version) & 0xFF,
	       (version>>24) & 0xF,
	       (version>>16) & 0xFF);
	if ((version & 0xFFFF) >= 0x380E) 
	  {
	     printk("surfDriver: enabling burst read on SURF%d\n",i);
	     surfList[i].surf->lab_counter = 0x80000000;
	  }
     }   
   
  //DEBUG("surfDriver: las0Base (0x%X) at 0x%X\n", startAddress, (unsigned int) surfList[i].las0Base);

  // Initialize semaphore
  sema_init(&surfList[i].sema,1);
  // Register character device
  surfDriver_registerSurfCharacterDevice(&surfList[i], i);
  // Sysfs
  if (surfDriver_addSurfDevice(&surfList[i])) {
     printk(KERN_ERR "surfDriver: unable to register sysfs entry\n");
  }
  surfList[i].inUse = 1;
  
  return 0;
}

static void surfDriver_remove(struct pci_dev *dev) {
  unsigned long startAddress, endAddress;
  int i;
  for (i=0;i<MAX_SURFS;i++) {
    if (surfList[i].dev == dev) {
      down(&surfList[i].sema);
      // remove the sysfs entry
      surfDriver_removeSurfDevice(&surfList[i]);
      surfDriver_unregisterSurfCharacterDevice(&surfList[i]);

      startAddress = pci_resource_start(dev, PLX9030_BAR_LAS0);
      endAddress = pci_resource_end(dev, PLX9030_BAR_LAS0);
      iounmap(surfList[i].surf);
      iounmap(surfList[i].hk);
      iounmap(surfList[i].lab);
      kfree(surfList[i].txbuf);
      /*
       * For non-prefetchable read mode, we need to release
       * the region at startAddress, with size
       * endAddress-startAddress+1 (so if it's 0x0->0x1FFF, it's
       * a region size of 0x2000)
       */
      release_mem_region(startAddress, endAddress-startAddress+1);
      startAddress = pci_resource_start(dev, PLX9030_BAR_PLX);
      endAddress = pci_resource_end(dev, PLX9030_BAR_PLX);      
      iounmap(surfList[i].plx);
      release_mem_region(startAddress, endAddress-startAddress+1);
      pci_disable_device(dev);
      surfList[i].surfDev = 0x0;
      surfList[i].inUse = 0;
      surfList[i].dev = 0x0;
      surfList[i].plx = 0x0;
      surfList[i].surf = 0x0;
      surfList[i].hk = 0x0;
      surfList[i].lab = 0x0;
    }
  }
}

static void __exit surfDriver_exit(void)
{
  DEBUG("surfDriver: surfDriver_exit\n");
  // this calls all of the "surfDriver_remove" functions
  pci_unregister_driver(&pci_driver);  
  // release char major number, now that the character device
  // for each is closed.
  surfDriver_releasemajor();
  // unregister surf /sysfs class
  surfDriver_unregisterSurfClass();
}

int surfDriver_readoutmode(void)
{
  return transfer_mode;
}


module_init(surfDriver_init);
module_exit(surfDriver_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Patrick Allison <patrick@phys.hawaii.edu>");
MODULE_DESCRIPTION("Driver for Sampling Unit for RF (SURF)");
MODULE_VERSION(SURFDRIVER_VERSION);
MODULE_DEVICE_TABLE(pci, pci_ids);
