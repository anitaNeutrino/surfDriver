/*
 *
 * surfDriver: surfDriver.h
 *
 * Main header file. Contains device structure definitions.
 *
 * PSA v1.0 03/21/08
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/cdev.h>
#include <linux/semaphore.h>

#include "pci9030.h"
#include "surfDriver_ioctl.h"

#ifndef PCI_VENDOR_ID_PLX
#define PCI_VENDOR_ID_PLX 0x10b5
#endif

#define PLX9030_BAR_PLX   0
#define PLX9030_BAR_PLXIO 1
#define PLX9030_BAR_LAS0  2
#define PLX9030_BAR_LAS1  3
#define PLX9030_BAR_LAS2  4
#define PLX9030_BAR_LAS3  5

#define PCI_DEVICE_ID_PLX9030 0x9030

#define MAX_SURFS 16

// 8 registers, which is 32 bytes.
// So we go from 0x00-0x1F.
#define SURF_REGISTER_BASE     0x00
#define SURF_REGISTER_SIZE     0x20
// Theoretically 128x4 bytes, or
// 512 bytes total. So we go from
// 0x200-0x3FF.
#define SURF_HOUSEKEEPING_BASE 0x200
#define SURF_HOUSEKEEPING_SIZE 0x200
// The LABD buffer is 9 channels, 260 samples, 2 bytes per sample.
// This is 4680 bytes.
// So we go from 0x2000-0x3FFF.
#define SURF_LAB_BASE          0x2000
#define SURF_LAB_SIZE          0x2000

struct surfDriver_regs 
{
   unsigned int ident;
   unsigned int version;
   unsigned int hk_counter;
   unsigned int lab_counter;
   unsigned int buffer_status;
   unsigned int event_id;
   unsigned int clear;
   unsigned int shortmask;
};
   
#define SURF_CLEAR_ALL 0x1
#define SURF_CLEAR_EVT 0x2
#define SURF_UPDATE_DAC 0x4

#define SURF_EVENT_RDY 0x1
#define SURF_EVENT_BUFFER_EMPTY 0x8
#define SURF_BUFFER_MASK 0x6

struct surfDriver_stat {
  unsigned int bytesRead; // total number of bytes pushed through driver
  unsigned int bytesWritten; // ditto, for bytes pushed into the driver
  unsigned int ncycles; // number of cycles that last event transfer took
  unsigned int nevents; // total number of events pushed through driver
};

enum surfDriver_mode {
  surf_DataRead,
  surf_DataWrite, // DAC loading
  surf_HkRead,
  surf_HkWrite
};

// These GPIOs are the corresponding data pins
// in the GPIOc register.
//
// The names are based on the actual names in the firmware.
enum plxGPIO {
  surfGPIO_ED_Ready = (1<<2),  // GPIO0
  surfGPIO_Clr_Event = (1<<5), // GPIO1
  surfGPIO_Clr_All = (1<<8),   // GPIO2
  surfGPIO_RD_notWR = (1<<11), // GPIO3
  surfGPIO_DAT_nHK = (1<<14),  // GPIO4
  surfGPIO_Clr_HK = (1<<17),   // GPIO5
  surfGPIO_Load_DACs = (1<<20), // GPIO6
  surfGPIO_intEnable = (1<<23) // GPIO7
};

enum plxGPIODIR {
  plxDirectionIn = 0,
  plxDirectionOut = 1
};

struct surfDriver_dev {
  int inUse;
  dev_t devno;                        // device number (major/minor chardev)
  struct pci_dev *dev;                // PCI Device Structure
  struct proc_dir_entry *procDir;     // /proc directory
  struct pci9030_regs *plx;           // plx registers.
  struct surfDriver_regs *surf;       // SURF registers.
  volatile unsigned int *hk;          // HK space.
  volatile unsigned int *lab;         // LAB space.
  struct surfDriver_stat statistics;  // per-device statistics
  enum surfDriver_mode mode;          // current device mode
  struct semaphore sema;              // mutex semaphore
  struct cdev cdev;                   // character device
  struct class_device *surfDev;       // SURF class device (for sysfs)
  unsigned long las0addr;             // Original LAS0 address
  unsigned long cacheaddr;            // base cache address (remapped)
  unsigned int *txbuf;		      // Transfer buffer.
};

// sD_main.c
int surfDriver_readoutmode(void);

// sD_char.c
int surfDriver_getmajor(void);
void surfDriver_releasemajor(void);
int surfDriver_registerSurfCharacterDevice(struct surfDriver_dev *dev,
						  int index);
void surfDriver_unregisterSurfCharacterDevice(struct surfDriver_dev *dev);
void surfDriver_enableint(struct surfDriver_dev *devp);
void surfDriver_disableint(struct surfDriver_dev *devp);
int surfDriver_queryint(struct surfDriver_dev *devp);


int surfDriverWrite(struct surfDriver_dev *devp,
		    struct surfDriverRead *readp);
int surfDriverRead(struct surfDriver_dev *devp,
		   struct surfDriverRead *readp);

// sD_plx.c
unsigned int plx9030_gpioc(volatile unsigned int *plxBase,
			   unsigned int *gpiovalp);
void plx9030_toggle(volatile unsigned int *plxBase, enum plxGPIO toggle);
void plx9030_setgpio(volatile unsigned int *plxBase, enum plxGPIO gpio,
		     unsigned char val);
void plx9030_setgpios(volatile unsigned int *plxBase, 
		      enum plxGPIO *gpio,
		      unsigned char *val,
		      unsigned int ngpios);
unsigned int plx9030_getgpio(volatile unsigned int *plxBase, enum plxGPIO gpio);
void plx9030_setgpiodirection(volatile unsigned int *plxBase,
			      enum plxGPIO gpio,
			      enum plxGPIODIR dir);
void surfDriver_setupGPIOs(struct surfDriver_dev *devp);
void surfDriver_setupTiming(struct surfDriver_dev *devp,
			    unsigned char ultrafast);
void surfDriver_remapPlx(struct surfDriver_dev *devp,
			 unsigned long startAddr,
			 int size,
			 int prefetchable);

// sD_transfer.c
int surfDriver_user_datatransfer(struct surfDriver_dev *devp,
				 char __user *buff,
				 unsigned int count);

unsigned int *surfDriver_datatransfer(struct surfDriver_dev *devp,
				      unsigned int *buffer,
				      unsigned int count);
unsigned int *surfDriver_hktransfer(struct surfDriver_dev *devp,
				    unsigned int *buffer,
				    unsigned int count);
// sD_sysfs.c

int surfDriver_addSurfDevice(struct surfDriver_dev *devp);
void surfDriver_removeSurfDevice(struct surfDriver_dev *devp);
int surfDriver_registerSurfClass(void);
void surfDriver_unregisterSurfClass(void);

/*
 * DEBUG...
 */
#define DEBUG(format, ...)
//#define DEBUG(format, ...) printk(KERN_INFO format, ## __VA_ARGS__)
