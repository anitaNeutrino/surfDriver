#ifndef PCI9030_H
#define PCI9030_H

struct pci9030_regs 
{
   unsigned int las0rr;
   unsigned int las1rr;
   unsigned int las2rr;
   unsigned int las3rr;
   unsigned int eromrr;
   unsigned int las0ba;
   unsigned int las1ba;
   unsigned int las2ba;
   unsigned int las3ba;
   unsigned int eromba;
   unsigned int las0brd;
   unsigned int las1brd;
   unsigned int las2brd;
   unsigned int las3brd;
   unsigned int erombrd;
   unsigned int cs0base;
   unsigned int cs1base;
   unsigned int cs2base;
   unsigned int cs3base;
   unsigned short intcsr;
   unsigned short prot_area;
   unsigned int cntrl;
   unsigned int gpioc;
   unsigned int pmdatasel;
   unsigned int pmdatascale;
};

#define PCI9030_BAR_PLX   0
#define PCI9030_BAR_PLXIO 1
#define PCI9030_BAR_LAS0  2
#define PCI9030_BAR_LAS1  3
#define PCI9030_BAR_LAS2  4
#define PCI9030_BAR_LAS3  5

#define PCI9030_BRD_BURST_EN (1<<0)
#define PCI9030_BRD_NREADY_EN (1<<1)
#define PCI9030_BRD_NBTERM_EN (1<<2)
#define PCI9030_BRD_PREFETCH_COUNTER (1<<5)
#define PCI9030_BRD_PREFETCH_NONE (0)
#define PCI9030_BRD_PREFETCH_4 (0x1 << 3)
#define PCI9030_BRD_PREFETCH_8 (0x2 << 3)
#define PCI9030_BRD_PREFETCH_16 (0x3 << 3)
#define PCI9030_BRD_BUS_8 (0x0<<22)
#define PCI9030_BRD_BUS_16 (0x1<<22)
#define PCI9030_BRD_BUS_32 (0x2<<22)

#define PCI9030_GPIOC_GPIO2_NCS2 (1 << 6)
#define PCI9030_GPIOC_GPIO3_NCS3 (1 << 9)

#define PCI9030_INTCSR_LINT1_ENABLE (1 << 0)
#define PCI9030_INTCSR_LINT1_POLARITY_HIGH (1 << 1)
#define PCI9030_INTCSR_LINT1_STATUS (1 << 2)
#define PCI9030_INTCSR_LINT2_ENABLE (1 << 3)
#define PCI9030_INTCSR_LINT2_POLARITY_HIGH (1 << 4)
#define PCI9030_INTCSR_LINT2_STATUS (1 << 5)
#define PCI9030_INTCSR_PCI_ENABLE (1 << 6)
#define PCI9030_INTCSR_SOFTWARE_INTERRUPT (1 << 7)
#define PCI9030_INTCSR_LINT1_EDGE (1 << 8)
#define PCI9030_INTCSR_LINT2_EDGE (1 << 9)
#define PCI9030_INTCSR_LINT1_CLEAR (1 << 10)
#define PCI9030_INTCSR_LINT2_CLEAR (1 << 11)

#define PCI9030_CS_EN (1<<0)
#define PCI9030_CS_PROG(size,base) (( (size) >> 1 ) | ( base ))

#define PCI9030_CNTRL_READ_NO_FLUSH (1 << 16)
#define PCI9030_CNTRL_DEFAULT (0xF << 19)
#define PCI9030_CNTRL_PCI_R22 (0x1 << 14)
#endif
