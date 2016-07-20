/*
 *
 * surfDriver: sD_plx.c
 *
 * PLX interface. This is pretty non-surfDriver generic stuff.
 *
 * PSA v1.0 03/21/08
 */

#include "surfDriver.h"
#include "pci9030.h"

void surfDriver_setupTiming(struct surfDriver_dev *devp,
			    unsigned char ultrafast) {
    unsigned int val;
    int tmp;
    // Reset the PCI9030.
    tmp = devp->plx->cntrl;
    devp->plx->cntrl = tmp | (1<<30);
    devp->plx->cntrl = tmp;


    devp->plx->las0brd = 
     PCI9030_BRD_PREFETCH_COUNTER |
     PCI9030_BRD_NBTERM_EN |
     PCI9030_BRD_NREADY_EN |
     PCI9030_BRD_BUS_32;
    printk("surfDriver: setting up brd (base %8.8x las0brd %8.8x) : %8.8x\n",
	 devp->plx,
	 &(devp->plx->las0brd),
	 devp->plx->las0brd);
   devp->plx->cntrl = 
     PCI9030_CNTRL_DEFAULT | PCI9030_CNTRL_READ_NO_FLUSH;
   printk("surfDriver: setting up cntrl (base %8.8x cntrl %8.8x) : %8.8x\n",
	  devp->plx,
	  &(devp->plx->cntrl),
	  devp->plx->cntrl);
   // Programming 0x1000 size, 0x1000 base does
   // 0x1801
   // This puts it at a
    devp->plx->cs0base = PCI9030_CS_EN | PCI9030_CS_PROG(SURF_REGISTER_SIZE, SURF_REGISTER_BASE);
    devp->plx->cs2base = PCI9030_CS_EN | PCI9030_CS_PROG(SURF_HOUSEKEEPING_SIZE, SURF_HOUSEKEEPING_BASE);
    devp->plx->cs3base = PCI9030_CS_EN | PCI9030_CS_PROG(SURF_LAB_SIZE, SURF_LAB_BASE);
}

void surfDriver_setupGPIOs(struct surfDriver_dev *devp) {

   devp->plx->gpioc = 
     PCI9030_GPIOC_GPIO2_NCS2 | 
     PCI9030_GPIOC_GPIO3_NCS3;

   printk("surfDriver: setting up gpioc (base %8.8x gpioc %8.8x) : %8.8x\n",
	  devp->plx,
	  &(devp->plx->gpioc),
	  devp->plx->gpioc);
}
